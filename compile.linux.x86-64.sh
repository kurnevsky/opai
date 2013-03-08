#! /bin/bash
g++ -o opai-linux-x86-64 -Wall -Wcast-qual -Wextra -Wshadow -pedantic -msse2 -flto -lrt -lboost_random -std=c++0x -fopenmp -Ofast -DLINUX -DENVIRONMENT_64 main.cpp bot.cpp field.cpp minimax.cpp position_estimate.cpp uct.cpp mtdf.cpp
