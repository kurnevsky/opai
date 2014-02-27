#pragma once

#include "config.h"
#include "basic_types.h"
#include "field.h"
#include "trajectories.h"

using namespace std;

Score alphabeta(Field* field, int depth, Pos pos, Trajectories* last, Score alpha, Score beta, int* emptyBoard);
Score getEnemyEstimate(Field** fields, int** emptyBoards, int maxThreads, Trajectories* last, int depth);
Pos minimax(Field* field, int depth);
