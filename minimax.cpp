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
Score alphabeta(Field* field, size_t depth, Pos pos, Trajectories* last, Score alpha, Score beta, int* empty_board)
{
	Trajectories cur_trajectories(field, empty_board);

	// Делаем ход, выбранный на предыдущем уровне рекурсии, после чего этот ход становится вражеским.
	field->doUnsafeStep(pos);

	if (depth == 0)
	{
		auto best_estimate = field->getScore(field->getPlayer());
		field->undoStep();
		return -best_estimate;
	}

	if (field->getDScore() < 0) // Если точка поставлена в окружение.
	{
		field->undoStep();
		return -SCORE_INFINITY; // Для CurPlayer это хорошо, то есть оценка Infinity.
	}

	cur_trajectories.build_trajectories(last, pos);

	list<Pos>* moves = cur_trajectories.get_points();

	if (moves->size() == 0)
	{
		auto best_estimate = field->getScore(field->getPlayer());
		field->undoStep();
		return -best_estimate;
	}

	for (auto i = moves->begin(); i != moves->end(); i++)
	{
		Score cur_estimate = alphabeta(field, depth - 1, *i, &cur_trajectories, -alpha - 1, -alpha, empty_board);
		if (cur_estimate > alpha && cur_estimate < beta)
			cur_estimate = alphabeta(field, depth - 1, *i, &cur_trajectories, -beta, -cur_estimate, empty_board);
		if (cur_estimate > alpha)
		{
			alpha = cur_estimate;
			if (alpha >= beta)
				break;
		}
	}

	field->undoStep();
	return -alpha;
}

Score get_enemy_estimate(Field* field, Trajectories* last, size_t depth)
{
	Trajectories cur_trajectories(field);
	Score result;
	vector<Pos> moves;

	field->setNextPlayer();
	cur_trajectories.build_trajectories(last);

	moves.assign(cur_trajectories.get_points()->begin(), cur_trajectories.get_points()->end());
	if (moves.size() == 0)
	{
		result = field->getScore(field->getPlayer());
	}
	else
	{
		auto alpha = -cur_trajectories.get_max_score(nextPlayer(field->getPlayer()));
		auto beta = cur_trajectories.get_max_score(field->getPlayer());
		#pragma omp parallel
		{
			Field* local_field = new Field(*field);
			int* empty_board = new int[field->getLength()];

			#pragma omp for schedule(dynamic, 1)
			for (auto i = moves.begin(); i < moves.end(); i++)
			{
				if (alpha < beta)
				{
					Score cur_estimate = alphabeta(local_field, depth - 1, *i, &cur_trajectories, -alpha - 1, -alpha, empty_board);
					if (cur_estimate > alpha && cur_estimate < beta)
						cur_estimate = alphabeta(local_field, depth - 1, *i, &cur_trajectories, -beta, -cur_estimate, empty_board);
					#pragma omp critical
					{
						if (cur_estimate > alpha) // Обновляем нижнюю границу.
							alpha = cur_estimate;
					}
				}
			}

			delete empty_board;
			delete local_field;
		}
		result = alpha;
	}

	field->setNextPlayer();
	return -result;
}

// CurField - поле, на котором производится оценка.
// Depth - глубина оценки.
// Moves - на входе возможные ходы, на выходе лучшие из них.
Pos minimax(Field* field, size_t depth)
{
	// Главные траектории - свои и вражеские.
	Trajectories cur_trajectories(field, NULL, depth);
	Pos result;
	vector<Pos> moves;

	// Делаем что-то только когда глубина просчета положительная и колическтво возможных ходов на входе не равно 0.
	if (depth <= 0)
		return -1;

	// Получаем ходы из траекторий (которые имеет смысл рассматривать), и находим пересечение со входными возможными точками.
	cur_trajectories.build_trajectories();
	moves.assign(cur_trajectories.get_points()->begin(), cur_trajectories.get_points()->end());
	// Если нет возможных ходов, входящих в траектории - выходим.
	if (moves.size() == 0)
		return -1;
	// Для почти всех возможных точек, не входящих в траектории оценка будет такая же, как если бы игрок CurPlayer пропустил ход.
	//int enemy_estimate = get_enemy_estimate(cur_field, Trajectories[cur_field.get_player()], Trajectories[next_player(cur_field.get_player())], depth);

	auto alpha = -cur_trajectories.get_max_score(nextPlayer(field->getPlayer()));
	auto beta = cur_trajectories.get_max_score(field->getPlayer());
	#pragma omp parallel
	{
		Field* local_field = new Field(*field);
		int* empty_board = new int[field->getLength()];

		#pragma omp for schedule(dynamic, 1)
		for (auto i = moves.begin(); i < moves.end(); i++)
		{
			if (alpha < beta)
			{
				Score cur_estimate = alphabeta(local_field, depth - 1, *i, &cur_trajectories, -alpha - 1, -alpha, empty_board);
				if (cur_estimate > alpha && cur_estimate < beta)
					cur_estimate = alphabeta(local_field, depth - 1, *i, &cur_trajectories, -beta, -cur_estimate, empty_board);
				#pragma omp critical
				{
					if (cur_estimate > alpha) // Обновляем нижнюю границу.
					{
						alpha = cur_estimate;
						result = *i;
					}
				}
			}
		}

		delete empty_board;
		delete local_field;
	}
	return alpha == get_enemy_estimate(field, &cur_trajectories, depth - 1) ? -1 : result;
}
