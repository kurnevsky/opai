#pragma once

#include "config.h"
#include "basic_types.h"
#include <climits>
#include <time.h>

using namespace std;

inline int getTime()
{
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

class Timer
{
private:
  int _lastTime;

public:
  Timer() { set(); }
  void set() { _lastTime = getTime(); }
  int get()
  {
    int curTime = getTime();
    if (curTime > _lastTime)
      return curTime - _lastTime;
    else
      return numeric_limits<int>::max() - _lastTime + curTime + 1;
  }
};
