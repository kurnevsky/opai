#pragma once

#include "config.h"
#include "basic_types.h"
#include "field.h"
#include "trajectories.h"

using namespace std;

Score alphabeta(Field* field, Depth depth, Pos pos, Trajectories* last, Score alpha, Score beta, int* emptyBoard);
Pos minimax(Field* field, Depth depth);
