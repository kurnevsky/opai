#! /bin/bash
i486-mingw32-g++ -o opai.exe -mwindows -Wall -Wcast-qual -Wextra -Wshadow -pedantic -m32 -msse2 -flto -static -std=c++0x -fopenmp -Ofast -DWINDOWS -DENVIRONMENT_32 main.cpp bot.cpp field.cpp minimax.cpp position_estimate.cpp uct.cpp
