#pragma once

#include "config.h"
#include "basic_types.h"
#include "zobrist.h"
#include "player.h"
#include <stack>
#include <list>
#include <vector>
#include <algorithm>
#include <functional>
#include <utility>

using namespace std;

class Field
{
private:

  // State bits and masks
  // Бит, указывающий номер игрока.
  static const PosValue playerBit = 0x1;
  // Бит, указывающий на наличие точки в поле.
  static const PosValue putBit = 0x2;
  // Бит, указывающий на захваченность точки.
  static const PosValue surBit = 0x4;
  // Бит, указывающий на то, захватывает ли что-нибудь точка на поле.
  static const PosValue boundBit = 0x8;
  // Бит, указывающий на пустую базу.
  static const PosValue emptyBaseBit = 0x10;
  // Бит для временных пометок полей.
  static const PosValue tagBit = 0x20;
  // Бит, которым помечаются границы поля.
  static const PosValue badBit = 0x40;

  // Маски для определения условий.
  static const PosValue enableMask = badBit | surBit | putBit | playerBit;
  static const PosValue boundMask = badBit | boundBit | surBit | putBit | playerBit;

public:
  // Get state functions.
  // Функции получения состояния.

  // Получить по координате игрока, чья точка там поставлена.
  Player getPlayer(const Pos pos) const { return _points[pos] & playerBit; }
  // Проверить по координате, поставлена ли там точка.
  bool isPutted(const Pos pos) const { return (_points[pos] & putBit) != 0; }
  // Прверить по координате, является ли точка окружающей базу.
  bool isBaseBound(const Pos pos) const { return (_points[pos] & boundBit) != 0; }
  // Проверить по координате, захвачено ли поле.
  bool isCaptured(const Pos pos) const { return (_points[pos] & surBit) != 0; }
  // Проверить по координате, лежит ли она в пустой базе.
  bool isInEmptyBase(const Pos pos) const { return (_points[pos] & emptyBaseBit) != 0; }
  // Проверить по координате, помечено ли поле.
  bool isTagged(const Pos pos) const { return (_points[pos] & tagBit) != 0; }
  // Получить условие по координате.
  PosValue getEnableCond(const Pos pos) const { return _points[pos] & enableMask; }
  // Проверка незанятости поля по условию.
  bool isEnable(const Pos pos, const PosValue enableCond) const { return (_points[pos] & enableMask) == enableCond; }
  // Проверка занятости поля по условию.
  bool isNotEnable(const Pos pos, const PosValue enableCond) const { return (_points[pos] & enableMask) != enableCond; }
  // Проверка на то, захвачено ли поле.
  bool isBound(const Pos pos, const PosValue boundCond) const { return (_points[pos] & boundMask) == boundCond; }
  // Проверка на то, не захвачено ли поле.
  bool isNotBound(const Pos pos, const PosValue boundCond) const { return (_points[pos] & boundMask) != boundCond; }
  // Провека на то, возможно ли поставить точку в полке.
  bool puttingAllow(const Pos pos) const { return !(_points[pos] & (putBit | surBit | badBit)); }

  // Set state functions.
  // Функции установки состояния.

  // Пометить поле по координате как содержащее точку.
  void setPutted(const Pos pos) { _points[pos] |= putBit; }
  // Убрать с поля по координате put_bit.
  void clearPuted(const Pos pos) { _points[pos] &= ~putBit; }
  // Пометить поле по координате как принадлежащее игроку.
  void setPlayer(const Pos pos, const Player player) { _points[pos] = (_points[pos] & ~playerBit) | player; }
  // Пометить поле по координате как содержащее точку игрока.
  void setPlayerPutted(const Pos pos, const Player player) { _points[pos] = (_points[pos] & ~playerBit) | player | putBit; }
  // Пометить битом SurBit (захватить).
  void capture(const Pos pos) { _points[pos] |= surBit; }
  // Убрать бит SurBit.
  void free(const Pos pos) { _points[pos] &= ~surBit; }
  // Пометить точку как окружающую базу.
  void setBaseBound(const Pos pos) { _points[pos] |= boundBit; }
  // Пометить точку как не окружающую базу.
  void clearBaseBound(const Pos pos) { _points[pos] &= ~boundBit; }
  void setEmptyBase(const Pos pos) { _points[pos] |= emptyBaseBit; }
  void clearEmptyBase(const Pos pos) { _points[pos] &= ~emptyBaseBit; }
  // Установить бит TagBit.
  void setTag(const Pos pos) { _points[pos] |= tagBit; }
  // Убрать бит TagBit.
  void clearTag(const Pos pos) { _points[pos] &= ~tagBit; }
  void setBad(const Pos pos) { _points[pos] |= badBit; }
  void clearBad(const Pos pos) { _points[pos] &= ~badBit; }

private:
  vector<BoardChange> _changes;
  // Main points array (game board).
  // Основной массив точек (игровая доска).
  PosValue* _points;
  // Real field width.
  // Действительная ширина поля.
  Coord _width;
  // Real field height.
  // Действительная высота поля.
  Coord _height;
  // Current player color.
  // Текущий цвет игроков.
  Player _player;
  // Capture points count.
  // Количество захваченных точек.
  Score _captureCount[2];
  Zobrist* _zobrist;
  Hash _hash;

public:
  // History points sequance.
  // Последовательность поставленных точек.
  vector<Pos> pointsSeq;

private:
  // Возвращает косое произведение векторов Pos1 и Pos2.
  int square(const Pos pos1, const Pos pos2) const { return toX(pos1) * toY(pos2) - toY(pos1) * toX(pos2); }
  //  * . .   x . *   . x x   . . .
  //  . o .   x o .   . o .   . o x
  //  x x .   . . .   . . *   * . x
  //  o - center pos
  //  x - pos
  //  * - result
  void getFirstNextPos(const Pos centerPos, Pos &pos) const
  {
    if (pos < centerPos)
    {
      if ((pos == nw(centerPos)) || (pos == centerPos - 1))
        pos = ne(centerPos);
      else
        pos = se(centerPos);
    }
    else
    {
      if ((pos == centerPos + 1) || (pos == se(centerPos)))
        pos = sw(centerPos);
      else
        pos = nw(centerPos);
    }
  }
  //  . . .   * . .   x * .   . x *   . . x   . . .   . . .   . . .
  //  * o .   x o .   . o .   . o .   . o *   . o x   . o .   . o .
  //  x . .   . . .   . . .   . . .   . . .   . . *   . * x   * x .
  //  o - center pos
  //  x - pos
  //  * - result
  void getNextPos(const Pos centerPos, Pos &pos) const
  {
    if (pos < centerPos)
    {
      if (pos == nw(centerPos))
        pos = n(centerPos);
      else if (pos == n(centerPos))
        pos = ne(centerPos);
      else if (pos == ne(centerPos))
        pos = e(centerPos);
      else
        pos = nw(centerPos);
    }
    else
    {
      if (pos == e(centerPos))
                pos = se(centerPos);
      else if (pos == se(centerPos))
        pos = s(centerPos);
      else if (pos == s(centerPos))
        pos = sw(centerPos);
      else
        pos = w(centerPos);
    }
  }
  // Возвращает количество групп точек рядом с CenterPos.
  // InpChainPoints - возможные точки цикла, InpSurPoints - возможные окруженные точки.
  int getInputPoints(const Pos centerPos, const PosValue enableCond, Pos inpChainPoints[], Pos inpSurPoints[]) const
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
  // Изменение счета игроков.
  void addSubCapturedFreed(const Player player, const Score captured, const Score freed)
  {
    if (captured == -1)
    {
      _captureCount[nextPlayer(player)]++;
    }
    else
    {
      _captureCount[player] += captured;
      _captureCount[nextPlayer(player)] -= freed;
    }
  }
  // Изменяет Captured/Free в зависимости от того, захвачена или окружена точка.
  void checkCapturedAndFreed(const Pos pos, const Player player, Score &captured, Score &freed)
  {
    if (isPutted(pos))
    {
      if (getPlayer(pos) != player)
        captured++;
      else if (isCaptured(pos))
        freed++;
    }
  }
  // Захватывает/освобождает окруженную точку.
  void setCaptureFreeState(const Pos pos, const Player player)
  {
    if (isPutted(pos))
    {
      if (getPlayer(pos) != player)
        capture(pos);
      else
        free(pos);
    }
    else
    {
      capture(pos);
    }
  }
  // Удаляет пометку пустой базы с поля точек, начиная с позиции StartPos.
  void removeEmptyBase(const Pos startPos)
  {
    wave(startPos,
        [&](Pos pos)->bool
        {
          if (isInEmptyBase(pos))
          {
            _changes.back().changes.push(pair<Pos, PosValue>(pos, _points[pos] & !tagBit));
            clearEmptyBase(pos);
            return true;
          }
          else
          {
            return false;
          }
        });
  }
  bool buildChain(const Pos startPos, const PosValue enableCond, const Pos directionPos, list<Pos> &chain)
  {
    chain.clear();
    chain.push_back(startPos);
    Pos pos = directionPos;
    Pos centerPos = startPos;
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
      getFirstNextPos(centerPos, pos);
      while (isNotEnable(pos, enableCond))
        getNextPos(centerPos, pos);
      baseSquare += square(centerPos, pos);
    }
    while (pos != startPos);
    for (auto i = chain.begin(); i != chain.end(); i++)
      clearTag(*i);
    return (baseSquare < 0 && chain.size() > 2);
  }
  void findSurround(list<Pos> &chain, Pos insidePoint, Player player)
  {
    // Количество захваченных точек.
    int curCaptureCount = 0;
    // Количество захваченных пустых полей.
    int curFreedCount = 0;
    list<Pos> surPoints;
    // Помечаем точки цепочки.
    for (auto i = chain.begin(); i != chain.end(); i++)
      setTag(*i);
    wave(insidePoint,
        [&, player](Pos pos)->bool
        {
          if (isNotBound(pos, player | putBit | boundBit))
          {
            checkCapturedAndFreed(pos, player, curCaptureCount, curFreedCount);
            surPoints.push_back(pos);
            return true;
          }
          else
          {
            return false;
          }
        });
    // Изменение счета игроков.
    addSubCapturedFreed(player, curCaptureCount, curFreedCount);
#if SUR_COND != 1
    if (curCaptureCount != 0) // Если захватили точки.
#endif
    {
      for (auto i = chain.begin(); i != chain.end(); i++)
      {
        clearTag(*i);
        // Добавляем в список изменений точки цепочки.
        _changes.back().changes.push(pair<Pos, PosValue>(*i, _points[*i]));
        // Помечаем точки цепочки.
        setBaseBound(*i);
      }
      for (auto i = surPoints.begin(); i != surPoints.end(); i++)
      {
        _changes.back().changes.push(pair<Pos, PosValue>(*i, _points[*i]));

        setCaptureFreeState(*i, player);
      }
    }
    else // Если ничего не захватили.
    {
      for (auto i = chain.begin(); i != chain.end(); i++)
        clearTag(*i);
      for (auto i = surPoints.begin(); i != surPoints.end(); i++)
      {
        _changes.back().changes.push(pair<Pos, PosValue>(*i, _points[*i]));

        if (!isPutted(*i))
          setEmptyBase(*i);
      }
    }
  }
  void updateHash(Player player, Pos pos) { _hash ^= _zobrist->getHash((player + 1) * pos); }
  IntersectionState getIntersectionState(const Pos pos, const Pos nextPos) const
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

public:
  // Конструктор.
  Field(const Coord width, const Coord height, const BeginPattern begin_pattern, Zobrist* zobrist)
  {
    _width = width;
    _height = height;
    _player = playerRed;
    _captureCount[playerRed] = 0;
    _captureCount[playerBlack] = 0;
    _points = new PosValue[getLength()];
    fill_n(_points, getLength(), 0);
    for (Coord x = -1; x <= width; x++)
    {
      setBad(toPos(x, -1));
      setBad(toPos(x, height));
    }
    for (Coord y = -1; y <= height; y++)
    {
      setBad(toPos(-1, y));
      setBad(toPos(width, y));
    }
    _changes.reserve(getLength());
    pointsSeq.reserve(getLength());
    _zobrist = zobrist;
    _hash = 0;
    placeBeginPattern(begin_pattern);
  }
  // Конструктор копирования.
  Field(const Field &orig)
  {
    _width = orig._width;
    _height = orig._height;
    _player = orig._player;
    _captureCount[playerRed] = orig._captureCount[playerRed];
    _captureCount[playerBlack] = orig._captureCount[playerBlack];
    _points = new PosValue[getLength()];
    copy_n(orig._points, getLength(), _points);
    _changes.reserve(getLength());
    pointsSeq.reserve(getLength());
    _changes.assign(orig._changes.begin(), orig._changes.end());
    pointsSeq.assign(orig.pointsSeq.begin(), orig.pointsSeq.end());
    _zobrist = orig._zobrist;
    _hash = orig._hash;
  }
  // Деструктор.
  ~Field()
  {
    delete _points;
  }
  Score getScore(Player player) const { return _captureCount[player] - _captureCount[nextPlayer(player)]; }
  Score getPrevScore(Player player) const { return _changes.back().captureCount[player] - _changes.back().captureCount[nextPlayer(player)]; }
  Player getLastPlayer() const { return getPlayer(pointsSeq.back()); }
  Score getDScore(Player player) const { return getScore(player) - getPrevScore(player); }
  Score getDScore() const { return getDScore(getLastPlayer()); }
  Player getPlayer() const { return _player; }
  Hash getHash() const { return _hash; }
  Zobrist& getZobrist() const { return *_zobrist; };
  Coord getWidth() const { return _width; }
  Coord getHeight() const { return _height; }
  Pos getLength() const { return (_width + 2) * (_height + 2); }
  Pos minPos() const { return toPos(0, 0); }
  Pos maxPos() const { return toPos(_width - 1, _height - 1); }
  Pos n(Pos pos) const { return pos - (_width + 2); }
  Pos s(Pos pos) const { return pos + (_width + 2); }
  Pos w(Pos pos) const { return pos - 1; }
  Pos e(Pos pos) const { return pos + 1; }
  Pos nw(Pos pos) const { return pos - (_width + 2) - 1; }
  Pos ne(Pos pos) const { return pos - (_width + 2) + 1; }
  Pos sw(Pos pos) const { return pos + (_width + 2) - 1; }
  Pos se(Pos pos) const { return pos + (_width + 2) + 1; }
  Pos toPos(const Coord x, const Coord y) const { return (y + 1) * (_width + 2) + x + 1; }
  Coord toX(const Pos pos) const { return static_cast<Coord>(pos % (_width + 2) - 1); }
  Coord toY(const Pos pos) const { return static_cast<Coord>(pos / (_width + 2) - 1); }
  // Конвертация из Pos в XY.
  void toXY(const Pos pos, Coord &x, Coord &y) const { x = toX(pos); y = toY(pos); }
  void setPlayer(const Player player) { _player = player; }
  // Установить следующего игрока как текущего.
  void setNextPlayer() { setPlayer(nextPlayer(_player)); }
  // Поставить точку на поле следующего по очереди игрока.
  bool doStep(const Pos pos)
  {
    if (puttingAllow(pos))
    {
      doUnsafeStep(pos);
      return true;
    }
    return false;
  }
  // Поставить точку на поле.
  bool doStep(const Pos pos, const Player player)
  {
    if (puttingAllow(pos))
    {
      doUnsafeStep(pos, player);
      return true;
    }
    return false;
  }
  // Поставить точку на поле максимально быстро (без дополнительных проверок).
  void doUnsafeStep(const Pos pos)
  {
    doUnsafeStep(pos, _player);
    setNextPlayer();
  }
  void doUnsafeStep(const Pos pos, const Player player)
  {
    _changes.push_back(BoardChange());
    _changes.back().captureCount[0] = _captureCount[0];
    _changes.back().captureCount[1] = _captureCount[1];
    _changes.back().player = _player;
    // Добавляем в изменения поставленную точку.
    _changes.back().changes.push(pair<Pos, PosValue>(pos, _points[pos]));
    setPlayerPutted(pos, player);
    pointsSeq.push_back(pos);
    checkClosure(pos, player);
  }
  // Откат хода.
  void undoStep()
  {
    pointsSeq.pop_back();
    while (!_changes.back().changes.empty())
    {
      _points[_changes.back().changes.top().first] = _changes.back().changes.top().second;
      _changes.back().changes.pop();
    }
    _player = _changes.back().player;
    _captureCount[0] = _changes.back().captureCount[0];
    _captureCount[1] = _changes.back().captureCount[1];
    _changes.pop_back();
  }
  // Проверяет, находятся ли две точки рядом.
  bool isNear(const Pos pos1, const Pos pos2) const
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
  bool isNearPoints(const Pos centerPos, const Player player) const
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
  int numberNearPoints(const Pos centerPos, const Player player) const
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
  int numberNearGroups(const Pos centerPos, const Player player) const
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
  void wave(Pos startPos, function<bool(Pos)> cond)
  {
    // Очередь для волнового алгоритма (обхода в ширину).
    list<Pos> q;
    if (!cond(startPos))
      return;
    q.push_back(startPos);
    setTag(startPos);
    auto it = q.begin();
    while (it != q.end())
    {
      if (!isTagged(w(*it)) && cond(w(*it)))
      {
        q.push_back(w(*it));
        setTag(w(*it));
      }
      if (!isTagged(n(*it)) && cond(n(*it)))
      {
        q.push_back(n(*it));
        setTag(n(*it));
      }
      if (!isTagged(e(*it)) && cond(e(*it)))
      {
        q.push_back(e(*it));
        setTag(e(*it));
      }
      if (!isTagged(s(*it)) && cond(s(*it)))
      {
        q.push_back(s(*it));
        setTag(s(*it));
      }
      it++;
    }
    for (it = q.begin(); it != q.end(); it++)
      clearTag(*it);
  }
  bool isPointInsideRing(const Pos pos, const list<Pos> &ring) const
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
  // Проверяет поставленную точку на наличие созданных ею окружений, и окружает, если они есть.
  void checkClosure(const Pos startPos, Player player)
  {
    int inpPointsCount;
    Pos inpChainPoints[4], inpSurPoints[4];
    list<Pos> chain;
    if (isInEmptyBase(startPos)) // Если точка поставлена в пустую базу.
    {
      // Проверяем, в чью пустую базу поставлена точка.
      Pos pos = startPos - 1;
      while (!isPutted(pos))
        pos--;

      if (getPlayer(pos) == getPlayer(startPos)) // Если поставили в свою пустую базу.
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
        if (isBaseBound(startPos))
        {
          removeEmptyBase(startPos);
          return;
        }
      }
#endif
      pos++;
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
};
