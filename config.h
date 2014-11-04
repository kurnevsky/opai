#pragma once

// Правила обработки пустых баз.
// STANDART = 0 - если PlayerRed ставит в пустую базу и ничего не обводит, то PlayerBlack обводит эту территорию.
// ALWAYS = 1 - обводить базу, даже если нет вражеских точек внутри.
// ALWAYS_ENEMY = 2 - обводит всегда PlayerBlack, если PlayerRed поставил точку в пустую базу.
#define SUR_COND 0

// Включает сортировку по вероятностям для улучшения альфабета-отсечения.
#define ALPHABETA_SORT 0

#define UCT_DEPTH 6

#define UCT_WHEN_CREATE_CHILDREN 2

// Larger values give uniform search.
// Smaller values give very selective search.
#define UCTK 1

// Must be double!
#define UCT_DRAW_WEIGHT 0.4

// Радиус, внутри которого происходит анализ UCT.
#define UCT_RADIUS 3

// 0 - UCB1
// 1 - UCT1-tuned
#define UCB_TYPE 1

// Способ поиска лучшего хода.
// 0 - position estimate
// 1 - minimax
// 2 - uct
// 3 - minimax with uct
// 4 - MTD(f)
// 5 - MTD(f) with uct
#define SEARCH_TYPE 3
#define SEARCH_WITH_COMPLEXITY_TYPE 3
#define SEARCH_WITH_TIME_TYPE 2

#define MIN_MINIMAX_DEPTH 0
#define MAX_MINIMAX_DEPTH 10
#define DEFAULT_MINIMAX_DEPTH 8
#define MIN_MTDF_DEPTH 0
#define MAX_MTDF_DEPTH 10
#define DEFAULT_MTDF_DEPTH 8
#define MIN_UCT_ITERATIONS 0
#define MAX_UCT_ITERATIONS 250000
#define DEFAULT_UCT_ITERATIONS 200000
#define MIN_COMPLEXITY 0
#define MAX_COMPLEXITY 100
