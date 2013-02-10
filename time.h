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

inline ulong get_time()
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

class timer
{
private:
	ulong _last_time;

public:
	inline timer() { set(); }
	inline void set() { _last_time = get_time(); }
	inline ulong get()
	{
		ulong cur_time = get_time();
		if (cur_time > _last_time)
			return cur_time - _last_time;
		else
			return ULONG_MAX - _last_time + cur_time + 1;
	}
};
