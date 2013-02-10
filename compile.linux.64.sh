#! /bin/bash
g++ -o opai -Wall -Wcast-qual -Wextra -Wshadow -pedantic -msse2 -flto -lrt -lboost_random -std=c++0x -fopenmp -Ofast -DLINUX -DENVIRONMENT_64 main.cpp bot.cpp field.cpp minimax.cpp position_estimate.cpp uct.cpp
