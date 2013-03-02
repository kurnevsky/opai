#include "config.h"
#include "mtdf.h"
#include "minimax.h"
#include "field.h"
#include "trajectories.h"
#include <omp.h>
#include <algorithm>
#include <limits>

using namespace std;

Score mtdf_alphabeta(Field* field, vector<Pos>* moves, size_t depth, Trajectories* last, Score alpha, Score beta, int* empty_board, Pos* best)
{
	for (auto i = moves->begin(); i != moves->end(); i++)
	{
		Score cur_estimate = alphabeta(field, depth - 1, *i, last, -beta, -alpha, empty_board);
		if (cur_estimate > alpha)
		{
			alpha = cur_estimate;
			*best = *i;
			if (alpha >= beta)
				break;
		}
	}

	return alpha;
}

// CurField - поле, на котором производится оценка.
// Depth - глубина оценки.
Pos mtdf(Field* field, size_t depth)
{
	// Главные траектории - свои и вражеские.
	Trajectories cur_trajectories(field, NULL, depth);
	vector<Pos> moves;
	Pos result;

	// Делаем что-то только когда глубина просчета положительная и колическтво возможных ходов на входе не равно 0.
	if (depth <= 0)
		return -1;

	// Получаем ходы из траекторий (которые имеет смысл рассматривать), и находим пересечение со входными возможными точками.
	cur_trajectories.build_trajectories();
	moves.assign(cur_trajectories.get_points()->begin(), cur_trajectories.get_points()->end());
	// Если нет возможных ходов, входящих в траектории - выходим.
	if (moves.size() == 0)
		return -1;

	Score alpha = -cur_trajectories.get_max_score(next_player(field->get_player()));
	Score beta = cur_trajectories.get_max_score(field->get_player());

	int* empty_board = new int[field->getLength()];

	do
	{
		int center = (alpha + beta) / 2;
		Score cur_estimate = mtdf_alphabeta(field, &moves, depth, &cur_trajectories, center, center + 1, empty_board, &result);
		if (cur_estimate > center)
			alpha = cur_estimate;
		else
			beta = cur_estimate;
	}
	while (beta - alpha > 1);

	delete empty_board;

	return result;
}
