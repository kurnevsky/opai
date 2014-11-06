#pragma once

#include <atomic>
#include "field.h"

using namespace std;

// Node of UCT tree.
struct uctNode
{
  // Number of wins of this node.
  atomic<int> wins;
  // Number of draws of this node
  atomic<int> draws;
  // Number of visits to this node
  atomic<int> visits;
  // Position of move.
  int move;
  // Child node.
  atomic<uctNode*> child;
  // Sibling node.
  uctNode* sibling;
  // Constructor.
  uctNode()
  {
    wins = 0;
    draws = 0;
    visits = 0;
    move = 0;
    child = nullptr;
    sibling = nullptr;
  }
};

// Get best move by UCT analysis.
// field - field to find best move.
// gen - random number generator.
// maxSimulations - number of UCT simulations.
// Returns position of best move, or -1 if not found.
int uct(Field* field, mt19937_64* gen, int maxSimulations);

// Get best move by UCT analysis.
// field - field to find best move.
// gen - random number generator.
// time - number of milliseconds to thinking.
// Returns position of best move, or -1 if not found.
int uctWithTime(Field* field, mt19937_64* gen, int time);
