#include "config.h"
#include "minimax.h"
#include "field.h"
#include "trajectories.h"
#include <omp.h>
#include <algorithm>
#include <limits>
#include <math.h>

using namespace std;

// Рекурсивная функция минимакса.
// CurField - поле, на котором ведется поиск лучшего хода.
// TrajectoriesBoard - доска, на которую проецируются траектории. Должна быть заполнена нулями. Нужна для оптимизации.
// Depth - глубина просчета.
// Pos - последний выбранный, но не сделанный ход.
// alpha, beta - интервал оценок, вне которого искать нет смысла.
// На выходе оценка позиции для CurPlayer (до хода Pos).
int alphabeta(Field* field, int depth, int pos, Trajectories* last, int alpha, int beta, int* emptyBoard)
{
  Trajectories curTrajectories(field, emptyBoard);
  // Делаем ход, выбранный на предыдущем уровне рекурсии, после чего этот ход становится вражеским.
  field->doUnsafeStep(pos);
  if (depth == 0)
  {
    int bestEstimate = field->getScore(field->getPlayer());
    field->undoStep();
    return -bestEstimate;
  }
  if (field->getDScore() < 0) // Если точка поставлена в окружение.
  {
    field->undoStep();
    return -numeric_limits<int>::max(); // Для CurPlayer это хорошо, то есть оценка Infinity.
  }
  curTrajectories.buildTrajectories(last, pos);
  list<int>* moves = curTrajectories.getPoints();
  if (moves->size() == 0)
  {
    int bestEstimate = field->getScore(field->getPlayer());
    field->undoStep();
    return -bestEstimate;
  }
  for (auto i = moves->begin(); i != moves->end(); i++)
  {
    int curEstimate = alphabeta(field, depth - 1, *i, &curTrajectories, -alpha - 1, -alpha, emptyBoard);
    if (curEstimate > alpha && curEstimate < beta)
      curEstimate = alphabeta(field, depth - 1, *i, &curTrajectories, -beta, -curEstimate, emptyBoard);
    if (curEstimate > alpha)
    {
      alpha = curEstimate;
      if (alpha >= beta)
        break;
    }
  }
  field->undoStep();
  return -alpha;
}

int getEnemyEstimate(Field** fields, int** emptyBoards, int maxThreads, Trajectories* last, int depth)
{
  Trajectories curTrajectories(fields[0], emptyBoards[0]);
  int result;
  vector<int> moves;
  for (int i = 0; i < maxThreads; i++)
    fields[i]->setNextPlayer();
  curTrajectories.buildTrajectories(last);
  moves.assign(curTrajectories.getPoints()->begin(), curTrajectories.getPoints()->end());
  if (moves.size() == 0)
  {
    result = fields[0]->getScore(fields[0]->getPlayer());
  }
  else
  {
    int alpha = -curTrajectories.getMaxScore(nextPlayer(fields[0]->getPlayer()));
    int beta = curTrajectories.getMaxScore(fields[0]->getPlayer());
    #pragma omp parallel
    {
      int threadNum = omp_get_thread_num();
      #pragma omp for schedule(dynamic, 1)
      for (auto i = moves.begin(); i < moves.end(); i++)
      {
        if (alpha < beta)
        {
          int curEstimate = alphabeta(fields[threadNum], depth - 1, *i, &curTrajectories, -alpha - 1, -alpha, emptyBoards[threadNum]);
          if (curEstimate > alpha && curEstimate < beta)
            curEstimate = alphabeta(fields[threadNum], depth - 1, *i, &curTrajectories, -beta, -curEstimate, emptyBoards[threadNum]);
          #pragma omp critical
          {
            if (curEstimate > alpha) // Обновляем нижнюю границу.
              alpha = curEstimate;
          }
        }
      }
    }
    result = alpha;
  }
  for (int i = 0; i < maxThreads; i++)
    fields[i]->setNextPlayer();
  return -result;
}

// CurField - поле, на котором производится оценка.
// Depth - глубина оценки.
// Moves - на входе возможные ходы, на выходе лучшие из них.
int minimax(Field* field, int depth)
{
  if (depth <= 0)
    return -1;
  int* emptyBoard = new int[field->getLength()];
  fill_n(emptyBoard, field->getLength(), 0);
  // Главные траектории - свои и вражеские.
  Trajectories curTrajectories(field, emptyBoard);
  int result;
  vector<int> moves;
  // Получаем ходы из траекторий (которые имеет смысл рассматривать), и находим пересечение со входными возможными точками.
  curTrajectories.buildTrajectories(depth);
  moves.assign(curTrajectories.getPoints()->begin(), curTrajectories.getPoints()->end());
  // Если нет возможных ходов, входящих в траектории - выходим.
  if (moves.size() == 0)
  {
    delete emptyBoard;
    return -1;
  }
  // Для почти всех возможных точек, не входящих в траектории оценка будет такая же, как если бы игрок CurPlayer пропустил ход.
  //int enemy_estimate = get_enemy_estimate(cur_field, Trajectories[cur_field.get_player()], Trajectories[next_player(cur_field.get_player())], depth);
  int maxThreads = omp_get_max_threads();
  int** emptyBoards = new int*[maxThreads];
  emptyBoards[0] = emptyBoard;
  for (auto i = 1; i < maxThreads; i++)
  {
    emptyBoards[i] = new int[field->getLength()];
    fill_n(emptyBoards[i], field->getLength(), 0);
  }
  Field** fields = new Field*[maxThreads];
  fields[0] = field;
  for (int i = 1; i < maxThreads; i++)
    fields[i] = new Field(*field);
  int alpha = -curTrajectories.getMaxScore(nextPlayer(field->getPlayer()));
  int beta = curTrajectories.getMaxScore(field->getPlayer());
  #pragma omp parallel
  {
    int threadNum = omp_get_thread_num();
    #pragma omp for schedule(dynamic, 1)
    for (auto i = moves.begin(); i < moves.end(); i++)
    {
      if (alpha < beta)
      {
        int curEstimate = alphabeta(fields[threadNum], depth - 1, *i, &curTrajectories, -alpha - 1, -alpha, emptyBoards[threadNum]);
        if (curEstimate > alpha && curEstimate < beta)
          curEstimate = alphabeta(fields[threadNum], depth - 1, *i, &curTrajectories, -beta, -curEstimate, emptyBoards[threadNum]);
        #pragma omp critical
        {
          if (curEstimate > alpha) // Обновляем нижнюю границу.
          {
            alpha = curEstimate;
            result = *i;
          }
        }
      }
    }
  }
  result = alpha == getEnemyEstimate(fields, emptyBoards, maxThreads, &curTrajectories, depth - 1) ? -1 : result;
  for (int i = 0; i < maxThreads; i++)
    delete emptyBoards[i];
  delete emptyBoards;
  for (int i = 1; i < maxThreads; i++)
    delete fields[i];
  delete fields;
  return result;
}
