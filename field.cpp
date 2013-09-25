#include "field.h"

using namespace std;

int Field::getInputPoints(const Pos centerPos, const PosValue enableCond, Pos inpChainPoints[], Pos inpSurPoints[]) const
{
	int result = 0;
	if (isNotEnable(w(centerPos), enableCond))
	{
		if (isEnable(nw(centerPos), enableCond))
		{
			inpChainPoints[0] = nw(centerPos);
			inpSurPoints[0] = w(centerPos);
			result++;
		}
		else if (isEnable(n(centerPos), enableCond))
		{
			inpChainPoints[0] = n(centerPos);
			inpSurPoints[0] = w(centerPos);
			result++;
		}
	}
	if (isNotEnable(s(centerPos), enableCond))
	{
		if (isEnable(sw(centerPos), enableCond))
		{
			inpChainPoints[result] = sw(centerPos);
			inpSurPoints[result] = s(centerPos);
			result++;
		}
		else if (isEnable(w(centerPos), enableCond))
		{
			inpChainPoints[result] = w(centerPos);
			inpSurPoints[result] = s(centerPos);
			result++;
		}
	}
	if (isNotEnable(e(centerPos), enableCond))
	{
		if (isEnable(se(centerPos), enableCond))
		{
			inpChainPoints[result] = se(centerPos);
			inpSurPoints[result] = e(centerPos);
			result++;
		}
		else if (isEnable(s(centerPos), enableCond))
		{
			inpChainPoints[result] = s(centerPos);
			inpSurPoints[result] = e(centerPos);
			result++;
		}
	}
	if (isNotEnable(n(centerPos), enableCond))
	{
		if (isEnable(ne(centerPos), enableCond))
		{
			inpChainPoints[result] = ne(centerPos);
			inpSurPoints[result] = n(centerPos);
			result++;
		}
		else if (isEnable(e(centerPos), enableCond))
		{
			inpChainPoints[result] = e(centerPos);
			inpSurPoints[result] = n(centerPos);
			result++;
		}
	}
	return result;
}

void Field::placeBeginPattern(BeginPattern pattern)
{
	switch (pattern)
	{
	case (BEGIN_PATTERN_CROSSWIRE):
		doStep(toPos(_width / 2 - 1, _height / 2 - 1));
		doStep(toPos(_width / 2, _height / 2 - 1));
		doStep(toPos(_width / 2, _height / 2));
		doStep(toPos(_width / 2 - 1, _height / 2));
		break;
	case (BEGIN_PATTERN_SQUARE):
		doStep(toPos(_width / 2 - 1, _height / 2 - 1));
		doStep(toPos(_width / 2, _height / 2 - 1));
		doStep(toPos(_width / 2 - 1, _height / 2));
		doStep(toPos(_width / 2, _height / 2));
		break;
	case (BEGIN_PATTERN_CLEAN):
		break;
	}
}

void Field::removeEmptyBase(const Pos startPos)
{
	wave(	startPos,
			[&](Pos pos)->bool
			{
				if (isInEmptyBase(pos))
				{
					_changes.back().changes.push(pair<Pos, PosValue>(pos, _points[pos] & !tagBit));
					clearEmptyBase(pos);
					return true;
				}
				else
				{
					return false;
				}
			});
}

bool Field::buildChain(const Pos startPos, const PosValue enableCond, const Pos directionPos, list<Pos> &chain)
{
	chain.clear();
	chain.push_back(startPos);
	auto pos = directionPos;
	auto centerPos = startPos;
	// Площадь базы.
	auto baseSquare = square(centerPos, pos);
	do
	{
		if (isTagged(pos))
		{
			while (chain.back() != pos)
			{
				clearTag(chain.back());
				chain.pop_back();
			}
		}
		else
		{
			setTag(pos);
			chain.push_back(pos);
		}
		swap(pos, centerPos);
		getFirstNextPos(centerPos, pos);
		while (isNotEnable(pos, enableCond))
			getNextPos(centerPos, pos);
		baseSquare += square(centerPos, pos);
	}
	while (pos != startPos);
	for (auto i = chain.begin(); i != chain.end(); i++)
		clearTag(*i);
	return (baseSquare < 0 && chain.size() > 2);
}

void Field::findSurround(list<Pos> &chain, Pos insidePoint, Player player)
{
	// Количество захваченных точек.
	auto curCaptureCount = 0;
	// Количество захваченных пустых полей.
	auto curFreedCount = 0;
	list<Pos> surPoints;
	// Помечаем точки цепочки.
	for (auto i = chain.begin(); i != chain.end(); i++)
		setTag(*i);
	wave(	insidePoint,
			[&, player](Pos pos)->bool
			{
				if (isNotBound(pos, player | putBit | boundBit))
				{
					checkCapturedAndFreed(pos, player, curCaptureCount, curFreedCount);
					surPoints.push_back(pos);
					return true;
				}
				else
				{
					return false;
				}
			});
	// Изменение счета игроков.
	addSubCapturedFreed(player, curCaptureCount, curFreedCount);
#if SUR_COND != 1
	if (curCaptureCount != 0) // Если захватили точки.
#endif
	{
		for (auto i = chain.begin(); i != chain.end(); i++)
		{
			clearTag(*i);
			// Добавляем в список изменений точки цепочки.
			_changes.back().changes.push(pair<Pos, PosValue>(*i, _points[*i]));
			// Помечаем точки цепочки.
			setBaseBound(*i);
		}
		for (auto i = surPoints.begin(); i != surPoints.end(); i++)
		{
			_changes.back().changes.push(pair<Pos, PosValue>(*i, _points[*i]));

			setCaptureFreeState(*i, player);
		}
	}
	else // Если ничего не захватили.
	{
		for (auto i = chain.begin(); i != chain.end(); i++)
			clearTag(*i);
		for (auto i = surPoints.begin(); i != surPoints.end(); i++)
		{
			_changes.back().changes.push(pair<Pos, PosValue>(*i, _points[*i]));

			if (!isPutted(*i))
				setEmptyBase(*i);
		}
	}
}

Field::Field(const Coord width, const Coord height, const BeginPattern begin_pattern, Zobrist* zobrist)
{
	_width = width;
	_height = height;
	_player = playerRed;
	_captureCount[playerRed] = 0;
	_captureCount[playerBlack] = 0;
	_points = new PosValue[getLength()];
	fill_n(_points, getLength(), 0);
	for (Coord x = -1; x <= width; x++)
	{
		setBad(toPos(x, -1));
		setBad(toPos(x, height));
	}
	for (Coord y = -1; y <= height; y++)
	{
		setBad(toPos(-1, y));
		setBad(toPos(width, y));
	}
	_changes.reserve(getLength());
	pointsSeq.reserve(getLength());
	_zobrist = zobrist;
	_hash = 0;
	placeBeginPattern(begin_pattern);
}

Field::Field(const Field &orig)
{
	_width = orig._width;
	_height = orig._height;
	_player = orig._player;
	_captureCount[playerRed] = orig._captureCount[playerRed];
	_captureCount[playerBlack] = orig._captureCount[playerBlack];
	_points = new PosValue[getLength()];
	copy_n(orig._points, getLength(), _points);
	_changes.reserve(getLength());
	pointsSeq.reserve(getLength());
	_changes.assign(orig._changes.begin(), orig._changes.end());
	pointsSeq.assign(orig.pointsSeq.begin(), orig.pointsSeq.end());
	_zobrist = orig._zobrist;
	_hash = orig._hash;
}

Field::~Field()
{
	delete _points;
}

void Field::wave(Pos startPos, function<bool(Pos)> cond)
{
	// Очередь для волнового алгоритма (обхода в ширину).
	list<Pos> q;
	if (!cond(startPos))
		return;
	q.push_back(startPos);
	setTag(startPos);
	auto it = q.begin();
	while (it != q.end())
	{
		if (!isTagged(w(*it)) && cond(w(*it)))
		{
			q.push_back(w(*it));
			setTag(w(*it));
		}
		if (!isTagged(n(*it)) && cond(n(*it)))
		{
			q.push_back(n(*it));
			setTag(n(*it));
		}
		if (!isTagged(e(*it)) && cond(e(*it)))
		{
			q.push_back(e(*it));
			setTag(e(*it));
		}
		if (!isTagged(s(*it)) && cond(s(*it)))
		{
			q.push_back(s(*it));
			setTag(s(*it));
		}
		it++;
	}
	for (it = q.begin(); it != q.end(); it++)
		clearTag(*it);
}

bool Field::isPointInsideRing(const Pos pos, const list<Pos> &ring) const
{
	int intersections = 0;
	auto state = INTERSECTION_STATE_NONE;
	for (auto i = ring.begin(); i != ring.end(); i++)
	{
		switch (getIntersectionState(pos, *i))
		{
		case (INTERSECTION_STATE_NONE):
			state = INTERSECTION_STATE_NONE;
			break;
		case (INTERSECTION_STATE_UP):
			if (state == INTERSECTION_STATE_DOWN)
				intersections++;
			state = INTERSECTION_STATE_UP;
			break;
		case (INTERSECTION_STATE_DOWN):
			if (state == INTERSECTION_STATE_UP)
				intersections++;
			state = INTERSECTION_STATE_DOWN;
			break;
		case (INTERSECTION_STATE_TARGET):
			break;
		}
	}
	if (state == INTERSECTION_STATE_UP || state == INTERSECTION_STATE_DOWN)
	{
		auto i = ring.begin();
		auto beginState = getIntersectionState(pos, *i);
		while (beginState == INTERSECTION_STATE_TARGET)
		{
			i++;
			beginState = getIntersectionState(pos, *i);
		}
		if ((state == INTERSECTION_STATE_UP && beginState == INTERSECTION_STATE_DOWN) ||
			(state == INTERSECTION_STATE_DOWN && beginState == INTERSECTION_STATE_UP))
			intersections++;
	}
	return intersections % 2 == 1;
}

void Field::checkClosure(const Pos startPos, Player player)
{
	int inpPointsCount;
	Pos inpChainPoints[4], inpSurPoints[4];
	list<Pos> chain;
	if (isInEmptyBase(startPos)) // Если точка поставлена в пустую базу.
	{
		// Проверяем, в чью пустую базу поставлена точка.
		Pos pos = startPos - 1;
		while (!isPutted(pos))
			pos--;

		if (getPlayer(pos) == getPlayer(startPos)) // Если поставили в свою пустую базу.
		{
			clearEmptyBase(startPos);
			return;
		}
#if SUR_COND != 2 // Если приоритет не всегда у врага.
		inpPointsCount = getInputPoints(startPos, player | putBit, inpChainPoints, inpSurPoints);
		if (inpPointsCount > 1)
		{
			auto chainsCount = 0;
			for (auto i = 0; i < inpPointsCount; i++)
				if (buildChain(startPos, getPlayer(startPos) | putBit, inpChainPoints[i], chain))
				{
					findSurround(chain, inpSurPoints[i], player);
					chainsCount++;
					if (chainsCount == inpPointsCount - 1)
						break;
				}
				if (isBaseBound(startPos))
				{
					removeEmptyBase(startPos);
					return;
				}
		}
#endif
		pos++;
		do
		{
			pos--;
			while (!isEnable(pos, nextPlayer(player) | putBit))
				pos--;
			inpPointsCount = getInputPoints(pos, nextPlayer(player) | putBit, inpChainPoints, inpSurPoints);
			for (auto i = 0; i < inpPointsCount; i++)
				if (buildChain(pos, nextPlayer(player) | putBit, inpChainPoints[i], chain))
					if (isPointInsideRing(startPos, chain))
					{
						findSurround(chain, inpSurPoints[i], nextPlayer(player));
						break;
					}
		} while (!isCaptured(startPos));
	}
	else
	{
		inpPointsCount = getInputPoints(startPos, player | putBit, inpChainPoints, inpSurPoints);
		if (inpPointsCount > 1)
		{
			auto chainsCount = 0;
			for (auto i = 0; i < inpPointsCount; i++)
				if (buildChain(startPos, player | putBit, inpChainPoints[i], chain))
				{
					findSurround(chain, inpSurPoints[i], player);
					chainsCount++;
					if (chainsCount == inpPointsCount - 1)
						break;
				}
		}
	}
}

//bool Field::check_closure(const Pos start_pos, const Pos checked_pos, Player player)
//{
//	Pos InpChainPoints[4], InpSurPoints[4];
//
//	list<Pos> chain;
//
//	short InpPointsCount = get_input_points(start_pos, cur_player | put_bit, InpChainPoints, InpSurPoints);
//	if (InpPointsCount > 1)
//	{
//		for (ushort i = 0; i < InpPointsCount; i++)
//			if (build_chain(start_pos, get_player(start_pos) | put_bit, InpChainPoints[i], chain))
//				if (is_point_inside_ring(checked_pos, chain))
//				{
//					for (auto j = chain.begin(); j != chain.end(); j++)
//					{
//						// Добавляем в список изменений точки цепочки.
//						_changes.back().changes.push(pair<pos, value>(*j, _points[*j]));
//						// Помечаем точки цепочки.
//						set_base_bound(*j);
//					}
//					return true;
//				}
//	}
//
//	return false;
//}
