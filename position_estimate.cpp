﻿#include "config.h"
#include "basic_types.h"
#include "player.h"
#include "position_estimate.h"
#include <list>
#include <limits>

using namespace std;

// Учет позиционных эвристик.
int position_estimate(Field* field, Pos pos, Player player)
{
	int g1, g2;
	int c1, c2;
	int result;

	g1 = field->number_near_groups(pos, player);
	g2 = field->number_near_groups(pos, next_player(player));
	c1 = cg_summa[field->number_near_points(pos, player)];
	c2 = cg_summa[field->number_near_points(pos, next_player(player))];
	result = (g1 * 3 + g2 * 2) * (5 - abs(g1 - g2)) - c1 - c2;
	if (field->pointsSeq.size() > 0 && field->is_near(field->pointsSeq.back(), pos))
		result += 5;
	// Эмпирическая формула оценки важности точки при просчете ходов.
	return result;
}

Pos position_estimate(Field* field)
{
	auto best_estimate = numeric_limits<int>::min();
	Pos result = -1;
	for (Pos i = field->min_pos(); i <= field->max_pos(); i++)
		if (field->puttingAllow(i))
		{
			auto cur_estimate = position_estimate(field, i, field->getPlayer());
			if (cur_estimate > best_estimate)
			{
				best_estimate = cur_estimate;
				result = i;
			}
		}
	return result;
}
