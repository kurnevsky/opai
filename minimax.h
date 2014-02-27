﻿#pragma once

#include "config.h"
#include "basic_types.h"
#include "field.h"
#include "trajectories.h"

using namespace std;

int alphabeta(Field* field, int depth, Pos pos, Trajectories* last, int alpha, int beta, int* emptyBoard);
int getEnemyEstimate(Field** fields, int** emptyBoards, int maxThreads, Trajectories* last, int depth);
Pos minimax(Field* field, int depth);
