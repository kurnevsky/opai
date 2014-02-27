#pragma once

#include "config.h"
#include "basic_types.h"
#include "field.h"
#include <list>

using namespace std;

struct uctNode
{
  int wins;
  int visits;
  Pos move;
  uctNode* child;
  uctNode* sibling;

  uctNode()
  {
    wins = 0;
    visits = 0;
    move = 0;
    child = NULL;
    sibling = NULL;
  }
};

Pos uct(Field* field, mt* gen, int maxSimulations);
Pos uctWithTime(Field* field, mt* gen, int time);
