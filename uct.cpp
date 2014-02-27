#include "config.h"
#include "basic_types.h"
#include "uct.h"
#include "player.h"
#include "field.h"
#include "time.h"
#include <limits>
#include <queue>
#include <vector>
#include <algorithm>
#include <omp.h>

using namespace std;

Player playRandomGame(Field* field, mt* gen, vector<Pos>* possibleMoves)
{
  vector<Pos> moves(possibleMoves->size());
  auto putted = 0;
  Player result;
  moves[0] = (*possibleMoves)[0];
  for (size_t i = 1; i < possibleMoves->size(); i++)
  {
    uniform_int_distribution<size_t> dist(0, i);
    auto j = dist(*gen);
    moves[i] = moves[j];
    moves[j] = (*possibleMoves)[i];
  }
  for (auto i = moves.begin(); i < moves.end(); i++)
    if (field->puttingAllow(*i))
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
  for (auto i = 0; i < putted; i++)
    field->undoStep();
  return result;
}

void createChildren(Field* field, vector<Pos>* possibleMoves, uctNode* n)
{
  uctNode** curChild = &n->child;
  for (auto i = possibleMoves->begin(); i < possibleMoves->end(); i++)
    if (field->puttingAllow(*i))
    {
      *curChild = new uctNode();
      (*curChild)->move = *i;
      curChild = &(*curChild)->sibling;
    }
}

uctNode* uctSelect(mt* gen, uctNode* n)
{
  double bestUct = 0, uctValue;
  uctNode* result = NULL;
  uctNode* next = n->child;
  while (next != NULL)
  {
    if (next->visits > 0)
    {
      auto winRate = static_cast<double>(next->wins)/next->visits;
      auto uct = UCTK * sqrt(2 * log(static_cast<double>(n->visits)) / next->visits);
      uctValue = winRate + uct;
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

Player playSimulation(Field* field, mt* gen, vector<Pos>* possibleMoves, uctNode* n, Depth depth)
{
  Player randomResult;
  if (n->visits == 0 || depth == UCT_DEPTH)
  {
    randomResult = playRandomGame(field, gen, possibleMoves);
  }
  else
  {
    if (n->child == NULL)
      createChildren(field, possibleMoves, n);
    uctNode* next = uctSelect(gen, n);
    if (next == NULL)
    {
      n->visits = numeric_limits<Iterations>::max();
      if (field->getScore(nextPlayer(field->getPlayer())) > 0)
        n->wins = numeric_limits<Iterations>::max();
      if (field->getScore(playerRed) > 0)
        return playerRed;
      else if (field->getScore(playerBlack) > 0)
        return playerBlack;
      else
        return -1;
    }
    field->doUnsafeStep(next->move);
    randomResult = playSimulation(field, gen, possibleMoves, next, depth + 1);
    field->undoStep();
  }
  n->visits++;
  if (randomResult == nextPlayer(field->getPlayer()))
    n->wins++;
  return randomResult;
}

template<typename _Cont>
void generatePossibleMoves(Field* field, _Cont* possibleMoves)
{
  int* rField = new int[field->getLength()];
  fill_n(rField, field->getLength(), 0);
  queue<Pos> q;
  possibleMoves->clear();
  for (auto i = field->minPos(); i <= field->maxPos(); i++)
    if (field->isPutted(i)) //TODO: Класть соседей, а не сами точки.
      q.push(i);
  while (!q.empty())
  {
    if (field->puttingAllow(q.front())) //TODO: Убрать условие.
      possibleMoves->push_back(q.front());
    if (rField[q.front()] < UCT_RADIUS)
    {
      if (field->puttingAllow(field->n(q.front())) && rField[field->n(q.front())] == 0)
      {
        rField[field->n(q.front())] = rField[q.front()] + 1;
        q.push(field->n(q.front()));
      }
      if (field->puttingAllow(field->s(q.front())) && rField[field->s(q.front())] == 0)
      {
        rField[field->s(q.front())] = rField[q.front()] + 1;
        q.push(field->s(q.front()));
      }
      if (field->puttingAllow(field->w(q.front())) && rField[field->w(q.front())] == 0)
      {
        rField[field->w(q.front())] = rField[q.front()] + 1;
        q.push(field->w(q.front()));
      }
      if (field->puttingAllow(field->e(q.front())) && rField[field->e(q.front())] == 0)
      {
        rField[field->e(q.front())] = rField[q.front()] + 1;
        q.push(field->e(q.front()));
      }
    }
    q.pop();
  }
  delete rField;
}

void finalUct(uctNode* n)
{
  if (n->child != NULL)
    finalUct(n->child);
  if (n->sibling != NULL)
    finalUct(n->sibling);
  delete n;
}

Pos uct(Field* field, mt* gen, Iterations maxSimulations)
{
  // Список всех возможных ходов для UCT.
  vector<Pos> moves;
  double bestUct = 0;
  Pos result = -1;
  generatePossibleMoves(field, &moves);
  if (static_cast<size_t>(omp_get_max_threads()) > moves.size())
    omp_set_num_threads(moves.size());
  #pragma omp parallel
  {
    uctNode n;
    Field* localField = new Field(*field);
    uniform_int_distribution<size_t> localDist(numeric_limits<size_t>::min(), numeric_limits<size_t>::max());
    mt* localGen;
    #pragma omp critical
    localGen = new mt(localDist(*gen));
    uctNode** curChild = &n.child;
    for (auto i = moves.begin() + omp_get_thread_num(); i < moves.end(); i += omp_get_num_threads())
    {
      *curChild = new uctNode();
      (*curChild)->move = *i;
      curChild = &(*curChild)->sibling;
    }
    #pragma omp for
    for (Iterations i = 0; i < maxSimulations; i++)
      playSimulation(localField, localGen, &moves, &n, 0);
    #pragma omp critical
    {
      uctNode* next = n.child;
      while (next != NULL)
      {
        if (next->visits != 0)
        {
          auto winRate = static_cast<double>(next->wins)/next->visits;
          auto uct = UCTK * sqrt(2 * log(static_cast<double>(n.visits)) / next->visits);
          auto uctValue = winRate + uct;
          if (uctValue > bestUct)
          {
            bestUct = uctValue;
            result = next->move;
          }
        }
        next = next->sibling;
      }
    }

    if (n.child != NULL)
      finalUct(n.child);
    delete localGen;
    delete localField;
  }
  return result;
}

Pos uctWithTime(Field* field, mt* gen, Time time)
{
  // Список всех возможных ходов для UCT.
  vector<Pos> moves;
  double bestUct = 0;
  Pos result = -1;
  Timer t;
  generatePossibleMoves(field, &moves);
  if (static_cast<size_t>(omp_get_max_threads()) > moves.size())
    omp_set_num_threads(moves.size());
  #pragma omp parallel
  {
    uctNode n;
    Field* localField = new Field(*field);
    uniform_int_distribution<size_t> localDist(numeric_limits<size_t>::min(), numeric_limits<size_t>::max());
    mt* localGen;
    #pragma omp critical
    localGen = new mt(localDist(*gen));
    uctNode** curChild = &n.child;
    for (auto i = moves.begin() + omp_get_thread_num(); i < moves.end(); i += omp_get_num_threads())
    {
      *curChild = new uctNode();
      (*curChild)->move = *i;
      curChild = &(*curChild)->sibling;
    }
    while (t.get() < time)
      for (Iterations i = 0; i < UCT_ITERATIONS_BEFORE_CHECK_TIME; i++)
        playSimulation(localField, localGen, &moves, &n, 0);
    #pragma omp critical
    {
      uctNode* next = n.child;
      while (next != NULL)
      {
        if (next->visits != 0)
        {
          auto winRate = static_cast<double>(next->wins)/next->visits;
          auto uct = UCTK * sqrt(2 * log(static_cast<double>(n.visits)) / next->visits);
          auto uctValue = winRate + uct;
          if (uctValue > bestUct)
          {
            bestUct = uctValue;
            result = next->move;
          }
        }
        next = next->sibling;
      }
    }
    if (n.child != NULL)
      finalUct(n.child);
    delete localGen;
    delete localField;
  }
  return result;
}
