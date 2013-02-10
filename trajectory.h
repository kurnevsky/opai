#pragma once

#include "config.h"
#include "basic_types.h"
#include "zobrist.h"
#include <list>

class Trajectory
{
private:
	list<Pos> _points;
	zobrist *_zobrist;
	Hash _hash;
	bool _excluded;

public:
	inline Trajectory(zobrist* cur_zobrist) { _hash = 0; _excluded = false; _zobrist = cur_zobrist; }
	template<typename _InIt> inline Trajectory(_InIt first, _InIt last, zobrist* cur_zobrist) { _hash = 0; _excluded = false; _zobrist = cur_zobrist; assign(first, last); }
	template<typename _InIt> inline Trajectory(_InIt first, _InIt last, zobrist* cur_zobrist, size_t hash) { _hash = 0; _excluded = false; _zobrist = cur_zobrist; assign(first, last, hash); }
	inline Trajectory(const Trajectory &other) { _points = other._points; _hash = other._hash; _excluded = other._excluded; _zobrist = other._zobrist; }
	inline size_t size() const { return _points.size(); }
	inline bool empty() const { return _points.empty(); }
	inline void push_back(Pos pos) { _points.push_back(pos); _hash ^= _zobrist->get_hash(pos); }
	inline void clear() { _points.clear(); _hash = 0; _excluded = false; }
	inline const Trajectory& operator =(const Trajectory &other) { _points = other._points; _hash = other._hash; _excluded = other._excluded; _zobrist = other._zobrist; return *this; }
	inline void swap(Trajectory &other) { Trajectory tmp(*this); *this = other; other = tmp; }
	inline list<Pos>::iterator begin() { return _points.begin(); }
	inline list<Pos>::const_iterator begin() const { return _points.begin(); }
	inline list<Pos>::iterator end() { return _points.end(); }
	inline list<Pos>::const_iterator end() const { return _points.end(); }
	inline list<Pos>::reverse_iterator rbegin() { return _points.rbegin(); }
	inline list<Pos>::reverse_iterator rend() { return _points.rend(); }
	inline list<Pos>::const_reverse_iterator rbegin() const { return _points.rbegin(); }
	inline list<Pos>::const_reverse_iterator rend() const { return _points.rend(); }

	template<typename _InIt>
	inline void assign(_InIt first, _InIt last)
	{
		for (auto i = first; i != last; i++)
			push_back(*i, _zobrist->get_hash(*i));
	}
	template<typename _InIt>
	inline void assign(_InIt first, _InIt last, Hash hash)
	{
		_points.assign(first, last);
		_hash = hash;
	}

	inline Hash hash() const { return _hash; }
	inline void exclude() { _excluded = true; }
	inline void include() { _excluded = false; }
	inline bool excluded() const { return _excluded; }

	// Проверяет, во все ли точки траектории можно сделать ход.
	inline bool is_valid(Field* field) const
	{
		for (auto i = _points.begin(); i != _points.end(); i++)
			if (!field->putting_allow(*i))
				return false;
		return true;
	}
	// Проверяет, во все ли точки траектории можно сделать ход, кроме, возможно, точки cur_pos.
	inline bool is_valid(Field* field, Pos pos) const
	{
		for (auto i = _points.begin(); i != _points.end(); i++)
			if (*i != pos && !field->putting_allow(*i))
				return false;
		return true;
	}
};
