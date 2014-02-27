#pragma once

#include "config.h"
#include "basic_types.h"
#include <random>
#include <limits>
#include <algorithm>

using namespace std;

// Хеш Зобриста.
class Zobrist
{
private:
  // Размер таблицы хешей.
  Pos _size;
  // Хеши.
  Hash* _hashes;

public:
  // Конструктор.
  Zobrist(Pos size, mt* gen)
  {
    uniform_int_distribution<Hash> dist(numeric_limits<Hash>::min(), numeric_limits<Hash>::max());
    _size = size;
    _hashes = new Hash[size];
    for (Pos i = 0; i < size; i++)
      _hashes[i] = dist(*gen);
  }
  // Конструктор копирования.
  Zobrist(const Zobrist &other)
  {
    _size = other._size;
    _hashes = new Hash[other._size];
    copy_n(other._hashes, other._size, _hashes);
  }
  // Деструктор.
  ~Zobrist()
  {
    delete _hashes;
  }
  // Возвращает хеш Зобриста элемента pos.
  Hash getHash(Pos pos)
  {
    return _hashes[pos];
  }
};
