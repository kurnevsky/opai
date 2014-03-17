#pragma once

#include "zobrist.h"
#include <list>

class Trajectory
{
private:
  list<int> _points;
  Zobrist* _zobrist;
  int64_t _hash;
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
  Trajectory(_InIt first, _InIt last, Zobrist* zobrist, int64_t hash)
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
  void pushBack(int pos)
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
  list<int>::iterator begin()
  {
    return _points.begin();
  }
  list<int>::const_iterator begin() const
  {
    return _points.begin();
  }
  list<int>::iterator end()
  {
    return _points.end();
  }
  list<int>::const_iterator end() const
  {
    return _points.end();
  }
  list<int>::reverse_iterator rbegin()
  {
    return _points.rbegin();
  }
  list<int>::reverse_iterator rend()
  {
    return _points.rend();
  }
  list<int>::const_reverse_iterator rbegin() const
  {
    return _points.rbegin();
  }
  list<int>::const_reverse_iterator rend() const
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
  void assign(_InIt first, _InIt last, int64_t hash)
  {
    _points.assign(first, last);
    _hash = hash;
  }
  int64_t getHash() const
  {
    return _hash;
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
  bool isValid(Field* field, int pos) const
  {
    for (auto i = _points.begin(); i != _points.end(); i++)
      if (*i != pos && !field->puttingAllow(*i))
        return false;
    return true;
  }
};
