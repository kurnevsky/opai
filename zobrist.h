#pragma once

#include "config.h"
#include "basic_types.h"
#include <boost/random.hpp>
#include <limits>

using namespace std;
using namespace boost;

// Хеш Зобриста.
class Zobrist
{
private:
	// Размер таблицы хешей.
	int _size;
	// Хеши.
	Hash* _hashes;

public:
	// Конструктор.
	Zobrist(int size, mt* gen)
	{
		random::uniform_int_distribution<Hash> dist(numeric_limits<Hash>::min(), numeric_limits<Hash>::max());
		_size = size;
		_hashes = new Hash[size];
		for (auto i = 0; i < size; i++)
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
	// Возвращает хеш Зобриста элемента num.
	Hash getHash(size_t num)
	{
		return _hashes[num];
	}
};
