#pragma once

#include "config.h"
#include "basic_types.h"
#include "player.h"
#include "field.h"
#include "position_estimate.h"
#include "uct.h"
#include "minimax.h"
#include "zobrist.h"

using namespace std;

class Bot
{
private:
	mt* _gen;
	zobrist* _zobrist;
	Field* _field;
	size_t get_minimax_depth(size_t complexity);
	size_t get_mtdf_depth(size_t complexity);
	size_t get_uct_iterations(size_t complexity);
	bool is_field_occupied() const;
	bool boundary_check(Coord& x, Coord& y) const;
public:
	Bot(const Coord width, const Coord height, const BeginPattern begin_pattern, ptrdiff_t seed);
	~Bot();
	bool do_step(Coord x, Coord y, Player player);
	bool undo_step();
	void set_player(Player player);
	// Возвращает лучший найденный ход.
	void get(Coord& x, Coord& y);
	void get_with_complexity(Coord& x, Coord& y, size_t complexity);
	void get_with_time(Coord& x, Coord& y, Time time);
};
