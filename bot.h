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
	Zobrist* _zobrist;
	Field* _field;
	int getMinimaxDepth(int complexity);
	int getMtdfDepth(int complexity);
	int getUctIterations(int complexity);
	bool isFieldOccupied() const;
	bool boundaryCheck(Coord& x, Coord& y) const;
public:
	Bot(const Coord width, const Coord height, const BeginPattern beginPattern, ptrdiff_t seed);
	~Bot();
	bool doStep(Coord x, Coord y, Player player);
	bool undoStep();
	void setPlayer(Player player);
	// Возвращает лучший найденный ход.
	void get(Coord& x, Coord& y);
	void getWithComplexity(Coord& x, Coord& y, int complexity);
	void getWithTime(Coord& x, Coord& y, Time time);
};
