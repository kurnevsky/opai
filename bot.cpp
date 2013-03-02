#include "config.h"
#include "basic_types.h"
#include "bot.h"
#include "time.h"
#include "field.h"
#include "minimax.h"
#include "uct.h"
#include "position_estimate.h"
#include "zobrist.h"
#include <list>
#include "mtdf.h"

using namespace std;

Bot::Bot(const Coord width, const Coord height, const BeginPattern beginPattern, ptrdiff_t seed)
{
	_gen = new mt(seed);
	_zobrist = new Zobrist((width + 2) * (height + 2), _gen);
	_field = new Field(width, height, beginPattern, _zobrist);
}

Bot::~Bot()
{
	delete _field;
	delete _zobrist;
	delete _gen;
}

int Bot::getMinimaxDepth(int complexity)
{
	return (complexity - MIN_COMPLEXITY) * (MAX_MINIMAX_DEPTH - MIN_MINIMAX_DEPTH) / (MAX_COMPLEXITY - MIN_COMPLEXITY) + MIN_MINIMAX_DEPTH;
}

int Bot::getMtdfDepth(int complexity)
{
	return (complexity - MIN_COMPLEXITY) * (MAX_MTDF_DEPTH - MIN_MTDF_DEPTH) / (MAX_COMPLEXITY - MIN_COMPLEXITY) + MIN_MTDF_DEPTH;
}

int Bot::getUctIterations(int complexity)
{
	return (complexity - MIN_COMPLEXITY) * (MAX_UCT_ITERATIONS - MIN_UCT_ITERATIONS) / (MAX_COMPLEXITY - MIN_COMPLEXITY) + MIN_UCT_ITERATIONS;
}

bool Bot::doStep(Coord x, Coord y, Player player)
{
	return _field->do_step(_field->to_pos(x, y), player);
}

bool Bot::undoStep()
{
	if (_field->pointsSeq.size() == 0)
		return false;
	_field->undo_step();
	return true;
}

void Bot::setPlayer(Player player)
{
	_field->set_player(player);
}

bool Bot::isFieldOccupied() const
{
	for (auto i = _field->min_pos(); i <= _field->max_pos(); i++)
		if (_field->puttingAllow(i))
			return false;
	return true;
}

bool Bot::boundaryCheck(Coord& x, Coord& y) const
{
	if (_field->pointsSeq.size() == 0)
	{
		x = _field->getWidth() / 2;
		y = _field->getHeight() / 2;
		return true;
	}
	if (isFieldOccupied())
	{
		x = -1;
		y = -1;
		return true;
	}
	return false;
}

void Bot::get(Coord& x, Coord& y)
{
	if (boundaryCheck(x, y))
		return;
#if SEARCH_TYPE == 0 // position estimate
	auto result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_TYPE == 1 // minimax
	auto result =  minimax(_field, DEFAULT_MINIMAX_DEPTH);
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_TYPE == 2 // uct
	auto result = uct(_field, _gen, DEFAULT_UCT_ITERATIONS);
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_TYPE == 3 // minimax with uct
	auto result =  minimax(_field, DEFAULT_MINIMAX_DEPTH);
	if (result == -1)
		result = uct(_field, _gen, DEFAULT_UCT_ITERATIONS);
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_TYPE == 4 // MTD(f)
	auto result =  mtdf(_field, DEFAULT_MTDF_DEPTH);
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_TYPE == 5 // MTD(f) with uct
	auto result =  mtdf(_field, DEFAULT_MTDF_DEPTH);
	if (result == -1)
		result = uct(_field, _gen, DEFAULT_UCT_ITERATIONS);
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#else
#error Invalid SEARCH_TYPE.
#endif
}

void Bot::getWithComplexity(Coord& x, Coord& y, int complexity)
{
	if (boundaryCheck(x, y))
		return;
#if SEARCH_WITH_COMPLEXITY_TYPE == 0 // positon estimate
	auto result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_WITH_COMPLEXITY_TYPE == 1 // minimax
	auto result =  minimax(_field, getMinimaxDepth(complexity));
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_WITH_COMPLEXITY_TYPE == 2 // uct
	auto result = uct(_field, _gen, getUctIterations(complexity));
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_WITH_COMPLEXITY_TYPE == 3 // minimax with uct
	auto result =  minimax(_field, getMinimaxDepth(complexity));
	if (result == -1)
		result = uct(_field, _gen, getUctIterations(complexity));
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_WITH_COMPLEXITY_TYPE == 4 // MTD(f)
	auto result =  mtdf(_field, getMtdfDepth(complexity));
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_WITH_COMPLEXITY_TYPE == 5 // MTD(f) with uct
	auto result =  mtdf(_field, getMtdfDepth(complexity));
	if (result == -1)
		result = uct(_field, _gen, getUctIterations(complexity));
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#else
#error Invalid SEARCH_WITH_COMPLEXITY_TYPE.
#endif
}

void Bot::getWithTime(Coord& x, Coord& y, Time time)
{
	if (boundaryCheck(x, y))
		return;
#if SEARCH_WITH_TIME_TYPE == 0 // position estimate
	auto result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_WITH_TIME_TYPE == 1 // minimax
#error Invalid SEARCH_WITH_TIME_TYPE.
#elif SEARCH_WITH_TIME_TYPE == 2 // uct
	auto result = uct_with_time(_field, _gen, time);
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_WITH_TIME_TYPE == 3 // minimax with uct
#error Invalid SEARCH_WITH_TIME_TYPE.
#elif SEARCH_WITH_TIME_TYPE == 4 // MTD(f)
#error Invalid SEARCH_WITH_TIME_TYPE.
#elif SEARCH_WITH_TIME_TYPE == 5 // MTD(f) with uct
#error Invalid SEARCH_WITH_TIME_TYPE.
#else
#error Invalid SEARCH_WITH_TIME_TYPE.
#endif
}
