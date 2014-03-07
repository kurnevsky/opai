#pragma once

#include "config.h"
#include <utility>
#include <stack>
#include <random>

using namespace std;

typedef int Pos;

// Структура координат точки.
struct Point
{
  int x, y;
};

// Используемый шаблон в начале игры.
// BEGIN_PATTERN_CLEAN - начало с чистого поля.
// BEGIN_PATTERN_CROSSWIRE - начало со скреста.
// BEGIN_PATTERN_SQUARE - начало с квадрата.
enum BeginPattern
{
  BEGIN_PATTERN_CLEAN,
  BEGIN_PATTERN_CROSSWIRE,
  BEGIN_PATTERN_SQUARE
};

enum IntersectionState
{
  INTERSECTION_STATE_NONE,
  INTERSECTION_STATE_UP,
  INTERSECTION_STATE_DOWN,
  INTERSECTION_STATE_TARGET
};

// Одно изменение доски.
struct BoardChange
{
  // Предыдущий счет захваченных точек.
  int captureCount[2];
  // Предыдущий игрок.
  int player;
  // Предыдущий хеш.
  int64_t hash;
  // Список изменных точек (координата - значение до изменения).
  stack<pair<int, int>> changes;
};
