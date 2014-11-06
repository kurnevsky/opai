#include "config.h"
#include "uct.h"
#include "player.h"
#include "field.h"
#include <limits>
#include <queue>
#include <vector>
#include <algorithm>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <omp.h>

using namespace std;
using namespace boost;

// Play random game and get result.
// field - field to play.
// gen - random number generator.
// possibleMoves - allowed positions of moves.
// Returns number of winner, or -1 if draw.
int playRandomGame(Field* field, mt19937* gen, vector<int>* possibleMoves)
{
  vector<int> moves(possibleMoves->size());
  int putted = 0, result;
  moves[0] = (*possibleMoves)[0];
  for (size_t i = 1; i < possibleMoves->size(); i++)
  {
    uniform_int_distribution<size_t> dist(0, i);
    size_t j = dist(*gen);
    moves[i] = moves[j];
    moves[j] = (*possibleMoves)[i];
  }
  for (auto i = moves.begin(); i < moves.end(); i++)
    if (field->isPuttingAllowed(*i))
    {
      field->doUnsafeStep(*i);
      putted++;
    }
  if (field->getScore(playerRed) > 0)
    result = playerRed;
  else if (field->getScore(playerBlack) > 0)
    result = playerBlack;
  else
    result = -1;
  for (int i = 0; i < putted; i++)
    field->undoStep();
  return result;
}

void finalUctSiblings(uctNode* n)
{
  if (n->sibling != nullptr)
    finalUctSiblings(n->sibling);
  delete n;
}

// Create children of UCT node.
// field - field for creating children.
// possibleMoves - allowed positions of moves.
// node - UCT node for creating children.
void createChildren(Field* field, vector<int>* possibleMoves, uctNode* node)
{
  uctNode* children = nullptr;
  uctNode* null = nullptr;
  uctNode** curChild = &children;
  for (auto i = possibleMoves->begin(); i < possibleMoves->end(); i++)
    if (field->isPuttingAllowed(*i))
    {
      *curChild = new uctNode();
      (*curChild)->move = *i;
      curChild = &(*curChild)->sibling;
    }
  if (children != nullptr && !node->child.compare_exchange_strong(null, children, std::memory_order_relaxed))
  {
    finalUctSiblings(children);
  }
}

// Calculate UCB estimation of UCT node.
// parent - parent of UCT node.
// node - node for calculating UCB estimate.
// Returns UCB estimate.
double ucb(uctNode* parent, uctNode* node)
{
  int wins = node->wins.load(std::memory_order_relaxed);
  int draws = node->draws.load(std::memory_order_relaxed);
  int visits = node->visits.load(std::memory_order_relaxed);
  int parentVisits = parent->visits.load(std::memory_order_relaxed);
#if UCB_TYPE == 0
  double winRate = (wins + draws * UCT_DRAW_WEIGHT) / visits;
  double uct = UCTK * sqrt(2 * log(parentVisits) / visits);
  return winRate + uct;
#elif UCB_TYPE == 1
  double winRate = (wins + draws * UCT_DRAW_WEIGHT) / visits;
  double v = (wins + draws * UCT_DRAW_WEIGHT * UCT_DRAW_WEIGHT) / visits - winRate * winRate + sqrt(2 * log(parentVisits) / visits);
  double uct = UCTK * sqrt(min(0.25, v) * log(parentVisits) / visits);
  return winRate + uct;
#else
#error Invalid UCB_TYPE.
#endif
}

// Find child of UCT node with best UCB estimation.
// gen - random number generator.
// node - node to find.
// Returns child with best UCB estimation.
uctNode* uctSelect(mt19937* gen, uctNode* node)
{
  double bestUct = 0, uctValue;
  uctNode* result = nullptr;
  uctNode* next = node->child.load(std::memory_order_relaxed);
  while (next != nullptr)
  {
    int visits = next->visits.load(std::memory_order_relaxed);
    int wins = next->wins.load(std::memory_order_relaxed);
    if (visits == numeric_limits<int>::max())
    {
      if (wins == numeric_limits<int>::max())
        uctValue = 100000;
      else
        uctValue = -1;
    }
    else if (next->visits > 0)
    {
      uctValue = ucb(node, next);
    }
    else
    {
      uniform_int_distribution<int> dist(0, 999);
      uctValue = 10000 + dist(*gen);
    }
    if (uctValue > bestUct)
    {
      bestUct = uctValue;
      result = next;
    }
    next = next->sibling;
  }
  return result;
}

// Play one UCT simulation.
// field - field to play simulation.
// gen - random number generator.
// possibleMoves - allowed positions of moves.
// node - UCT node to play simulation.
// depth - current depth of UCT simulation.
// Returns number of winner, or -1 if draw.
int playSimulation(Field* field, mt19937* gen, vector<int>* possibleMoves, uctNode* node, int depth)
{
  int randomResult;
  if (node->visits < UCT_WHEN_CREATE_CHILDREN || depth == UCT_DEPTH)
  {
    randomResult = playRandomGame(field, gen, possibleMoves);
  }
  else
  {
    if (node->child.load() == nullptr)
      createChildren(field, possibleMoves, node);
    uctNode* next = uctSelect(gen, node);
    if (next == nullptr)
    {
      node->visits = numeric_limits<int>::max();
      if (field->getScore(nextPlayer(field->getPlayer())) > 0)
        node->wins = numeric_limits<int>::max();
      if (field->getScore(playerRed) > 0)
        return playerRed;
      else if (field->getScore(playerBlack) > 0)
        return playerBlack;
      else
        return -1;
    }
    field->doUnsafeStep(next->move);
    if (field->getDeltaScore() < 0)
    {
      field->undoStep();
      next->visits = numeric_limits<int>::max();
      return playSimulation(field, gen, possibleMoves, node, depth);
    }
    randomResult = playSimulation(field, gen, possibleMoves, next, depth + 1);
    field->undoStep();
  }
  node->visits++;
  if (randomResult == nextPlayer(field->getPlayer()))
    node->wins++;
  else if (randomResult == -1)
    node->draws++;
  return randomResult;
}

// Generate possible moves for UCT. It is moves which are spaced by not more than UCT_RADIUS cells from the putted points.
// field - field to generate possible moves.
// possibleMoves - container to put possible moves.
template<typename _Cont>
void generatePossibleMoves(Field* field, _Cont* possibleMoves)
{
  int length = field->getLength();
  int* rField = new int[length];
  fill_n(rField, length, 0);
  queue<int> q;
  possibleMoves->clear();
  for (int i = field->minPos(); i <= field->maxPos(); i++)
    if (field->isPutted(i))
      q.push(i);
  while (!q.empty())
  {
    int front = q.front();
    if (field->isPuttingAllowed(front))
      possibleMoves->push_back(front);
    if (rField[front] < UCT_RADIUS)
    {
      int nFront = field->n(front);
      if (field->isPuttingAllowed(nFront) && rField[nFront] == 0)
      {
        rField[nFront] = rField[front] + 1;
        q.push(nFront);
      }
      int sFront = field->s(front);
      if (field->isPuttingAllowed(sFront) && rField[sFront] == 0)
      {
        rField[sFront] = rField[front] + 1;
        q.push(sFront);
      }
      int wFront = field->w(front);
      if (field->isPuttingAllowed(wFront) && rField[wFront] == 0)
      {
        rField[wFront] = rField[front] + 1;
        q.push(wFront);
      }
      int eFront = field->e(front);
      if (field->isPuttingAllowed(eFront) && rField[eFront] == 0)
      {
        rField[eFront] = rField[front] + 1;
        q.push(eFront);
      }
    }
    q.pop();
  }
  delete[] rField;
}

// Delete UCT tree.
// n - start UCT node.
void finalUctNode(uctNode* n)
{
  uctNode* child = n->child.load();
  if (child != nullptr)
    finalUctNode(child);
  if (n->sibling != nullptr)
    finalUctNode(n->sibling);
  delete n;
}

// Get best move by UCT analysis. If maxSimulations != numeric_limits<int>::max() then needBreak ignored for optimization.
// field - field to find best move.
// gen - random number generator.
// maxSimulations - number of UCT simulations.
// needBreak - true if break is needed.
// Returns position of best move, or -1 if not found.
int uct(Field* field, mt19937_64* gen, int maxSimulations, bool* needBreak)
{
  // List of all possible moves for UCT.
  vector<int> moves;
  generatePossibleMoves(field, &moves);
  if (static_cast<size_t>(omp_get_max_threads()) > moves.size())
    omp_set_num_threads(moves.size());
  uctNode n;
  createChildren(field, &moves, &n);
  #pragma omp parallel
  {
    Field* localField = new Field(*field);
    uniform_int_distribution<int> localDist(numeric_limits<int>::min(), numeric_limits<int>::max());
    mt19937* localGen;
    #pragma omp critical
    localGen = new mt19937(localDist(*gen));
    if (maxSimulations == numeric_limits<int>::max())
    {
      while (!*needBreak)
        playSimulation(localField, localGen, &moves, &n, 0);
    }
    else
    {
      #pragma omp for
      for (int i = 0; i < maxSimulations; i++)
        playSimulation(localField, localGen, &moves, &n, 0);
    }
    delete localGen;
    delete localField;
  }
  double bestUct = 0;
  int result = -1;
  uctNode* next = n.child;
  while (next != nullptr)
  {
    if (next->visits != 0)
    {
      double uctValue = ucb(&n, next);
      if (uctValue > bestUct)
      {
        bestUct = uctValue;
        result = next->move;
      }
    }
    next = next->sibling;
  }
  if (n.child != nullptr)
    finalUctNode(n.child);
  return result;
}

// Get best move by UCT analysis.
// field - field to find best move.
// gen - random number generator.
// maxSimulations - number of UCT simulations.
// Returns position of best move, or -1 if not found.
int uct(Field* field, mt19937_64* gen, int maxSimulations)
{
  bool needBreak = false;
  return uct(field, gen, maxSimulations, &needBreak);
}

// Get best move by UCT analysis.
// field - field to find best move.
// gen - random number generator.
// time - number of milliseconds to thinking.
// Returns position of best move, or -1 if not found.
int uctWithTime(Field* field, mt19937_64* gen, int time)
{
  bool needBreak = false;
  asio::io_service io;
  asio::deadline_timer timer(io, posix_time::milliseconds(time));
  thread thread([&]() { timer.wait(); needBreak = true; });
  return uct(field, gen, numeric_limits<int>::max(), &needBreak);
}
