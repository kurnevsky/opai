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
	static const PosValue bound_mask = badBit | boundBit | surBit | putBit | playerBit;

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
	bool isBound(const Pos pos, const PosValue boundCond) const { return (_points[pos] & bound_mask) == boundCond; }
	// Проверка на то, не захвачено ли поле.
	bool isNotBound(const Pos pos, const PosValue boundCond) const { return (_points[pos] & bound_mask) != boundCond; }
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
	int square(const Pos pos1, const Pos pos2) const { return to_x(pos1) * to_y(pos2) - to_y(pos1) * to_x(pos2); }
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
	int getInputPoints(const Pos centerPos, const PosValue enableCond, Pos inpChainPoints[], Pos inpSurPoints[]) const;
	// Поставить начальные точки.
	void place_begin_pattern(BeginPattern pattern);
	// Изменение счета игроков.
	inline void add_sub_captured_freed(const Player player, const Score captured, const Score freed)
	{
		if (captured == -1)
		{
			_captureCount[next_player(player)]++;
		}
		else
		{
			_captureCount[player] += captured;
			_captureCount[next_player(player)] -= freed;
		}
	}
	// Изменяет Captured/Free в зависимости от того, захвачена или окружена точка.
	inline void check_captured_and_freed(const Pos pos, const Player player, Score &captured, Score &freed)
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
	inline void set_capture_free_state(const Pos pos, const Player player)
	{
		if (isPutted(pos))
		{
			if (getPlayer(pos) != player)
				capture(pos);
			else
				free(pos);
		}
		else
			capture(pos);
	}
	// Удаляет пометку пустой базы с поля точек, начиная с позиции StartPos.
	void remove_empty_base(const Pos start_pos);
	bool build_chain(const Pos start_pos, const PosValue enable_cond, const Pos direction_pos, list<Pos> &chain);
	void find_surround(list<Pos> &chain, Pos inside_point, Player player);
	inline void update_hash(Player player, Pos pos) { _hash ^= _zobrist->getHash((player + 1) * pos); }
	inline IntersectionState get_intersection_state(const Pos pos, const Pos next_pos) const
	{
		Point a, b;
		to_xy(pos, a.x, a.y);
		to_xy(next_pos, b.x, b.y);

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
	Field(const Coord width, const Coord height, const BeginPattern begin_pattern, Zobrist* zobr);
	// Конструктор копирования.
	Field(const Field &orig);
	// Деструктор.
	~Field();

	inline Score get_score(Player player) const { return _captureCount[player] - _captureCount[next_player(player)]; }
	inline Score get_prev_score(Player player) const { return _changes.back().captureCount[player] - _changes.back().captureCount[next_player(player)]; }
	inline Player get_last_player() const { return getPlayer(pointsSeq.back()); }
	inline Score get_d_score(Player player) const { return get_score(player) - get_prev_score(player); }
	inline Score get_d_score() const { return get_d_score(get_last_player()); }
	inline Player get_player() const { return _player; }
	inline Hash get_hash() const { return _hash; }
	inline Hash get_hash(Pos pos) const { return _zobrist->getHash(pos); }
	inline Zobrist& get_zobrist() const { return *_zobrist; };
	inline Coord width() const { return _width; }
	inline Coord height() const { return _height; }
	inline Pos length() const { return (_width + 2) * (_height + 2); }
	inline Pos min_pos() const { return to_pos(0, 0); }
	inline Pos max_pos() const { return to_pos(_width - 1, _height - 1); }
	inline Pos n(Pos pos) const { return pos - (_width + 2); }
	inline Pos s(Pos pos) const { return pos + (_width + 2); }
	inline Pos w(Pos pos) const { return pos - 1; }
	inline Pos e(Pos pos) const { return pos + 1; }
	inline Pos nw(Pos pos) const { return pos - (_width + 2) - 1; }
	inline Pos ne(Pos pos) const { return pos - (_width + 2) + 1; }
	inline Pos sw(Pos pos) const { return pos + (_width + 2) - 1; }
	inline Pos se(Pos pos) const { return pos + (_width + 2) + 1; }
	inline Pos to_pos(const Coord x, const Coord y) const { return (y + 1) * (_width + 2) + x + 1; }
	inline Coord to_x(const Pos pos) const { return static_cast<Coord>(pos % (_width + 2) - 1); }
	inline Coord to_y(const Pos pos) const { return static_cast<Coord>(pos / (_width + 2) - 1); }
	// Конвертация из Pos в XY.
	inline void to_xy(const Pos pos, Coord &x, Coord &y) const { x = to_x(pos); y = to_y(pos); }
	inline void set_player(const Player player) { _player = player; }
	// Установить следующего игрока как текущего.
	inline void set_next_player() { set_player(next_player(_player)); }
	// Поставить точку на поле следующего по очереди игрока.
	inline bool do_step(const Pos pos)
	{
		if (puttingAllow(pos))
		{
			do_unsafe_step(pos);
			return true;
		}
		return false;
	}
	// Поставить точку на поле.
	inline bool do_step(const Pos pos, const Player player)
	{
		if (puttingAllow(pos))
		{
			do_unsafe_step(pos, player);
			return true;
		}
		return false;
	}
	// Поставить точку на поле максимально быстро (без дополнительных проверок).
	inline void do_unsafe_step(const Pos pos)
	{
		_changes.push_back(BoardChange());
		_changes.back().captureCount[0] = _captureCount[0];
		_changes.back().captureCount[1] = _captureCount[1];
		_changes.back().player = _player;

		// Добавляем в изменения поставленную точку.
		_changes.back().changes.push(pair<Pos, PosValue>(pos, _points[pos]));

		setPlayerPutted(pos, _player);

		pointsSeq.push_back(pos);

		check_closure(pos, _player);

		set_next_player();
	}
	// Поставить точку на поле следующего по очереди игрока максимально быстро (без дополнительных проверок).
	inline void do_unsafe_step(const Pos pos, const Player player)
	{
		_changes.push_back(BoardChange());
		_changes.back().captureCount[0] = _captureCount[0];
		_changes.back().captureCount[1] = _captureCount[1];
		_changes.back().player = _player;

		// Добавляем в изменения поставленную точку.
		_changes.back().changes.push(pair<Pos, PosValue>(pos, _points[pos]));

		setPlayerPutted(pos, player);

		pointsSeq.push_back(pos);

		check_closure(pos, player);
	}
	// Откат хода.
	inline void undo_step()
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
	inline bool is_near(const Pos pos1, const Pos pos2) const
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
	// Проверяет, есть ли рядом с center_pos точки цвета player.
	inline bool is_near_points(const Pos center_pos, const Player player) const
	{
		if (isEnable(n(center_pos), putBit | player)  ||
			isEnable(s(center_pos), putBit | player)  ||
			isEnable(w(center_pos), putBit | player)  ||
			isEnable(e(center_pos), putBit | player)  ||
			isEnable(nw(center_pos), putBit | player) ||
			isEnable(ne(center_pos), putBit | player) ||
			isEnable(sw(center_pos), putBit | player) ||
			isEnable(se(center_pos), putBit | player))
			return true;
		else
			return false;
	}
	// Возвращает количество точек рядом с center_pos цвета player.
	inline short number_near_points(const Pos center_pos, const Player player) const
	{
		short result = 0;
		if (isEnable(n(center_pos), putBit | player))
			result++;
		if (isEnable(s(center_pos), putBit | player))
			result++;
		if (isEnable(w(center_pos), putBit | player))
			result++;
		if (isEnable(e(center_pos), putBit | player))
			result++;
		if (isEnable(nw(center_pos), putBit | player))
			result++;
		if (isEnable(ne(center_pos), putBit | player))
			result++;
		if (isEnable(sw(center_pos), putBit | player))
			result++;
		if (isEnable(se(center_pos), putBit | player))
			result++;
		return result;
	}
	// Возвращает количество групп точек рядом с center_pos.
	inline short number_near_groups(const Pos center_pos, const Player player) const
	{
		short result = 0;
		if (isNotEnable(w(center_pos), player | putBit) && (isEnable(nw(center_pos), player | putBit) || isEnable(n(center_pos), player | putBit)))
			result++;
		if (isNotEnable(s(center_pos), player | putBit) && (isEnable(sw(center_pos), player | putBit) || isEnable(w(center_pos), player | putBit)))
			result++;
		if (isNotEnable(e(center_pos), player | putBit) && (isEnable(se(center_pos), player | putBit) || isEnable(s(center_pos), player | putBit)))
			result++;
		if (isNotEnable(n(center_pos), player | putBit) && (isEnable(ne(center_pos), player | putBit) || isEnable(e(center_pos), player | putBit)))
			result++;
		return result;
	}
	void wave(Pos start_pos, function<bool(Pos)> cond);
	bool is_point_inside_ring(const Pos pos, const list<Pos> &ring) const;
	// Проверяет поставленную точку на наличие созданных ею окружений, и окружает, если они есть.
	void check_closure(const Pos start_pos, Player player);
};
