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

Bot::Bot(const Coord width, const Coord height, const BeginPattern begin_pattern, ptrdiff_t seed)
{
	_gen = new mt(seed);
	_zobrist = new Zobrist((width + 2) * (height + 2), _gen);
	_field = new Field(width, height, begin_pattern, _zobrist);
}

Bot::~Bot()
{
	delete _field;
	delete _zobrist;
	delete _gen;
}

size_t Bot::get_minimax_depth(size_t complexity)
{
	return (complexity - MIN_COMPLEXITY) * (MAX_MINIMAX_DEPTH - MIN_MINIMAX_DEPTH) / (MAX_COMPLEXITY - MIN_COMPLEXITY) + MIN_MINIMAX_DEPTH;
}

size_t Bot::get_mtdf_depth(size_t complexity)
{
	return (complexity - MIN_COMPLEXITY) * (MAX_MTDF_DEPTH - MIN_MTDF_DEPTH) / (MAX_COMPLEXITY - MIN_COMPLEXITY) + MIN_MTDF_DEPTH;
}

size_t Bot::get_uct_iterations(size_t complexity)
{
	return (complexity - MIN_COMPLEXITY) * (MAX_UCT_ITERATIONS - MIN_UCT_ITERATIONS) / (MAX_COMPLEXITY - MIN_COMPLEXITY) + MIN_UCT_ITERATIONS;
}

bool Bot::do_step(Coord x, Coord y, Player player)
{
	return _field->do_step(_field->to_pos(x, y), player);
}

bool Bot::undo_step()
{
	if (_field->points_seq.size() == 0)
		return false;
	_field->undo_step();
	return true;
}

void Bot::set_player(Player player)
{
	_field->set_player(player);
}

bool Bot::is_field_occupied() const
{
	for (Pos i = _field->min_pos(); i <= _field->max_pos(); i++)
		if (_field->putting_allow(i))
			return false;
	return true;
}

bool Bot::boundary_check(Coord& x, Coord& y) const
{
	if (_field->points_seq.size() == 0)
	{
		x = _field->width() / 2;
		y = _field->height() / 2;
		return true;
	}
	if (is_field_occupied())
	{
		x = -1;
		y = -1;
		return true;
	}
	return false;
}

void Bot::get(Coord& x, Coord& y)
{
	if (boundary_check(x, y))
		return;
#if SEARCH_TYPE == 0 // position estimate
	pos result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_TYPE == 1 // minimax
	pos result =  minimax(_field, DEFAULT_MINIMAX_DEPTH);
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_TYPE == 2 // uct
	pos result = uct(_field, _gen, DEFAULT_UCT_ITERATIONS);
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_TYPE == 3 // minimax with uct
	Pos result =  minimax(_field, DEFAULT_MINIMAX_DEPTH);
	if (result == -1)
		result = uct(_field, _gen, DEFAULT_UCT_ITERATIONS);
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_TYPE == 4 // MTD(f)
	Pos result =  mtdf(_field, DEFAULT_MTDF_DEPTH);
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_TYPE == 5 // MTD(f) with uct
	Pos result =  mtdf(_field, DEFAULT_MTDF_DEPTH);
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

void Bot::get_with_complexity(Coord& x, Coord& y, size_t complexity)
{
	if (boundary_check(x, y))
		return;
#if SEARCH_WITH_COMPLEXITY_TYPE == 0 // positon estimate
	pos result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_WITH_COMPLEXITY_TYPE == 1 // minimax
	pos result =  minimax(_field, get_minimax_depth(complexity));
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_WITH_COMPLEXITY_TYPE == 2 // uct
	pos result = uct(_field, _gen, get_uct_iterations(complexity));
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_WITH_COMPLEXITY_TYPE == 3 // minimax with uct
	Pos result =  minimax(_field, get_minimax_depth(complexity));
	if (result == -1)
		result = uct(_field, _gen, get_uct_iterations(complexity));
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_WITH_COMPLEXITY_TYPE == 4 // MTD(f)
	Pos result =  mtdf(_field, get_mtdf_depth(complexity));
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_WITH_COMPLEXITY_TYPE == 5 // MTD(f) with uct
	Pos result =  mtdf(_field, get_mtdf_depth(complexity));
	if (result == -1)
		result = uct(_field, _gen, get_uct_iterations(complexity));
	if (result == -1)
		result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#else
#error Invalid SEARCH_WITH_COMPLEXITY_TYPE.
#endif
}

void Bot::get_with_time(Coord& x, Coord& y, Time time)
{
	if (boundary_check(x, y))
		return;
#if SEARCH_WITH_TIME_TYPE == 0 // position estimate
	Pos result = position_estimate(_field);
	x = _field->to_x(result);
	y = _field->to_y(result);
#elif SEARCH_WITH_TIME_TYPE == 1 // minimax
#error Invalid SEARCH_WITH_TIME_TYPE.
#elif SEARCH_WITH_TIME_TYPE == 2 // uct
	Pos result = uct_with_time(_field, _gen, time);
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
