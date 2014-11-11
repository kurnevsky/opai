#pragma once

#include <atomic>
#include <algorithm>
#include <vector>
#include "field.h"

using namespace std;

// Node of UCT tree.
struct UctNode
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
  atomic<UctNode*> child;
  // Sibling node.
  UctNode* sibling;
  // Constructor.
  UctNode() : sibling(nullptr), move(0)
  {
    wins.store(0, memory_order_relaxed);
    draws.store(0, memory_order_relaxed);
    visits.store(0, memory_order_relaxed);
    child.store(nullptr, memory_order_relaxed);
  }
};

struct UctRoot
{
  UctNode* node;
  vector<int> moves;
  bool* movesField;
  int player;
  vector<int> pointsSeq;
  int komi;
  int komiIter;
  UctRoot(int length) : node(nullptr), player(-1), komi(0), komiIter(0)
  {
    movesField = new bool[length];
    fill_n(movesField, length, false);
  }
  ~UctRoot()
  {
    delete[] movesField;
  }
};

void updateUct(Field* field, UctRoot* root);

UctRoot* initUct(Field* field);

void finalUct(UctRoot* root);

// Get best move by UCT analysis.
// field - field to find best move.
// gen - random number generator.
// maxSimulations - number of UCT simulations.
// Returns position of best move, or -1 if not found.
int uct(UctRoot* root, Field* field, mt19937_64* gen, int maxSimulations);

// Get best move by UCT analysis.
// field - field to find best move.
// gen - random number generator.
// time - number of milliseconds to thinking.
// Returns position of best move, or -1 if not found.
int uctWithTime(UctRoot* root, Field* field, mt19937_64* gen, int time);
