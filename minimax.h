#pragma once

#include "config.h"
#include "basic_types.h"
#include "field.h"
#include "trajectories.h"

using namespace std;

Score alphabeta(Field* field, size_t depth, Pos pos, Trajectories* last, Score alpha, Score beta, int* empty_board);
Pos minimax(Field* field, size_t depth);
