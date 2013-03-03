#pragma once

#include "config.h"
#include <utility>
#include <stack>
#include <boost/random.hpp>

using namespace std;
using namespace boost;

typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef int Pos;
typedef int PosValue;
typedef int Player;
typedef int Score;
typedef int Coord;
typedef size_t Hash;
typedef uint Time;
typedef uint Depth;
typedef uint Complexity;
typedef uint Iterations;

#define SCORE_INFINITY numeric_limits<Score>::max()

// Структура координат точки.
struct Point
{
	Coord x, y;
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
	Score captureCount[2];
	// Предыдущий игрок.
	Player player;
	// Предыдущий хеш.
	Hash hash;
	// Список изменных точек (координата - значение до изменения).
	stack<pair<Pos, PosValue>> changes;
};

#if ENVIRONMENT_32
typedef random::mt19937 mt;
#elif ENVIRONMENT_64
typedef random::mt19937_64 mt;
#endif
