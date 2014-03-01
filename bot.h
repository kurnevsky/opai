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
  mt19937_64* _gen;
  Zobrist* _zobrist;
  Field* _field;
  int getMinimaxDepth(int complexity);
  int getMtdfDepth(int complexity);
  int getUctIterations(int complexity);
  bool isFieldOccupied() const;
  bool boundaryCheck(int& x, int& y) const;
public:
  Bot(const int width, const int height, const BeginPattern beginPattern, int64_t seed);
  ~Bot();
  bool doStep(int x, int y, Player player);
  bool undoStep();
  void setPlayer(Player player);
  // Возвращает лучший найденный ход.
  void get(int& x, int& y);
  void getWithComplexity(int& x, int& y, int complexity);
  void getWithTime(int& x, int& y, int time);
};
