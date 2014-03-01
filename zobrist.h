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
  int64_t* _hashes;

public:
  // Конструктор.
  Zobrist(Pos size, mt19937_64* gen)
  {
    uniform_int_distribution<int64_t> dist(numeric_limits<int64_t>::min(), numeric_limits<int64_t>::max());
    _size = size;
    _hashes = new int64_t[size];
    for (Pos i = 0; i < size; i++)
      _hashes[i] = dist(*gen);
  }
  // Конструктор копирования.
  Zobrist(const Zobrist &other)
  {
    _size = other._size;
    _hashes = new int64_t[other._size];
    copy_n(other._hashes, other._size, _hashes);
  }
  // Деструктор.
  ~Zobrist()
  {
    delete _hashes;
  }
  // Возвращает хеш Зобриста элемента pos.
  int64_t getHash(Pos pos)
  {
    return _hashes[pos];
  }
};
