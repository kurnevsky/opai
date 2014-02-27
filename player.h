#pragma once

#include "basic_types.h"

using namespace std;

// Значение бита первого игрока.
const Player playerRed = 0;
// Значение бита второго игрока.
const Player playerBlack = 1;

// Получить по игроку следующего игрока.
inline Player nextPlayer(const Player player)
{
  return player ^ 1;
}
