#pragma once

#include "config.h"
#include "basic_types.h"
#include <climits>

#if WINDOWS
#include <windows.h>
#elif LINUX
#include <time.h>
#endif

using namespace std;

inline Time getTime()
{
#if WINDOWS
	return GetTickCount();
#elif LINUX
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#else
#error Unknown OS
#endif
}

class Timer
{
private:
	Time _lastTime;

public:
	Timer() { set(); }
	void set() { _lastTime = getTime(); }
	Time get()
	{
		auto curTime = getTime();
		if (curTime > _lastTime)
			return curTime - _lastTime;
		else
			return numeric_limits<Time>::max() - _lastTime + curTime + 1;
	}
};
