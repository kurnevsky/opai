#include "config.h"
#include "basic_types.h"
#include "player.h"
#include "position_estimate.h"
#include <list>
#include <limits>

using namespace std;

const int cgSumma[] = {-5, -1, 0, 0, 1, 2, 5, 20, 30};

// Учет позиционных эвристик.
int positionEstimate(Field* field, Pos pos, int player)
{
  int g1 = field->numberNearGroups(pos, player);
  int g2 = field->numberNearGroups(pos, nextPlayer(player));
  int c1 = cgSumma[field->numberNearPoints(pos, player)];
  int c2 = cgSumma[field->numberNearPoints(pos, nextPlayer(player))];
  int result = (g1 * 3 + g2 * 2) * (5 - abs(g1 - g2)) - c1 - c2;
  if (field->pointsSeq.size() > 0 && field->isNear(field->pointsSeq.back(), pos))
    result += 5;
  return result;
}

Pos positionEstimate(Field* field)
{
  int bestEstimate = numeric_limits<int>::min();
  Pos result = -1;
  for (Pos i = field->minPos(); i <= field->maxPos(); i++)
    if (field->puttingAllow(i))
    {
      int curEstimate = positionEstimate(field, i, field->getPlayer());
      if (curEstimate > bestEstimate)
      {
        bestEstimate = curEstimate;
        result = i;
      }
    }
  return result;
}
