#pragma once

#include "config.h"
#include "basic_types.h"

using namespace std;

// Значение бита первого игрока.
const Player playerRed = 0x0;
// Значение бита второго игрока.
const Player playerBlack = 0x1;

// Получить по игроку следующего игрока.
inline Player nextPlayer(const Player player)
{
	return player ^ 1;
}
