#pragma once

#include "config.h"
#include "basic_types.h"
#include "player.h"
#include "zobrist.h"
#include <list>
#include <vector>
#include <algorithm>
#include <functional>
#include <utility>

using namespace std;

class Field
{
private:

  /** Constants **/

  /* State bits and masks */

  // Бит, указывающий номер игрока.
  static const int playerBit = 1;
  // Бит, указывающий на наличие точки в поле.
  static const int putBit = 2;
  // Бит, указывающий на захваченность точки.
  static const int surBit = 4;
  // Бит, указывающий на то, захватывает ли что-нибудь точка на поле.
  static const int boundBit = 8;
  // Бит, указывающий на пустую базу.
  static const int emptyBaseBit = 16;
  // Бит для временных пометок полей.
  static const int tagBit = 32;
  // Бит, которым помечаются границы поля.
  static const int badBit = 64;
  static const int enableMask = badBit | surBit | putBit | playerBit;
  static const int boundMask = enableMask | boundBit;

  /** Fields **/

  vector<BoardChange> _changes;
  // Main points array (game board).
  // Основной массив точек (игровая доска).
  int* _points;
  // Real field width.
  // Действительная ширина поля.
  int _width;
  // Real field height.
  // Действительная высота поля.
  int _height;
  // Current player color.
  // Текущий цвет игроков.
  int _player;
  // Capture points count.
  // Количество захваченных точек.
  int _captureCount[2];
  Zobrist* _zobrist;
  int64_t _hash;
  // History points sequance.
  // Последовательность поставленных точек.
  vector<int> _pointsSeq;

  /** Private methods **/

  /* Set state functions */

  // Пометить поле по координате как содержащее точку.
  void setPutted(const int pos)
  {
    _points[pos] |= putBit;
  }
  // Убрать с поля по координате put_bit.
  void clearPutted(const int pos)
  {
    _points[pos] &= ~putBit;
  }
  // Пометить поле по координате как принадлежащее игроку.
  void setPlayer(const int pos, const int player)
  {
    _points[pos] = (_points[pos] & ~playerBit) | player;
  }
  // Пометить поле по координате как содержащее точку игрока.
  void setPlayerPutted(const int pos, const int player)
  {
    _points[pos] = (_points[pos] & ~playerBit) | player | putBit;
  }
  // Пометить битом SurBit (захватить).
  void setCaptured(const int pos)
  {
    _points[pos] |= surBit;
  }
  // Убрать бит SurBit.
  void clearCaptured(const int pos)
  {
    _points[pos] &= ~surBit;
  }
  // Пометить точку как окружающую базу.
  void setBound(const int pos)
  {
    _points[pos] |= boundBit;
  }
  // Пометить точку как не окружающую базу.
  void clearBound(const int pos)
  {
    _points[pos] &= ~boundBit;
  }
  void setEmptyBase(const int pos)
  {
    _points[pos] |= emptyBaseBit;
  }
  void clearEmptyBase(const int pos)
  {
    _points[pos] &= ~emptyBaseBit;
  }
  void setBad(const int pos)
  {
    _points[pos] |= badBit;
  }
  void clearBad(const int pos)
  {
    _points[pos] &= ~badBit;
  }

  /* Other */

  // Skew product of vectors pos1 and pos2.
  int square(const int pos1, const int pos2) const
  {
    return toX(pos1) * toY(pos2) - toY(pos1) * toX(pos2);
  }
  //  * . .   x . *   . x x   . . .
  //  . o .   x o .   . o .   . o x
  //  x x .   . . .   . . *   * . x
  //  o - center pos
  //  x - pos
  //  * - result
  int getFirstNextPos(const int centerPos, const int pos) const
  {
    if (pos < centerPos)
    {
      if ((pos == nw(centerPos)) || (pos == centerPos - 1))
        return ne(centerPos);
      else
        return se(centerPos);
    }
    else
    {
      if ((pos == centerPos + 1) || (pos == se(centerPos)))
        return sw(centerPos);
      else
        return nw(centerPos);
    }
  }
  //  . . .   * . .   x * .   . x *   . . x   . . .   . . .   . . .
  //  * o .   x o .   . o .   . o .   . o *   . o x   . o .   . o .
  //  x . .   . . .   . . .   . . .   . . .   . . *   . * x   * x .
  //  o - center pos
  //  x - pos
  //  * - result
  int getNextPos(const int centerPos, const int pos) const
  {
    if (pos < centerPos)
    {
      if (pos == nw(centerPos))
        return n(centerPos);
      else if (pos == n(centerPos))
        return ne(centerPos);
      else if (pos == ne(centerPos))
        return e(centerPos);
      else
        return nw(centerPos);
    }
    else
    {
      if (pos == e(centerPos))
        return se(centerPos);
      else if (pos == se(centerPos))
        return s(centerPos);
      else if (pos == s(centerPos))
        return sw(centerPos);
      else
        return w(centerPos);
    }
  }
  // Возвращает количество групп точек рядом с CenterPos.
  // InpChainPoints - возможные точки цикла, InpSurPoints - возможные окруженные точки.
  int getInputPoints(const int centerPos, const int enableCond, int inpChainPoints[], int inpSurPoints[]) const
  {
    int result = 0;
    if (isNotEnable(w(centerPos), enableCond))
    {
      if (isEnable(nw(centerPos), enableCond))
      {
        inpChainPoints[0] = nw(centerPos);
        inpSurPoints[0] = w(centerPos);
        result++;
      }
      else if (isEnable(n(centerPos), enableCond))
      {
        inpChainPoints[0] = n(centerPos);
        inpSurPoints[0] = w(centerPos);
        result++;
      }
    }
    if (isNotEnable(s(centerPos), enableCond))
    {
      if (isEnable(sw(centerPos), enableCond))
      {
        inpChainPoints[result] = sw(centerPos);
        inpSurPoints[result] = s(centerPos);
        result++;
      }
      else if (isEnable(w(centerPos), enableCond))
      {
        inpChainPoints[result] = w(centerPos);
        inpSurPoints[result] = s(centerPos);
        result++;
      }
    }
    if (isNotEnable(e(centerPos), enableCond))
    {
      if (isEnable(se(centerPos), enableCond))
      {
        inpChainPoints[result] = se(centerPos);
        inpSurPoints[result] = e(centerPos);
        result++;
      }
      else if (isEnable(s(centerPos), enableCond))
      {
        inpChainPoints[result] = s(centerPos);
        inpSurPoints[result] = e(centerPos);
        result++;
      }
    }
    if (isNotEnable(n(centerPos), enableCond))
    {
      if (isEnable(ne(centerPos), enableCond))
      {
        inpChainPoints[result] = ne(centerPos);
        inpSurPoints[result] = n(centerPos);
        result++;
      }
      else if (isEnable(e(centerPos), enableCond))
      {
        inpChainPoints[result] = e(centerPos);
        inpSurPoints[result] = n(centerPos);
        result++;
      }
    }
    return result;
  }
  IntersectionState getIntersectionState(const int pos, const int nextPos) const
  {
    Point a, b;
    toXY(pos, a.x, a.y);
    toXY(nextPos, b.x, b.y);
    if (b.x <= a.x)
      switch (b.y - a.y)
      {
        case (1):
          return INTERSECTION_STATE_UP;
        case (0):
          return INTERSECTION_STATE_TARGET;
        case (-1):
          return INTERSECTION_STATE_DOWN;
        default:
          return INTERSECTION_STATE_NONE;
      }
    else
      return INTERSECTION_STATE_NONE;
  }
  // Поставить начальные точки.
  void placeBeginPattern(BeginPattern pattern)
  {
    switch (pattern)
    {
    case (BEGIN_PATTERN_CROSSWIRE):
      doStep(toPos(_width / 2 - 1, _height / 2 - 1));
      doStep(toPos(_width / 2, _height / 2 - 1));
      doStep(toPos(_width / 2, _height / 2));
      doStep(toPos(_width / 2 - 1, _height / 2));
      break;
    case (BEGIN_PATTERN_SQUARE):
      doStep(toPos(_width / 2 - 1, _height / 2 - 1));
      doStep(toPos(_width / 2, _height / 2 - 1));
      doStep(toPos(_width / 2 - 1, _height / 2));
      doStep(toPos(_width / 2, _height / 2));
      break;
    case (BEGIN_PATTERN_CLEAN):
      break;
    }
  }
  void updateHash(int pos, int player)
  {
    if (player == 0)
      _hash ^= _zobrist->getHash(pos);
    else
      _hash ^= _zobrist->getHash(getLength() + pos);
  }
  // Удаляет пометку пустой базы с поля точек, начиная с позиции StartPos.
  void removeEmptyBase(const int startPos)
  {
    wave(startPos, [&](int pos)->bool
    {
      if (isInEmptyBase(pos))
      {
        _changes.back().changes.emplace(pos, _points[pos]);
        clearEmptyBase(pos);
        return true;
      }
      else
      {
        return false;
      }
    });
  }
  bool buildChain(const int startPos, const int enableCond, const int directionPos, list<int> &chain)
  {
    chain.clear();
    chain.push_back(startPos);
    int pos = directionPos;
    int centerPos = startPos;
    // Площадь базы.
    int baseSquare = square(centerPos, pos);
    do
    {
      if (isTagged(pos))
      {
        while (chain.back() != pos)
        {
          clearTag(chain.back());
          chain.pop_back();
        }
      }
      else
      {
        setTag(pos);
        chain.push_back(pos);
      }
      swap(pos, centerPos);
      pos = getFirstNextPos(centerPos, pos);
      while (isNotEnable(pos, enableCond))
        pos = getNextPos(centerPos, pos);
      baseSquare += square(centerPos, pos);
    }
    while (pos != startPos);
    for (auto i = chain.begin(); i != chain.end(); i++)
      clearTag(*i);
    return (baseSquare < 0 && chain.size() > 2);
  }
  void findSurround(list<int> &chain, int insidePoint, int player)
  {
    // Количество захваченных точек.
    int curCaptureCount = 0; //captured
    // Количество захваченных пустых полей.
    int curFreedCount = 0;
    list<int> surPoints;
    // Помечаем точки цепочки.
    for (auto i = chain.begin(); i != chain.end(); i++)
      setTag(*i);
    wave(insidePoint, [&, player](int pos)->bool
    {
      if (isNotBound(pos, player | putBit | boundBit))
      {
        if (isPutted(pos))
        {
          if (getPlayer(pos) != player)
            curCaptureCount++;
          else if (isCaptured(pos))
            curFreedCount++;
        }
        surPoints.push_back(pos);
        return true;
      }
      else
      {
        return false;
      }
    });
    // Изменение счета игроков.
    _captureCount[player] += curCaptureCount;
    _captureCount[nextPlayer(player)] -= curFreedCount;
#if SUR_COND != 1
    if (curCaptureCount != 0) // Если захватили точки.
#endif
    {
      for (auto i = chain.begin(); i != chain.end(); i++)
      {
        int pos = *i;
        clearTag(pos);
        // Добавляем в список изменений точки цепочки.
        _changes.back().changes.emplace(pos, _points[pos]);
        // Помечаем точки цепочки.
        setBound(pos);
      }
      for (auto i = surPoints.begin(); i != surPoints.end(); i++)
      {
        int pos = *i;
        _changes.back().changes.emplace(pos, _points[pos]);
        if (!isPutted(pos))
        {
          if (!isCaptured(pos))
            setCaptured(pos);
          else
            updateHash(pos, nextPlayer(player));
          setPlayer(pos, player);
          updateHash(pos, player);
        }
        else
        {
          if (getPlayer(pos) != player)
          {
            setCaptured(pos);
            updateHash(pos, nextPlayer(player));
            updateHash(pos, player);
          }
          else if (isCaptured(pos))
          {
            clearCaptured(pos);
            updateHash(pos, nextPlayer(player));
            updateHash(pos, player);
          }
        }
      }
    }
    else // Если ничего не захватили.
    {
      for (auto i = chain.begin(); i != chain.end(); i++)
        clearTag(*i);
      for (auto i = surPoints.begin(); i != surPoints.end(); i++)
      {
        int pos = *i;
        _changes.back().changes.emplace(pos, _points[pos]);
        if (!isPutted(pos))
        {
          setEmptyBase(pos);
          setPlayer(pos, player);
        }
      }
    }
  }
  // Проверяет поставленную точку на наличие созданных ею окружений, и окружает, если они есть.
  void checkClosure(const int startPos, const int player)
  {
    int inpPointsCount;
    int inpChainPoints[4], inpSurPoints[4];
    list<int> chain;
    if (isInEmptyBase(startPos)) // Если точка поставлена в пустую базу.
    {
      if (getPlayer(startPos - 1) == getPlayer(startPos)) // Если поставили в свою пустую базу.
      {
        clearEmptyBase(startPos);
        return;
      }
#if SUR_COND != 2 // Если приоритет не всегда у врага.
      inpPointsCount = getInputPoints(startPos, player | putBit, inpChainPoints, inpSurPoints);
      if (inpPointsCount > 1)
      {
        int chainsCount = 0;
        for (int i = 0; i < inpPointsCount; i++)
          if (buildChain(startPos, getPlayer(startPos) | putBit, inpChainPoints[i], chain))
          {
            findSurround(chain, inpSurPoints[i], player);
            chainsCount++;
            if (chainsCount == inpPointsCount - 1)
              break;
          }
        if (chainsCount > 0)
        {
          removeEmptyBase(startPos);
          return;
        }
      }
#endif
      int pos = startPos;
      do
      {
        pos--;
        while (!isEnable(pos, nextPlayer(player) | putBit))
          pos--;
        inpPointsCount = getInputPoints(pos, nextPlayer(player) | putBit, inpChainPoints, inpSurPoints);
        for (int i = 0; i < inpPointsCount; i++)
          if (buildChain(pos, nextPlayer(player) | putBit, inpChainPoints[i], chain))
            if (isPointInsideRing(startPos, chain))
            {
              findSurround(chain, inpSurPoints[i], nextPlayer(player));
              break;
            }
      } while (!isCaptured(startPos));
    }
    else
    {
      inpPointsCount = getInputPoints(startPos, player | putBit, inpChainPoints, inpSurPoints);
      if (inpPointsCount > 1)
      {
        int chainsCount = 0;
        for (int i = 0; i < inpPointsCount; i++)
          if (buildChain(startPos, player | putBit, inpChainPoints[i], chain))
          {
            findSurround(chain, inpSurPoints[i], player);
            chainsCount++;
            if (chainsCount == inpPointsCount - 1)
              break;
          }
      }
    }
  }

public:

  /** Public methods **/

  /* Constructors and destructor */

  Field(const int width, const int height, const BeginPattern begin_pattern, Zobrist* zobrist)
  {
    _width = width;
    _height = height;
    _player = playerRed;
    _captureCount[playerRed] = 0;
    _captureCount[playerBlack] = 0;
    _points = new int[getLength()];
    fill_n(_points, getLength(), 0);
    for (int x = -1; x <= width; x++)
    {
      setBad(toPos(x, -1));
      setBad(toPos(x, height));
    }
    for (int y = -1; y <= height; y++)
    {
      setBad(toPos(-1, y));
      setBad(toPos(width, y));
    }
    _changes.reserve(getLength());
    _pointsSeq.reserve(getLength());
    _zobrist = zobrist;
    _hash = 0;
    placeBeginPattern(begin_pattern);
  }
  Field(const Field &orig)
  {
    _width = orig._width;
    _height = orig._height;
    _player = orig._player;
    _captureCount[playerRed] = orig._captureCount[playerRed];
    _captureCount[playerBlack] = orig._captureCount[playerBlack];
    _points = new int[getLength()];
    copy_n(orig._points, getLength(), _points);
    _changes.reserve(getLength());
    _pointsSeq.reserve(getLength());
    _changes.assign(orig._changes.begin(), orig._changes.end());
    _pointsSeq.assign(orig._pointsSeq.begin(), orig._pointsSeq.end());
    _zobrist = orig._zobrist;
    _hash = orig._hash;
  }
  ~Field()
  {
    delete _points;
  }

  /* Set state functions */

  // Установить бит TagBit.
  void setTag(const int pos)
  {
    _points[pos] |= tagBit;
  }
  // Убрать бит TagBit.
  void clearTag(const int pos)
  {
    _points[pos] &= ~tagBit;
  }

  /* Get state functions */

  // Получить по координате игрока, чья точка там поставлена.
  int getPlayer(const int pos) const
  {
    return _points[pos] & playerBit;
  }
  // Проверить по координате, поставлена ли там точка.
  bool isPutted(const int pos) const
  {
    return (_points[pos] & putBit) != 0;
  }
  // Прверить по координате, является ли точка окружающей базу.
  bool isBound(const int pos) const
  {
    return (_points[pos] & boundBit) != 0;
  }
  // Проверить по координате, захвачено ли поле.
  bool isCaptured(const int pos) const
  {
    return (_points[pos] & surBit) != 0;
  }
  // Проверить по координате, лежит ли она в пустой базе.
  bool isInEmptyBase(const int pos) const
  {
    return (_points[pos] & emptyBaseBit) != 0;
  }
  // Проверить по координате, помечено ли поле.
  bool isTagged(const int pos) const
  {
    return (_points[pos] & tagBit) != 0;
  }
  // Проверка незанятости поля по условию.
  bool isEnable(const int pos, const int enableCond) const
  {
    return (_points[pos] & enableMask) == enableCond;
  }
  // Проверка занятости поля по условию.
  bool isNotEnable(const int pos, const int enableCond) const
  {
    return (_points[pos] & enableMask) != enableCond;
  }
  // Проверка на то, захвачено ли поле.
  bool isBound(const int pos, const int boundCond) const
  {
    return (_points[pos] & boundMask) == boundCond;
  }
  // Проверка на то, не захвачено ли поле.
  bool isNotBound(const int pos, const int boundCond) const
  {
    return (_points[pos] & boundMask) != boundCond;
  }
  // Провека на то, возможно ли поставить точку в полке.
  bool isPuttingAllowed(const int pos) const
  {
    return (_points[pos] & (putBit | surBit | badBit)) == 0;
  }

  /** Getters **/

  int getMovesCount() const
  {
    return _pointsSeq.size();
  }
  const vector<int>& getPointsSeq() const
  {
    return _pointsSeq;
  }
  int getScore(int player) const
  {
    return _captureCount[player] - _captureCount[nextPlayer(player)];
  }
  int getPrevScore(int player) const
  {
    const BoardChange& lastChange = _changes.back();
    return lastChange.captureCount[player] - lastChange.captureCount[nextPlayer(player)];
  }
  int getLastPlayer() const
  {
    return getPlayer(_pointsSeq.back());
  }
  int getDeltaScore(int player) const
  {
    return getScore(player) - getPrevScore(player);
  }
  int getDeltaScore() const
  {
    return getDeltaScore(getLastPlayer());
  }
  int getPlayer() const
  {
    return _player;
  }
  int64_t getHash() const
  {
    return _hash;
  }
  int getWidth() const
  {
    return _width;
  }
  int getHeight() const
  {
    return _height;
  }
  int getLength() const
  {
    return (_width + 2) * (_height + 2);
  }
  Zobrist& getZobrist() const
  {
    return *_zobrist;
  };
  int minPos() const
  {
    return toPos(0, 0);
  }
  int maxPos() const
  {
    return toPos(_width - 1, _height - 1);
  }

  /** Position functions **/

  int n(const int pos) const
  {
    return pos - (_width + 2);
  }
  int s(const int pos) const
  {
    return pos + (_width + 2);
  }
  int w(const int pos) const
  {
    return pos - 1;
  }
  int e(const int pos) const
  {
    return pos + 1;
  }
  int nw(const int pos) const
  {
    return pos - (_width + 2) - 1;
  }
  int ne(const int pos) const
  {
    return pos - (_width + 2) + 1;
  }
  int sw(const int pos) const
  {
    return pos + (_width + 2) - 1;
  }
  int se(const int pos) const
  {
    return pos + (_width + 2) + 1;
  }
  int toPos(const int x, const int y) const
  {
    return (y + 1) * (_width + 2) + x + 1;
  }
  int toX(const int pos) const
  {
    return pos % (_width + 2) - 1;
  }
  int toY(const int pos) const
  {
    return pos / (_width + 2) - 1;
  }
  // Конвертация из Pos в XY.
  void toXY(const int pos, int &x, int &y) const
  {
    x = toX(pos);
    y = toY(pos);
  }

  /** Other **/

  // Проверяет, находятся ли две точки рядом.
  bool isNear(const int pos1, const int pos2) const
  {
    if (n(pos1) == pos2  ||
        s(pos1) == pos2  ||
        w(pos1) == pos2  ||
        e(pos1) == pos2  ||
        nw(pos1) == pos2 ||
        ne(pos1) == pos2 ||
        sw(pos1) == pos2 ||
        se(pos1) == pos2)
      return true;
    else
      return false;
  }
  // Проверяет, есть ли рядом с centerPos точки цвета player.
  bool isNearPoints(const int centerPos, const int player) const
  {
    if (isEnable(n(centerPos), putBit | player)  ||
        isEnable(s(centerPos), putBit | player)  ||
        isEnable(w(centerPos), putBit | player)  ||
        isEnable(e(centerPos), putBit | player)  ||
        isEnable(nw(centerPos), putBit | player) ||
        isEnable(ne(centerPos), putBit | player) ||
        isEnable(sw(centerPos), putBit | player) ||
        isEnable(se(centerPos), putBit | player))
      return true;
    else
      return false;
  }
  // Возвращает количество точек рядом с centerPos цвета player.
  int numberNearPoints(const int centerPos, const int player) const
  {
    int result = 0;
    if (isEnable(n(centerPos), putBit | player))
      result++;
    if (isEnable(s(centerPos), putBit | player))
      result++;
    if (isEnable(w(centerPos), putBit | player))
      result++;
    if (isEnable(e(centerPos), putBit | player))
      result++;
    if (isEnable(nw(centerPos), putBit | player))
      result++;
    if (isEnable(ne(centerPos), putBit | player))
      result++;
    if (isEnable(sw(centerPos), putBit | player))
      result++;
    if (isEnable(se(centerPos), putBit | player))
      result++;
    return result;
  }
  // Возвращает количество групп точек рядом с centerPos.
  int numberNearGroups(const int centerPos, const int player) const
  {
    int result = 0;
    if (isNotEnable(w(centerPos), player | putBit) && (isEnable(nw(centerPos), player | putBit) || isEnable(n(centerPos), player | putBit)))
      result++;
    if (isNotEnable(s(centerPos), player | putBit) && (isEnable(sw(centerPos), player | putBit) || isEnable(w(centerPos), player | putBit)))
      result++;
    if (isNotEnable(e(centerPos), player | putBit) && (isEnable(se(centerPos), player | putBit) || isEnable(s(centerPos), player | putBit)))
      result++;
    if (isNotEnable(n(centerPos), player | putBit) && (isEnable(ne(centerPos), player | putBit) || isEnable(e(centerPos), player | putBit)))
      result++;
    return result;
  }
  bool isPointInsideRing(const int pos, const list<int> &ring) const
  {
    int intersections = 0;
    IntersectionState state = INTERSECTION_STATE_NONE;
    for (auto i = ring.begin(); i != ring.end(); i++)
    {
      switch (getIntersectionState(pos, *i))
      {
      case (INTERSECTION_STATE_NONE):
        state = INTERSECTION_STATE_NONE;
        break;
      case (INTERSECTION_STATE_UP):
        if (state == INTERSECTION_STATE_DOWN)
          intersections++;
        state = INTERSECTION_STATE_UP;
        break;
      case (INTERSECTION_STATE_DOWN):
        if (state == INTERSECTION_STATE_UP)
          intersections++;
        state = INTERSECTION_STATE_DOWN;
        break;
      case (INTERSECTION_STATE_TARGET):
        break;
      }
    }
    if (state == INTERSECTION_STATE_UP || state == INTERSECTION_STATE_DOWN)
    {
      auto i = ring.begin();
      IntersectionState beginState = getIntersectionState(pos, *i);
      while (beginState == INTERSECTION_STATE_TARGET)
      {
        i++;
        beginState = getIntersectionState(pos, *i);
      }
      if ((state == INTERSECTION_STATE_UP && beginState == INTERSECTION_STATE_DOWN) ||
          (state == INTERSECTION_STATE_DOWN && beginState == INTERSECTION_STATE_UP))
        intersections++;
    }
    return intersections % 2 == 1;
  }
  void wave(const int startPos, const function<bool(int)> cond)
  {
    // Очередь для волнового алгоритма (обхода в ширину).
    list<int> q;
    if (!cond(startPos))
      return;
    q.push_back(startPos);
    setTag(startPos);
    auto it = q.begin();
    while (it != q.end())
    {
      int wPos = w(*it);
      if (!isTagged(wPos) && cond(wPos))
      {
        q.push_back(wPos);
        setTag(wPos);
      }
      int nPos = n(*it);
      if (!isTagged(nPos) && cond(nPos))
      {
        q.push_back(nPos);
        setTag(nPos);
      }
      int ePos = e(*it);
      if (!isTagged(ePos) && cond(ePos))
      {
        q.push_back(ePos);
        setTag(ePos);
      }
      int sPos = s(*it);
      if (!isTagged(sPos) && cond(sPos))
      {
        q.push_back(sPos);
        setTag(sPos);
      }
      it++;
    }
    for (it = q.begin(); it != q.end(); it++)
      clearTag(*it);
  }
  void setPlayer(const int player)
  {
    _player = player;
  }
  // Установить следующего игрока как текущего.
  void setNextPlayer()
  {
    setPlayer(nextPlayer(_player));
  }
  // Поставить точку на поле следующего по очереди игрока.
  bool doStep(const int pos)
  {
    if (isPuttingAllowed(pos))
    {
      doUnsafeStep(pos);
      return true;
    }
    return false;
  }
  // Поставить точку на поле.
  bool doStep(const int pos, const int player)
  {
    if (isPuttingAllowed(pos))
    {
      doUnsafeStep(pos, player);
      return true;
    }
    return false;
  }
  // Поставить точку на поле максимально быстро (без дополнительных проверок).
  void doUnsafeStep(const int pos)
  {
    doUnsafeStep(pos, _player);
  }
  void doUnsafeStep(const int pos, const int player)
  {
    _changes.emplace_back(_captureCount[0], _captureCount[1], _player, _hash);
    _changes.back().changes.emplace(pos, _points[pos]);
    _pointsSeq.push_back(pos);
    // Добавляем в изменения поставленную точку.
    setPlayerPutted(pos, player);
    updateHash(pos, player);
    checkClosure(pos, player);
    setPlayer(nextPlayer(player));
  }
  // Откат хода.
  void undoStep()
  {
    _pointsSeq.pop_back();
    BoardChange& change = _changes.back();
    while (!change.changes.empty())
    {
      pair<int, int>& top = change.changes.top();
      _points[top.first] = top.second;
      change.changes.pop();
    }
    _captureCount[0] = change.captureCount[0];
    _captureCount[1] = change.captureCount[1];
    _player = change.player;
    _hash = change.hash;
    _changes.pop_back();
  }
};
