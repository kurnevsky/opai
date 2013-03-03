#include "config.h"
#include "mtdf.h"
#include "minimax.h"
#include "field.h"
#include "trajectories.h"
#include <omp.h>
#include <algorithm>
#include <limits>

using namespace std;

Score mtdfAlphabeta(Field* field, vector<Pos>* moves, Depth depth, Trajectories* last, Score alpha, Score beta, int* emptyBoard, Pos* best)
{
	for (auto i = moves->begin(); i != moves->end(); i++)
	{
		auto curEstimate = alphabeta(field, depth - 1, *i, last, -beta, -alpha, emptyBoard);
		if (curEstimate > alpha)
		{
			alpha = curEstimate;
			*best = *i;
			if (alpha >= beta)
				break;
		}
	}
	return alpha;
}

// CurField - поле, на котором производится оценка.
// Depth - глубина оценки.
Pos mtdf(Field* field, Depth depth)
{
	// Главные траектории - свои и вражеские.
	Trajectories curTrajectories(field, NULL);
	vector<Pos> moves;
	Pos result;
	// Делаем что-то только когда глубина просчета положительная и колическтво возможных ходов на входе не равно 0.
	if (depth <= 0)
		return -1;
	// Получаем ходы из траекторий (которые имеет смысл рассматривать), и находим пересечение со входными возможными точками.
	curTrajectories.buildTrajectories(depth);
	moves.assign(curTrajectories.getPoints()->begin(), curTrajectories.getPoints()->end());
	// Если нет возможных ходов, входящих в траектории - выходим.
	if (moves.size() == 0)
		return -1;
	auto alpha = -curTrajectories.getMaxScore(nextPlayer(field->getPlayer()));
	auto beta = curTrajectories.getMaxScore(field->getPlayer());
	int* emptyBoard = new int[field->getLength()];
	fill_n(emptyBoard, field->getLength(), 0);
	do
	{
		auto center = (alpha + beta) / 2;
		auto curEstimate = mtdfAlphabeta(field, &moves, depth, &curTrajectories, center, center + 1, emptyBoard, &result);
		if (curEstimate > center)
			alpha = curEstimate;
		else
			beta = curEstimate;
	}
	while (beta - alpha > 1);
	delete emptyBoard;
	return result;
}
