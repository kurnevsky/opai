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
Score alphabeta(Field* field, Depth depth, Pos pos, Trajectories* last, Score alpha, Score beta, int* emptyBoard)
{
	Trajectories curTrajectories(field, emptyBoard);

	// Делаем ход, выбранный на предыдущем уровне рекурсии, после чего этот ход становится вражеским.
	field->doUnsafeStep(pos);

	if (depth == 0)
	{
		auto bestEstimate = field->getScore(field->getPlayer());
		field->undoStep();
		return -bestEstimate;
	}

	if (field->getDScore() < 0) // Если точка поставлена в окружение.
	{
		field->undoStep();
		return -SCORE_INFINITY; // Для CurPlayer это хорошо, то есть оценка Infinity.
	}

	curTrajectories.buildTrajectories(last, pos);

	list<Pos>* moves = curTrajectories.getPoints();

	if (moves->size() == 0)
	{
		auto bestEstimate = field->getScore(field->getPlayer());
		field->undoStep();
		return -bestEstimate;
	}

	for (auto i = moves->begin(); i != moves->end(); i++)
	{
		auto curEstimate = alphabeta(field, depth - 1, *i, &curTrajectories, -alpha - 1, -alpha, emptyBoard);
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

Score get_enemy_estimate(Field* field, Trajectories* last, Depth depth)
{
	Trajectories curTrajectories(field, NULL);
	Score result;
	vector<Pos> moves;

	field->setNextPlayer();
	curTrajectories.buildTrajectories(last);

	moves.assign(curTrajectories.getPoints()->begin(), curTrajectories.getPoints()->end());
	if (moves.size() == 0)
	{
		result = field->getScore(field->getPlayer());
	}
	else
	{
		auto alpha = -curTrajectories.getMaxScore(nextPlayer(field->getPlayer()));
		auto beta = curTrajectories.getMaxScore(field->getPlayer());
		#pragma omp parallel
		{
			Field* localField = new Field(*field);
			int* emptyBoard = new int[field->getLength()];
			fill_n(emptyBoard, field->getLength(), 0);

			#pragma omp for schedule(dynamic, 1)
			for (auto i = moves.begin(); i < moves.end(); i++)
			{
				if (alpha < beta)
				{
					auto curEstimate = alphabeta(localField, depth - 1, *i, &curTrajectories, -alpha - 1, -alpha, emptyBoard);
					if (curEstimate > alpha && curEstimate < beta)
						curEstimate = alphabeta(localField, depth - 1, *i, &curTrajectories, -beta, -curEstimate, emptyBoard);
					#pragma omp critical
					{
						if (curEstimate > alpha) // Обновляем нижнюю границу.
							alpha = curEstimate;
					}
				}
			}

			delete emptyBoard;
			delete localField;
		}
		result = alpha;
	}

	field->setNextPlayer();
	return -result;
}

// CurField - поле, на котором производится оценка.
// Depth - глубина оценки.
// Moves - на входе возможные ходы, на выходе лучшие из них.
Pos minimax(Field* field, Depth depth)
{
	// Главные траектории - свои и вражеские.
	Trajectories curTrajectories(field, NULL);
	Pos result;
	vector<Pos> moves;
	// Делаем что-то только когда глубина просчета положительная и колическтво возможных ходов на входе не равно 0.
	if (depth <= 0)
		return -1;
	// Получаем ходы из траекторий (которые имеет смысл рассматривать), и находим пересечение со входными возможными точками.
	curTrajectories.buildTrajectories(depth);
	moves.assign(curTrajectories.getPoints()->begin(), curTrajectories.getPoints()->end());
	// Если нет возможных ходов, входящих в траектории - выходим.
	if (moves.size() == 0)
		return -1;
	// Для почти всех возможных точек, не входящих в траектории оценка будет такая же, как если бы игрок CurPlayer пропустил ход.
	//int enemy_estimate = get_enemy_estimate(cur_field, Trajectories[cur_field.get_player()], Trajectories[next_player(cur_field.get_player())], depth);
	auto alpha = -curTrajectories.getMaxScore(nextPlayer(field->getPlayer()));
	auto beta = curTrajectories.getMaxScore(field->getPlayer());
	#pragma omp parallel
	{
		Field* localField = new Field(*field);
		int* emptyBoard = new int[field->getLength()];
		fill_n(emptyBoard, field->getLength(), 0);

		#pragma omp for schedule(dynamic, 1)
		for (auto i = moves.begin(); i < moves.end(); i++)
		{
			if (alpha < beta)
			{
				auto curEstimate = alphabeta(localField, depth - 1, *i, &curTrajectories, -alpha - 1, -alpha, emptyBoard);
				if (curEstimate > alpha && curEstimate < beta)
					curEstimate = alphabeta(localField, depth - 1, *i, &curTrajectories, -beta, -curEstimate, emptyBoard);
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

		delete emptyBoard;
		delete localField;
	}
	return alpha == get_enemy_estimate(field, &curTrajectories, depth - 1) ? -1 : result;
}
