#pragma once

#include "field.h"
#include "trajectory.h"
#include <list>
#include <vector>
#include <algorithm>

class Trajectories
{
private:
  int _depth[2];
  Field* _field;
  list<Trajectory> _trajectories[2];
  int* _trajectoriesBoard;
  Zobrist* _zobrist;
  list<int> _moves[2];
  list<int> _allMoves;

private:
  template<typename _InIt>
  void addTrajectory(_InIt begin, _InIt end, int player)
  {
    int64_t hash = 0;
    // Эвристические проверки.
    // Каждая точка траектории должна окружать что-либо и иметь рядом хотя бы 2 группы точек.
    // Если нет - не добавляем эту траекторию.
    for (auto i = begin; i < end; i++)
      if (!_field->isBaseBound(*i) || (_field->numberNearGroups(*i, player) < 2))
        return;
    // Высчитываем хеш траектории и сравниваем с уже существующими для исключения повторов.
    for (auto i = begin; i < end; i++)
      hash ^= _zobrist->getHash(*i);
    for (auto i = _trajectories[player].begin(); i != _trajectories[player].end(); i++)
      if (hash == i->getHash())
        return; // В теории возможны коллизии. Неплохо было бы сделать точную проверку.
    _trajectories[player].push_back(Trajectory(begin, end, _zobrist, hash));
  }
  void addTrajectory(Trajectory* trajectory, int player)
  {
    _trajectories[player].push_back(Trajectory(*trajectory));
  }
  void addTrajectory(Trajectory* trajectory, int pos, int player)
  {
    if (trajectory->size() == 1)
      return;
    _trajectories[player].push_back(Trajectory(_zobrist));
    for (auto i = trajectory->begin(); i != trajectory->end(); i++)
      if (*i != pos)
        _trajectories[player].back().pushBack(*i);
  }
  void buildTrajectoriesRecursive(int depth, int player)
  {
    for (auto pos = _field->minPos(); pos <= _field->maxPos(); pos++)
    {
      if (_field->puttingAllow(pos) && _field->isNearPoints(pos, player))
      {
        if (_field->isInEmptyBase(pos)) // Если поставили в пустую базу (свою или нет), то дальше строить траекторию нет нужды.
        {
          _field->doUnsafeStep(pos, player);
          if (_field->getDScore(player) > 0)
            addTrajectory(_field->getPointsSeq().end() - (_depth[player] - depth), _field->getPointsSeq().end(), player);
          _field->undoStep();
        }
        else
        {
          _field->doUnsafeStep(pos, player);

#if SUR_COND == 1
          if (_field->isBaseBound(pos) && _field->getDScore(player) == 0)
          {
            _field->undoStep();
            continue;
          }
#endif

          if (_field->getDScore(player) > 0)
            addTrajectory(_field->getPointsSeq().end() - (_depth[player] - depth), _field->getPointsSeq().end(), player);
          else if (depth > 0)
            buildTrajectoriesRecursive(depth - 1, player);

          _field->undoStep();
        }
      }
    }
  }
  void project(Trajectory* trajectory)
  {
    for (auto j = trajectory->begin(); j != trajectory->end(); j++)
      _trajectoriesBoard[*j]++;
  }
  // Проецирует траектории на доску TrajectoriesBoard (для каждой точки Pos очередной траектории инкрементирует TrajectoriesBoard[Pos]).
  // Для оптимизации в данной реализации функции не проверяются траектории на исключенность (поле Excluded).
  void project(int player)
  {
    for (auto i = _trajectories[player].begin(); i != _trajectories[player].end(); i++)
      if (!i->excluded())
        project(&(*i));
  }
  void project()
  {
    project(playerRed);
    project(playerBlack);
  }
  void unproject(Trajectory* trajectory)
  {
    for (auto j = trajectory->begin(); j != trajectory->end(); j++)
      _trajectoriesBoard[*j]--;
  }
  // Удаляет проекцию траекторий с доски TrajectoriesBoard.
  void unproject(int player)
  {
    for (auto i = _trajectories[player].begin(); i != _trajectories[player].end(); i++)
      if (!i->excluded())
        unproject(&(*i));
  }
  void unproject()
  {
    unproject(playerRed);
    unproject(playerBlack);
  }
  void includeAllTrajectories(int player)
  {
    for (auto i = _trajectories[player].begin(); i != _trajectories[player].end(); i++)
      i->include();
  }
  void includeAllTrajectories()
  {
    includeAllTrajectories(playerRed);
    includeAllTrajectories(playerBlack);
  }
  // Возвращает хеш Зобриста пересечения двух траекторий.
  int64_t getIntersectHash(Trajectory* t1, Trajectory* t2)
  {
    auto resultHash = t1->getHash();
    for (auto i = t2->begin(); i != t2->end(); i++)
      if (find(t1->begin(), t1->end(), *i) == t1->end())
        resultHash ^= _zobrist->getHash(*i);
    return resultHash;
  }
  bool excludeUnnecessaryTrajectories(int player)
  {
    auto need_exclude = false;
    for (auto i = _trajectories[player].begin(); i != _trajectories[player].end(); i++)
    {
      if (i->excluded())
        continue;
      // Считаем в c количество точек, входящих только в эту траекторию.
      auto c = 0;
      for (auto j = i->begin(); j != i->end(); j++)
        if (_trajectoriesBoard[*j] == 1)
          c++;
      // Если точек, входящих только в эту траекторию, > 1, то исключаем эту траекторию.
      if (c > 1)
      {
        need_exclude = true;
        i->exclude();
        unproject(&(*i));
      }
    }
    return need_exclude;
  }
  void excludeUnnecessaryTrajectories()
  {
    while (excludeUnnecessaryTrajectories(playerRed) || excludeUnnecessaryTrajectories(playerBlack));
  }
  // Исключает составные траектории.
  void excludeCompositeTrajectories(int player)
  {
    list<Trajectory>::iterator i, j, k;
    for (k = _trajectories[player].begin(); k != _trajectories[player].end(); k++)
      for (i = _trajectories[player].begin(); i != --_trajectories[player].end(); i++)
        if (k->size() > i->size())
          for (j = i, j++; j != _trajectories[player].end(); j++)
            if (k->size() > j->size() && k->getHash() == getIntersectHash(&(*i), &(*j)))
              k->exclude();
  }
  void excludeCompositeTrajectories()
  {
    excludeCompositeTrajectories(playerRed);
    excludeCompositeTrajectories(playerBlack);
  }
  void getPoints(list<int>* moves, int player)
  {
    for (auto i = _trajectories[player].begin(); i != _trajectories[player].end(); i++)
      if (!i->excluded())
        for (auto j = i->begin(); j != i->end(); j++)
          if (find(moves->begin(), moves->end(), *j) == moves->end())
            moves->push_back(*j);
  }
  int calculateMaxScore(int player, int depth)
  {
    auto result = _field->getScore(player);
    if (depth > 0)
    {
      for (auto i = _moves[player].begin(); i != _moves[player].end(); i++)
        if (_field->puttingAllow(*i))
        {
          _field->doUnsafeStep(*i, player);
          if (_field->getDScore(player) >= 0)
          {
            auto curScore = calculateMaxScore(player, depth - 1);
            if (curScore > result)
              result = curScore;
          }
          _field->undoStep();
        }
    }
    return result;
  }

public:
  Trajectories(Field* field, int* emptyBoard)
  {
    _field = field;
    _trajectoriesBoard = emptyBoard;
    _zobrist = &field->getZobrist();
  }
  int getCurPlayer()
  {
    return _field->getPlayer();
  }
  int getEnemyPlayer()
  {
    return nextPlayer(_field->getPlayer());
  }
  void clear(int player)
  {
    _trajectories[player].clear();
  }
  void clear()
  {
    clear(playerRed);
    clear(playerBlack);
  }
  void buildPlayerTrajectories(int player)
  {
    if (_depth[player] > 0)
      buildTrajectoriesRecursive(_depth[player] - 1, player);
  }
  void calculateMoves()
  {
    excludeCompositeTrajectories();
    // Проецируем неисключенные траектории на доску.
    project();
    // Исключаем те траектории, у которых имеется более одной точки, принадлежащей только ей.
    excludeUnnecessaryTrajectories();
    // Получаем список точек, входящих в оставшиеся неисключенные траектории.
    getPoints(&_moves[playerRed], playerRed);
    getPoints(&_moves[playerBlack], playerBlack);
    _allMoves.assign(_moves[playerRed].begin(), _moves[playerRed].end());
    for (auto i = _moves[playerBlack].begin(); i != _moves[playerBlack].end(); i++)
      if (find(_allMoves.begin(), _allMoves.end(), *i) == _allMoves.end())
        _allMoves.push_back(*i);
#if ALPHABETA_SORT
    _allMoves.sort([&](pos x, pos y){ return _trajectories_board[x] < _trajectories_board[y]; });
#endif
    // Очищаем доску от проекций.
    unproject();
    // После получения списка ходов обратно включаем в рассмотрение все траектории (для следующего уровня рекурсии).
    includeAllTrajectories();
  }
  void buildTrajectories(int depth)
  {
    _depth[getCurPlayer()] = (depth + 1) / 2;
    _depth[getEnemyPlayer()] = depth / 2;
    buildPlayerTrajectories(getCurPlayer());
    buildPlayerTrajectories(getEnemyPlayer());
    calculateMoves();
  }
  void buildTrajectories(Trajectories* last, int pos)
  {
    _depth[getCurPlayer()] = last->_depth[getCurPlayer()];
    _depth[getEnemyPlayer()] = last->_depth[getEnemyPlayer()] - 1;

    if (_depth[getCurPlayer()] > 0)
      buildTrajectoriesRecursive(_depth[getCurPlayer()] - 1, getCurPlayer());

    if (_depth[getEnemyPlayer()] > 0)
      for (auto i = last->_trajectories[getEnemyPlayer()].begin(); i != last->_trajectories[getEnemyPlayer()].end(); i++)
        if ((static_cast<int>(i->size()) <= _depth[getEnemyPlayer()] ||
            (static_cast<int>(i->size()) == _depth[getEnemyPlayer()] + 1 &&
              find(i->begin(), i->end(), pos) != i->end())) && i->isValid(_field, pos))
          addTrajectory(&(*i), pos, getEnemyPlayer());

    calculateMoves();
  }
  // Строит траектории с учетом предыдущих траекторий и того, что последний ход был сделан не на траектории (или не сделан вовсе).
  void buildTrajectories(Trajectories* last)
  {
    _depth[getCurPlayer()] = last->_depth[getCurPlayer()];
    _depth[getEnemyPlayer()] = last->_depth[getEnemyPlayer()] - 1;

    if (_depth[getCurPlayer()] > 0)
      for (auto i = last->_trajectories[getCurPlayer()].begin(); i != last->_trajectories[getCurPlayer()].end(); i++)
        addTrajectory(&(*i), getCurPlayer());

    if (_depth[getEnemyPlayer()] > 0)
      for (auto i = last->_trajectories[getEnemyPlayer()].begin(); i != last->_trajectories[getEnemyPlayer()].end(); i++)
        if (static_cast<int>(i->size()) <= _depth[getEnemyPlayer()])
          addTrajectory(&(*i), getEnemyPlayer());

    calculateMoves();
  }
  // Получить список ходов.
  list<int>* getPoints()
  {
    return &_allMoves;
  }
  int getMaxScore(int player)
  {
    return calculateMaxScore(player, _depth[player]) + _depth[nextPlayer(player)];
  }
};
