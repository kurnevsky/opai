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

Bot::Bot(const int width, const int height, const BeginPattern beginPattern, int64_t seed)
{
  _gen = new mt19937_64(seed);
  _zobrist = new Zobrist((width + 2) * (height + 2), _gen);
  _field = new Field(width, height, beginPattern, _zobrist);
  _uctRoot = initUct(_field);
}

Bot::~Bot()
{
  delete _field;
  delete _zobrist;
  delete _gen;
  finalUct(_uctRoot);
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

bool Bot::doStep(int x, int y, int player)
{
  return _field->doStep(_field->toPos(x, y), player);
}

bool Bot::undoStep()
{
  if (_field->getMovesCount() == 0)
    return false;
  _field->undoStep();
  return true;
}

void Bot::setPlayer(int player)
{
  _field->setPlayer(player);
}

bool Bot::isFieldOccupied() const
{
  for (int i = _field->minPos(); i <= _field->maxPos(); i++)
    if (_field->isPuttingAllowed(i))
      return false;
  return true;
}

bool Bot::boundaryCheck(int& x, int& y) const
{
  int width = _field->getWidth();
  int height = _field->getHeight();
  if (width < 3 || height < 3)
  {
    x = -1;
    y = -1;
    return true;
  }
  if (_field->getMovesCount() == 0)
  {
    x = _field->getWidth() / 2;
    y = _field->getHeight() / 2;
    return true;
  }
  if (_field->getMovesCount() == 1)
  {
    _field->toXY(_field->getPointsSeq()[0], x, y);
    if (x == 0 || x == width - 1 || y == 0 || y == height - 1)
    {
      x = width / 2;
      y = height / 2;
    }
    else if (min(x, width - x - 1) < min(y, height - y - 1))
    {
      if (x < width - x - 1)
        x++;
      else
        x--;
    }
    else if (min(x, width - x - 1) > min(y, height - y - 1))
    {
      if (y < height - y - 1)
        y++;
      else
        y--;
    }
    else
    {
      int dx = x - width / 2;
      int dy = y - height / 2;
      if (abs(dx) > abs(dy))
      {
        if (dx < 0)
          x++;
        else
          x--;
      }
      else
      {
        if (dy < 0)
          y++;
        else
          y--;
      }
    }
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

void Bot::get(int& x, int& y)
{
  if (boundaryCheck(x, y))
    return;
#if SEARCH_TYPE == 0 // position estimate
  int result = positionEstimate(_field);
  x = _field->toX(result);
  y = _field->toY(result);
#elif SEARCH_TYPE == 1 // minimax
  int result =  minimax(_field, DEFAULT_MINIMAX_DEPTH);
  if (result == -1)
    result = positionEstimate(_field);
  x = _field->toX(result);
  y = _field->toY(result);
#elif SEARCH_TYPE == 2 // uct
  updateUct(_field, _uctRoot);
  int result = uct(_uctRoot, _field, _gen, DEFAULT_UCT_ITERATIONS);
  if (result == -1)
    result = positionEstimate(_field);
  x = _field->toX(result);
  y = _field->toY(result);
#elif SEARCH_TYPE == 3 // minimax with uct
  int result =  minimax(_field, DEFAULT_MINIMAX_DEPTH);
  if (result == -1)
  {
    updateUct(_field, _uctRoot);
    result = uct(_uctRoot, _field, _gen, DEFAULT_UCT_ITERATIONS);
  }
  if (result == -1)
    result = positionEstimate(_field);
  x = _field->toX(result);
  y = _field->toY(result);
#elif SEARCH_TYPE == 4 // MTD(f)
  int result =  mtdf(_field, DEFAULT_MTDF_DEPTH);
  if (result == -1)
    result = positionEstimate(_field);
  x = _field->toX(result);
  y = _field->toY(result);
#elif SEARCH_TYPE == 5 // MTD(f) with uct
  int result =  mtdf(_field, DEFAULT_MTDF_DEPTH);
  if (result == -1)
  {
    updateUct(_field, _uctRoot);
    result = uct(_uctRoot, _field, _gen, DEFAULT_UCT_ITERATIONS);
  }
  if (result == -1)
    result = positionEstimate(_field);
  x = _field->toX(result);
  y = _field->toY(result);
#else
#error Invalid SEARCH_TYPE.
#endif
}

void Bot::getWithComplexity(int& x, int& y, int complexity)
{
  if (boundaryCheck(x, y))
    return;
#if SEARCH_WITH_COMPLEXITY_TYPE == 0 // positon estimate
  int result = positionEstimate(_field);
  x = _field->toX(result);
  y = _field->toY(result);
#elif SEARCH_WITH_COMPLEXITY_TYPE == 1 // minimax
  int result =  minimax(_field, getMinimaxDepth(complexity));
  if (result == -1)
    result = positionEstimate(_field);
  x = _field->toX(result);
  y = _field->toY(result);
#elif SEARCH_WITH_COMPLEXITY_TYPE == 2 // uct
  updateUct(_field, _uctRoot);
  int result = uct(_uctRoot, _field, _gen, getUctIterations(complexity));
  if (result == -1)
    result = positionEstimate(_field);
  x = _field->toX(result);
  y = _field->toY(result);
#elif SEARCH_WITH_COMPLEXITY_TYPE == 3 // minimax with uct
  int result =  minimax(_field, getMinimaxDepth(complexity));
  if (result == -1)
  {
    updateUct(_field, _uctRoot);
    result = uct(_uctRoot, _field, _gen, getUctIterations(complexity));
  }
  if (result == -1)
    result = positionEstimate(_field);
  x = _field->toX(result);
  y = _field->toY(result);
#elif SEARCH_WITH_COMPLEXITY_TYPE == 4 // MTD(f)
  int result =  mtdf(_field, getMtdfDepth(complexity));
  if (result == -1)
    result = positionEstimate(_field);
  x = _field->toX(result);
  y = _field->toY(result);
#elif SEARCH_WITH_COMPLEXITY_TYPE == 5 // MTD(f) with uct
  int result =  mtdf(_field, getMtdfDepth(complexity));
  if (result == -1)
  {
    updateUct(_field, _uctRoot);
    result = uct(_uctRoot, _field, _gen, getUctIterations(complexity));
  }
  if (result == -1)
    result = positionEstimate(_field);
  x = _field->toX(result);
  y = _field->toY(result);
#else
#error Invalid SEARCH_WITH_COMPLEXITY_TYPE.
#endif
}

void Bot::getWithTime(int& x, int& y, int time)
{
  if (boundaryCheck(x, y))
    return;
#if SEARCH_WITH_TIME_TYPE == 0 // position estimate
  int result = positionEstimate(_field);
  x = _field->toX(result);
  y = _field->toY(result);
#elif SEARCH_WITH_TIME_TYPE == 1 // minimax
#error Invalid SEARCH_WITH_TIME_TYPE.
#elif SEARCH_WITH_TIME_TYPE == 2 // uct
  updateUct(_field, _uctRoot);
  int result = uctWithTime(_uctRoot, _field, _gen, time);
  if (result == -1)
    result = positionEstimate(_field);
  x = _field->toX(result);
  y = _field->toY(result);
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
