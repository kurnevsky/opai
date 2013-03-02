#pragma once

#include "config.h"
#include "basic_types.h"
#include "zobrist.h"
#include <list>

class Trajectory
{
private:
	list<Pos> _points;
	Zobrist* _zobrist;
	Hash _hash;
	bool _excluded;

public:
	Trajectory(Zobrist* zobrist)
	{
		_hash = 0;
		_excluded = false;
		_zobrist = zobrist;
	}
	template<typename _InIt>
	Trajectory(_InIt first, _InIt last, Zobrist* zobrist)
	{
		_hash = 0;
		_excluded = false;
		_zobrist = zobrist;
		assign(first, last);
	}
	template<typename _InIt>
	Trajectory(_InIt first, _InIt last, Zobrist* zobrist, Hash hash)
	{
		_excluded = false;
		_zobrist = zobrist;
		assign(first, last, hash);
	}
	Trajectory(const Trajectory &other)
	{
		_points = other._points;
		_hash = other._hash;
		_excluded = other._excluded;
		_zobrist = other._zobrist;
	}
	size_t size() const
	{
		return _points.size();
	}
	bool empty() const
	{
		return _points.empty();
	}
	void pushBack(Pos pos)
	{
		_points.push_back(pos);
		_hash ^= _zobrist->getHash(pos);
	}
	void clear()
	{
		_points.clear();
		_hash = 0;
		_excluded = false;
	}
	const Trajectory& operator =(const Trajectory &other)
	{
		_points = other._points;
		_hash = other._hash;
		_excluded = other._excluded;
		_zobrist = other._zobrist;
		return *this;
	}
	void swap(Trajectory &other)
	{
		Trajectory tmp(*this);
		*this = other;
		other = tmp;
	}
	list<Pos>::iterator begin()
	{
		return _points.begin();
	}
	list<Pos>::const_iterator begin() const
	{
		return _points.begin();
	}
	list<Pos>::iterator end()
	{
		return _points.end();
	}
	list<Pos>::const_iterator end() const
	{
		return _points.end();
	}
	list<Pos>::reverse_iterator rbegin()
	{
		return _points.rbegin();
	}
	list<Pos>::reverse_iterator rend()
	{
		return _points.rend();
	}
	list<Pos>::const_reverse_iterator rbegin() const
	{
		return _points.rbegin();
	}
	list<Pos>::const_reverse_iterator rend() const
	{
		return _points.rend();
	}
	template<typename _InIt>
	void assign(_InIt first, _InIt last)
	{
		for (auto i = first; i != last; i++)
			push_back(*i, _zobrist->getHash(*i));
	}
	template<typename _InIt>
	void assign(_InIt first, _InIt last, Hash hash)
	{
		_points.assign(first, last);
		_hash = hash;
	}
	Hash hash() const
	{
		return
		_hash;
	}
	void exclude()
	{
		_excluded = true;
	}
	void include()
	{
		_excluded = false;
	}
	bool excluded() const
	{
		return _excluded;
	}
	// Проверяет, во все ли точки траектории можно сделать ход.
	bool isValid(Field* field) const
	{
		for (auto i = _points.begin(); i != _points.end(); i++)
			if (!field->puttingAllow(*i))
				return false;
		return true;
	}
	// Проверяет, во все ли точки траектории можно сделать ход, кроме, возможно, точки cur_pos.
	bool isValid(Field* field, Pos pos) const
	{
		for (auto i = _points.begin(); i != _points.end(); i++)
			if (*i != pos && !field->puttingAllow(*i))
				return false;
		return true;
	}
};
