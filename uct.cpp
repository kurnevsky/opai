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

using namespace std;
using namespace boost;

// Play random game and get result.
// field - field to play.
// gen - random number generator.
// possibleMoves - allowed positions of moves.
// Returns number of winner, or -1 if draw.
int playRandomGame(Field* field, mt19937* gen, vector<int>* possibleMoves, int* moves, int komi)
{
  int redKomi;
  if (field->getPlayer() == playerRed)
    redKomi = komi;
  else
    redKomi = -komi;
  int putted = 0, result;
  moves[0] = (*possibleMoves)[0];
  int size = static_cast<int>(possibleMoves->size());
  for (int i = 1; i < size; i++)
  {
    uniform_int_distribution<int> dist(0, i);
    int j = dist(*gen);
    moves[i] = moves[j];
    moves[j] = (*possibleMoves)[i];
  }
  for (int i = 0; i < size; i++)
  {
    int pos = moves[i];
    if (field->isPuttingAllowed(pos) && !field->isInEmptyBase(pos))
    {
      field->doUnsafeStep(pos);
      putted++;
    }
  }
  if (field->getScore(playerRed) > redKomi)
    result = playerRed;
  else if (field->getScore(playerBlack) > -redKomi)
    result = playerBlack;
  else
    result = -1;
  for (int i = 0; i < putted; i++)
    field->undoStep();
  return result;
}

void finalUctSiblings(UctNode* n)
{
  if (n->sibling != nullptr)
    finalUctSiblings(n->sibling);
  delete n;
}

// Create children of UCT node.
// field - field for creating children.
// possibleMoves - allowed positions of moves.
// node - UCT node for creating children.
void createChildren(Field* field, vector<int>* possibleMoves, UctNode* node)
{
  UctNode* children = nullptr;
  UctNode* null = nullptr;
  UctNode** curChild = &children;
  for (auto i = possibleMoves->begin(); i < possibleMoves->end(); i++)
    if (field->isPuttingAllowed(*i))
    {
      *curChild = new UctNode();
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
double ucb(UctNode* parent, UctNode* node)
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
UctNode* uctSelect(mt19937* gen, UctNode* node)
{
  double bestUct = 0, uctValue;
  UctNode* result = nullptr;
  UctNode* next = node->child.load(std::memory_order_relaxed);
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
int playSimulation(Field* field, mt19937* gen, vector<int>* possibleMoves, int* moves, UctNode* node, int depth, int komi)
{
  int randomResult;
  if (node->visits.load(std::memory_order_relaxed) < UCT_WHEN_CREATE_CHILDREN || depth == UCT_DEPTH)
  {
    randomResult = playRandomGame(field, gen, possibleMoves, moves, komi);
  }
  else
  {
    if (node->child.load(std::memory_order_relaxed) == nullptr)
      createChildren(field, possibleMoves, node);
    UctNode* next = uctSelect(gen, node);
    if (next == nullptr)
    {
      int redKomi;
      if (field->getPlayer() == playerRed)
        redKomi = komi;
      else
        redKomi = -komi;
      if (field->getScore(playerRed) > redKomi)
        randomResult = playerRed;
      else if (field->getScore(playerBlack) > -redKomi)
        randomResult = playerBlack;
      else
        randomResult = -1;
    }
    else
    {
      field->doUnsafeStep(next->move);
      if (field->getDeltaScore() < 0)
      {
        field->undoStep();
        next->visits.store(numeric_limits<int>::max(), std::memory_order_relaxed);
        return playSimulation(field, gen, possibleMoves, moves, node, depth, komi);
      }
      randomResult = playSimulation(field, gen, possibleMoves, moves, next, depth + 1, -komi);
      field->undoStep();
    }
  }
  node->visits.fetch_add(1, std::memory_order_relaxed);
  if (randomResult == nextPlayer(field->getPlayer()))
    node->wins.fetch_add(1, std::memory_order_relaxed);
  else if (randomResult == -1)
    node->draws.fetch_add(1, std::memory_order_relaxed);
  return randomResult;
}

void playSimulation(Field* field, mt19937* gen, UctRoot* root, int* moves, int& ratched)
{
  playSimulation(field, gen, &root->moves, moves, root->node, 0, root->komi);
#if DYNAMIC_KOMI == 1
  int visits = root->node->visits.load(std::memory_order_relaxed);
  double winRate = 1 - (root->node->wins.load(std::memory_order_relaxed) + root->node->draws.load(std::memory_order_relaxed) * UCT_DRAW_WEIGHT) / visits;
  if ((winRate < UCT_RED || (winRate > UCT_GREEN && root->komi < ratched)) && visits - root->komiIter > root->komiIter / UCT_KOMI_INTERVAL && visits > UCT_KOMI_MIN_ITERATIONS)
  {
    #pragma omp critical
    {
      if (visits - root->komiIter > root->komiIter / UCT_KOMI_INTERVAL)
      {
        root->komiIter = visits;
        if (winRate < UCT_RED)
        {
          if (root->komi > 0)
            ratched = root->komi;
          root->komi--;
        }
        else
        {
          root->komi++;
        }
      }
    }
  }
#endif
}

// Delete UCT tree.
// n - start UCT node.
void finalUctNode(UctNode* n)
{
  UctNode* child = n->child.load(std::memory_order_relaxed);
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
int uct(UctRoot* root, Field* field, mt19937_64* gen, int maxSimulations, bool* needBreak)
{
  int ratched = numeric_limits<int>::max();
  #pragma omp parallel
  {
    Field* localField = new Field(*field);
    int* moves = new int[root->moves.size()];
    uniform_int_distribution<int> localDist(numeric_limits<int>::min(), numeric_limits<int>::max());
    mt19937* localGen;
    #pragma omp critical
    localGen = new mt19937(localDist(*gen));
    if (maxSimulations == numeric_limits<int>::max())
    {
      while (!*needBreak)
        playSimulation(localField, localGen, root, moves, ratched);
    }
    else
    {
      #pragma omp for
      for (int i = 0; i < maxSimulations; i++)
        playSimulation(localField, localGen, root, moves, ratched);
    }
    delete localGen;
    delete[] moves;
    delete localField;
  }
  double bestUct = 0;
  int result = -1;
  UctNode* next = root->node->child.load(std::memory_order_relaxed);
  while (next != nullptr)
  {
    if (next->visits != 0)
    {
      double uctValue = ucb(root->node, next);
      if (uctValue > bestUct)
      {
        bestUct = uctValue;
        result = next->move;
      }
    }
    next = next->sibling;
  }
  return result;
}

// Get best move by UCT analysis.
// field - field to find best move.
// gen - random number generator.
// maxSimulations - number of UCT simulations.
// Returns position of best move, or -1 if not found.
int uct(UctRoot* root, Field* field, mt19937_64* gen, int maxSimulations)
{
  bool needBreak = false;
  return uct(root, field, gen, maxSimulations, &needBreak);
}

// Get best move by UCT analysis.
// field - field to find best move.
// gen - random number generator.
// time - number of milliseconds to thinking.
// Returns position of best move, or -1 if not found.
int uctWithTime(UctRoot* root, Field* field, mt19937_64* gen, int time)
{
  bool needBreak = false;
  asio::io_service io;
  asio::deadline_timer timer(io, posix_time::milliseconds(time));
  thread thread([&]() { timer.wait(); needBreak = true; });
  return uct(root, field, gen, numeric_limits<int>::max(), &needBreak);
}

void clearUct(UctRoot* root, int length)
{
  if (root->node != nullptr)
  {
    finalUctNode(root->node);
    root->node = nullptr;
  }
  root->moves.clear();
  fill_n(root->movesField, length, false);
  root->player = -1;
  root->komi = 0;
  root->komiIter = 0;
}

void initUct(Field* field, UctRoot* root)
{
  root->node = new UctNode;
  root->player = field->getPlayer();
  root->komi = field->getScore(root->player);
  const vector<int>& pointsSeq = field->getPointsSeq();
  root->pointsSeq.assign(pointsSeq.begin(), pointsSeq.end());
  for (auto it = pointsSeq.begin(); it != pointsSeq.end(); it++)
  {
    int startPos = *it;
    field->wave(startPos, [&, startPos, field, root](int pos)->bool
    {
      if (field->manhattan(startPos, pos) <= UCT_RADIUS)
      {
        if (!root->movesField[pos] && field->isPuttingAllowed(pos))
        {
          root->movesField[pos] = true;
          root->moves.push_back(pos);
        }
        return true;
      }
      else
      {
        return false;
      }
    });
  }
}

UctRoot* initUct(Field* field)
{
  UctRoot* root = new UctRoot(field->getLength());
  initUct(field, root);
  return root;
}

void finalUctNodeExcept(UctNode* n, UctNode* except)
{
  if (n == except)
  {
    if (n->sibling != nullptr)
      finalUctNode(n->sibling);
    n->sibling = nullptr;
  }
  else
  {
    UctNode* child = n->child.load(std::memory_order_relaxed);
    if (child != nullptr)
      finalUctNode(child);
    if (n->sibling != nullptr)
      finalUctNodeExcept(n->sibling, except);
    delete n;
  }
}

void expandUctNode(UctNode* n, vector<int>* moves)
{
  UctNode* next = n->child.load(std::memory_order_relaxed);
  if (next == nullptr)
  {
    if (n->visits.load(std::memory_order_relaxed) == numeric_limits<int>::max())
    {
      n->wins.store(0, std::memory_order_relaxed);
      n->draws.store(0, std::memory_order_relaxed);
      n->visits.store(0, std::memory_order_relaxed);
    }
    return;
  }
  while (next->sibling != nullptr)
  {
    expandUctNode(next, moves);
    next = next->sibling;
  }
  expandUctNode(next, moves);
  for (auto it = moves->begin(); it != moves->end(); it++)
  {
    next->sibling = new UctNode();
    next->sibling->move = *it;
    next = next->sibling;
  }
}

bool updateUctStep(Field* field, UctRoot* root)
{
  const vector<int>& pointsSeq = field->getPointsSeq();
  int nextPos = pointsSeq[root->pointsSeq.size()];
  if (field->getPlayer(nextPos) != root->player)
  {
    clearUct(root, field->getLength());
    initUct(field, root);
    return false;
  }
  UctNode* next = root->node->child.load(std::memory_order_relaxed);
  while (next != nullptr && next->move != nextPos)
  {
    next = next->sibling;
  }
  if (next == nullptr)
  {
    clearUct(root, field->getLength());
    initUct(field, root);
    return false;
  }
  finalUctNodeExcept(root->node->child.load(std::memory_order_relaxed), next);
  delete root->node;
  root->node = next;
  for (auto it = root->moves.begin(); it != root->moves.end();)
  {
    if (field->isPuttingAllowed(*it))
    {
      it++;
    }
    else
    {
      root->movesField[*it] = false;
      root->moves.erase(it);
    }
  }
  vector<int> addedMoves;
  field->wave(nextPos, [&, nextPos, field, root](int pos)->bool
  {
    if (field->manhattan(nextPos, pos) <= UCT_RADIUS)
    {
      if (!root->movesField[pos] && field->isPuttingAllowed(pos))
      {
        root->movesField[pos] = true;
        root->moves.push_back(pos);
        addedMoves.push_back(pos);
      }
      return true;
    }
    else
    {
      return false;
    }
  });
  if (!addedMoves.empty())
    expandUctNode(next, &addedMoves);
  root->pointsSeq.push_back(nextPos);
  root->player = nextPlayer(root->player);
  root->komi = -root->komi;
  root->komiIter = next->visits.load(std::memory_order_relaxed);
  return root->pointsSeq.size() < pointsSeq.size();
}

void updateUct(Field* field, UctRoot* root)
{
  int movesCount = field->getMovesCount();
  int uctMovesCount = static_cast<int>(root->pointsSeq.size());
  const vector<int>& pointsSeq = field->getPointsSeq();
  if (movesCount < uctMovesCount || !equal(root->pointsSeq.begin(), root->pointsSeq.end(), pointsSeq.begin()))
  {
    clearUct(root, field->getLength());
    initUct(field, root);
  }
  else if (movesCount == uctMovesCount)
  {
    if (field->getPlayer() != root->player)
    {
      clearUct(root, field->getLength());
      initUct(field, root);
    }
  }
  else
  {
    while (updateUctStep(field, root));
  }
}

void finalUct(UctRoot* root)
{
  if (root->node != nullptr)
    finalUctNode(root->node);
  delete root;
}
