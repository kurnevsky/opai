#pragma once

#include <random>
#include <limits>
#include <algorithm>

using namespace std;

// Class representing Zobrist hash.
class Zobrist
{
private:
  // Size of hash table.
  int _size;
  // Hash table
  int64_t* _hashes;
public:
  // Constructor.
  Zobrist(const int size, mt19937_64* gen)
  {
    uniform_int_distribution<int64_t> dist(numeric_limits<int64_t>::min(), numeric_limits<int64_t>::max());
    _size = size;
    _hashes = new int64_t[size];
    for (int i = 0; i < size; i++)
      _hashes[i] = dist(*gen);
  }
  // Copy constructor.
  Zobrist(const Zobrist &other)
  {
    _size = other._size;
    _hashes = new int64_t[other._size];
    copy_n(other._hashes, other._size, _hashes);
  }
  // Destructor.
  ~Zobrist()
  {
    delete _hashes;
  }
  // Get hash by number.
  int64_t getHash(const int pos) const
  {
    return _hashes[pos];
  }
};
