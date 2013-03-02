#include "config.h"
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

	g1 = field->numberNearGroups(pos, player);
	g2 = field->numberNearGroups(pos, nextPlayer(player));
	c1 = cg_summa[field->numberNearPoints(pos, player)];
	c2 = cg_summa[field->numberNearPoints(pos, nextPlayer(player))];
	result = (g1 * 3 + g2 * 2) * (5 - abs(g1 - g2)) - c1 - c2;
	if (field->pointsSeq.size() > 0 && field->isNear(field->pointsSeq.back(), pos))
		result += 5;
	// Эмпирическая формула оценки важности точки при просчете ходов.
	return result;
}

Pos position_estimate(Field* field)
{
	auto best_estimate = numeric_limits<int>::min();
	auto result = -1;
	for (auto i = field->minPos(); i <= field->maxPos(); i++)
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
