#include "config.h"
#include "mtdf.h"
#include "minimax.h"
#include "field.h"
#include "trajectories.h"
#include <omp.h>
#include <algorithm>
#include <limits>

using namespace std;

int mtdfAlphabeta(Field** fields, vector<int>* moves, int depth, Trajectories* last, int alpha, int beta, int** emptyBoards, int* best)
{
  #pragma omp parallel
  {
    int threadNum = omp_get_thread_num();
    #pragma omp for schedule(dynamic, 1)
    for (auto i = moves->begin(); i < moves->end(); i++)
    {
      if (alpha < beta)
      {
        int curEstimate = alphabeta(fields[threadNum], depth - 1, *i, last, -beta, -alpha, emptyBoards[threadNum]);
        #pragma omp critical
        {
          if (curEstimate > alpha)
          {
            alpha = curEstimate;
            *best = *i;
          }
        }
      }
    }
  }
  return alpha;
}

// CurField - поле, на котором производится оценка.
// Depth - глубина оценки.
int mtdf(Field* field, int depth)
{
  if (depth <= 0)
    return -1;
  int* emptyBoard = new int[field->getLength()];
  fill_n(emptyBoard, field->getLength(), 0);
  // Главные траектории - свои и вражеские.
  Trajectories curTrajectories(field, emptyBoard);
  vector<int> moves;
  int result;
  // Получаем ходы из траекторий (которые имеет смысл рассматривать), и находим пересечение со входными возможными точками.
  curTrajectories.buildTrajectories(depth);
  moves.assign(curTrajectories.getPoints()->begin(), curTrajectories.getPoints()->end());
  // Если нет возможных ходов, входящих в траектории - выходим.
  if (moves.size() == 0)
  {
    delete[] emptyBoard;
    return -1;
  }
  int alpha = -curTrajectories.getMaxScore(nextPlayer(field->getPlayer()));
  int beta = curTrajectories.getMaxScore(field->getPlayer());
  int maxThreads = omp_get_max_threads();
  int** emptyBoards = new int*[maxThreads];
  emptyBoards[0] = emptyBoard;
  for (int i = 1; i < maxThreads; i++)
  {
    emptyBoards[i] = new int[field->getLength()];
    fill_n(emptyBoards[i], field->getLength(), 0);
  }
  Field** fields = new Field*[maxThreads];
  fields[0] = field;
  for (int i = 1; i < maxThreads; i++)
    fields[i] = new Field(*field);
  do
  {
    int center = (alpha + beta) / 2;
    if ((alpha + beta) % 2 == -1)
      center--;
    int curEstimate = mtdfAlphabeta(fields, &moves, depth, &curTrajectories, center, center + 1, emptyBoards, &result);
    if (curEstimate > center)
      alpha = curEstimate;
    else
      beta = curEstimate;
  }
  while (alpha != beta);//(beta - alpha > 1);
  result = alpha == getEnemyEstimate(fields, emptyBoards, maxThreads, &curTrajectories, depth - 1) ? -1 : result;
  for (int i = 0; i < maxThreads; i++)
    delete[] emptyBoards[i];
  delete[] emptyBoards;
  for (int i = 1; i < maxThreads; i++)
    delete fields[i];
  delete[] fields;
  return result;
}
