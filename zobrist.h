#pragma once

#include "config.h"
#include "basic_types.h"
#include <boost/random.hpp>
#include <limits>

using namespace std;
using namespace boost;

// Хеш Зобриста.
class zobrist
{
private:
	// Размер таблицы хешей.
	size_t _size;
	// Хеши.
	Hash *_hashes;

public:
	// Конструктор.
	inline zobrist(size_t size, mt* gen)
	{
		random::uniform_int_distribution<Hash> dist(numeric_limits<Hash>::min(), numeric_limits<Hash>::max());
		_size = size;
		_hashes = new Hash[size];
		for (size_t i = 0; i < size; i++)
			_hashes[i] = dist(*gen);
	}
	// Конструктор копирования.
	inline zobrist(const zobrist &other)
	{
		_size = other._size;
		_hashes = new Hash[other._size];
		copy_n(other._hashes, other._size, _hashes);
	}
	// Деструктор.
	inline ~zobrist()
	{
		delete _hashes;
	}
	// Возвращает хеш Зобриста элемента num.
	inline Hash get_hash(size_t num)
	{
		return _hashes[num];
	}
};