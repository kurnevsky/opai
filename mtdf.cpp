#include "config.h"
#include "mtdf.h"
#include "minimax.h"
#include "field.h"
#include "trajectories.h"
#include <omp.h>
#include <algorithm>
#include <limits>

using namespace std;

Score mtdfAlphabeta(Field** fields, vector<Pos>* moves, Depth depth, Trajectories* last, Score alpha, Score beta, int** emptyBoards, Pos* best)
{
	#pragma omp parallel
	{
		auto threadNum = omp_get_thread_num();
		#pragma omp for schedule(dynamic, 1)
		for (auto i = moves->begin(); i < moves->end(); i++)
		{
			if (alpha < beta)
			{
				auto curEstimate = alphabeta(fields[threadNum], depth - 1, *i, last, -beta, -alpha, emptyBoards[threadNum]);
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
Pos mtdf(Field* field, Depth depth)
{
	int* emptyBoard = new int[field->getLength()];
	fill_n(emptyBoard, field->getLength(), 0);
	// Главные траектории - свои и вражеские.
	Trajectories curTrajectories(field, emptyBoard);
	vector<Pos> moves;
	Pos result;
	// Делаем что-то только когда глубина просчета положительная и колическтво возможных ходов на входе не равно 0.
	if (depth <= 0)
	{
		delete emptyBoard;
		return -1;
	}
	// Получаем ходы из траекторий (которые имеет смысл рассматривать), и находим пересечение со входными возможными точками.
	curTrajectories.buildTrajectories(depth);
	moves.assign(curTrajectories.getPoints()->begin(), curTrajectories.getPoints()->end());
	// Если нет возможных ходов, входящих в траектории - выходим.
	if (moves.size() == 0)
	{
		delete emptyBoard;
		return -1;
	}
	auto alpha = -curTrajectories.getMaxScore(nextPlayer(field->getPlayer()));
	auto beta = curTrajectories.getMaxScore(field->getPlayer());
	auto maxThreads = omp_get_max_threads();
	int** emptyBoards = new int*[maxThreads];
	emptyBoards[0] = emptyBoard;
	for (auto i = 1; i < maxThreads; i++)
	{
		emptyBoards[i] = new int[field->getLength()];
		fill_n(emptyBoards[i], field->getLength(), 0);
	}
	Field** fields = new Field*[maxThreads];
	fields[0] = field;
	for (auto i = 1; i < maxThreads; i++)
		fields[i] = new Field(*field);
	do
	{
		auto center = (alpha + beta) / 2;
		auto curEstimate = mtdfAlphabeta(fields, &moves, depth, &curTrajectories, center, center + 1, emptyBoards, &result);
		if (curEstimate > center)
			alpha = curEstimate;
		else
			beta = curEstimate;
	}
	while (alpha != beta);//(beta - alpha > 1);
	result = alpha == getEnemyEstimate(fields, emptyBoards, maxThreads, &curTrajectories, depth - 1) ? -1 : result;
	for (auto i = 0; i < maxThreads; i++)
		delete emptyBoards[i];
	delete emptyBoards;
	for (auto i = 1; i < maxThreads; i++)
		delete fields[i];
	delete fields;
	return result;
}
