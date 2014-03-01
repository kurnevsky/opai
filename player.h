#pragma once

using namespace std;

// Значение бита первого игрока.
const int playerRed = 0;
// Значение бита второго игрока.
const int playerBlack = 1;

// Получить по игроку следующего игрока.
inline int nextPlayer(const int player)
{
  return player ^ 1;
}
