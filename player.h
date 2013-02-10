#pragma once

#include "config.h"
#include "basic_types.h"

using namespace std;

// Значение бита первого игрока.
const Player player_red = 0x0;
// Значение бита второго игрока.
const Player player_black = 0x1;

// Получить по игроку следующего игрока.
inline Player next_player(const Player player)
{
	return player ^ 1;
}