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
	static const PosValue player_bit = 0x1;
	// Бит, указывающий на наличие точки в поле.
	static const PosValue put_bit = 0x2;
	// Бит, указывающий на захваченность точки.
	static const PosValue sur_bit = 0x4;
	// Бит, указывающий на то, захватывает ли что-нибудь точка на поле.
	static const PosValue bound_bit = 0x8;
	// Бит, указывающий на пустую базу.
	static const PosValue empty_base_bit = 0x10;
	// Бит для временных пометок полей.
	static const PosValue tag_bit = 0x20;
	// Бит, которым помечаются границы поля.
	static const PosValue bad_bit = 0x40;

	// Маски для определения условий.
	static const PosValue enable_mask = bad_bit | sur_bit | put_bit | player_bit;
	static const PosValue bound_mask = bad_bit | bound_bit | sur_bit | put_bit | player_bit;

public:
	// Get state functions.
	// Функции получения состояния.

	// Получить по координате игрока, чья точка там поставлена.
	inline Player get_player(const Pos pos) const { return _points[pos] & player_bit; }
	// Проверить по координате, поставлена ли там точка.
	inline bool is_putted(const Pos pos) const { return (_points[pos] & put_bit) != 0; }
	// Прверить по координате, является ли точка окружающей базу.
	inline bool is_base_bound(const Pos pos) const { return (_points[pos] & bound_bit) != 0; }
	// Проверить по координате, захвачено ли поле.
	inline bool is_captured(const Pos pos) const { return (_points[pos] & sur_bit) != 0; }
	// Проверить по координате, лежит ли она в пустой базе.
	inline bool is_in_empty_base(const Pos pos) const { return (_points[pos] & empty_base_bit) != 0; }
	// Проверить по координате, помечено ли поле.
	inline bool is_tagged(const Pos pos) const { return (_points[pos] & tag_bit) != 0; }
	// Получить условие по координате.
	inline PosValue get_enable_cond(const Pos pos) const { return _points[pos] & enable_mask; }
	// Проверка незанятости поля по условию.
	inline bool is_enable(const Pos pos, const PosValue enable_cond) const { return (_points[pos] & enable_mask) == enable_cond; }
	// Проверка занятости поля по условию.
	inline bool is_not_enable(const Pos pos, const PosValue enable_cond) const { return (_points[pos] & enable_mask) != enable_cond; }
	// Проверка на то, захвачено ли поле.
	inline bool is_bound(const Pos pos, const PosValue bound_cond) const { return (_points[pos] & bound_mask) == bound_cond; }
	// Проверка на то, не захвачено ли поле.
	inline bool is_not_bound(const Pos pos, const PosValue bound_cond) const { return (_points[pos] & bound_mask) != bound_cond; }
	// Провека на то, возможно ли поставить точку в полке.
	inline bool putting_allow(const Pos pos) const { return !(_points[pos] & (put_bit | sur_bit | bad_bit)); }

	// Set state functions.
	// Функции установки состояния.

	// Пометить поле по координате как содержащее точку.
	inline void set_putted(const Pos pos) { _points[pos] |= put_bit; }
	// Убрать с поля по координате put_bit.
	inline void clear_put_bit(const Pos pos) { _points[pos] &= ~put_bit; }
	// Пометить поле по координате как принадлежащее игроку.
	inline void set_player(const Pos pos, const Player player) { _points[pos] = (_points[pos] & ~player_bit) | player; }
	// Пометить поле по координате как содержащее точку игрока.
	inline void set_player_putted(const Pos pos, const Player player) { _points[pos] = (_points[pos] & ~player_bit) | player | put_bit; }
	// Пометить битом SurBit (захватить).
	inline void capture(const Pos pos) { _points[pos] |= sur_bit; }
	// Убрать бит SurBit.
	inline void free(const Pos pos) { _points[pos] &= ~sur_bit; }
	// Пометить точку как окружающую базу.
	inline void set_base_bound(const Pos pos) { _points[pos] |= bound_bit; }
	// Пометить точку как не окружающую базу.
	inline void clear_base_bound(const Pos pos) { _points[pos] &= ~bound_bit; }
	inline void set_empty_base(const Pos pos) { _points[pos] |= empty_base_bit; }
	inline void clear_empty_base(const Pos pos) { _points[pos] &= ~empty_base_bit; }
	// Установить бит TagBit.
	inline void set_tag(const Pos pos) { _points[pos] |= tag_bit; }
	// Убрать бит TagBit.
	inline void clear_tag(const Pos pos) { _points[pos] &= ~tag_bit; }
	inline void set_bad(const Pos pos) { _points[pos] |= bad_bit; }
	inline void clear_bad(const Pos pos) { _points[pos] &= ~bad_bit; }

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
	Score _capture_count[2];

	Zobrist* _zobrist;

	Hash _hash;

public:
	// History points sequance.
	// Последовательность поставленных точек.
	vector<Pos> points_seq;

private:
	// Возвращает косое произведение векторов Pos1 и Pos2.
	inline int square(const Pos pos1, const Pos pos2) const { return to_x(pos1) * to_y(pos2) - to_y(pos1) * to_x(pos2); }
	//  * . .   x . *   . x x   . . .
	//  . o .   x o .   . o .   . o x
	//  x x .   . . .   . . *   * . x
	//  o - center pos
	//  x - pos
	//  * - result
	inline void get_first_next_pos(const Pos center_pos, Pos &pos) const
	{
		if (pos < center_pos)
		{
			if ((pos == nw(center_pos)) || (pos == center_pos - 1))
				pos = ne(center_pos);
			else
				pos = se(center_pos);
		}
		else
		{
			if ((pos == center_pos + 1) || (pos == se(center_pos)))
				pos = sw(center_pos);
			else
				pos = nw(center_pos);
		}
	}
	//  . . .   * . .   x * .   . x *   . . x   . . .   . . .   . . .
	//  * o .   x o .   . o .   . o .   . o *   . o x   . o .   . o .
	//  x . .   . . .   . . .   . . .   . . .   . . *   . * x   * x .
	//  o - center pos
	//  x - pos
	//  * - result
	inline void get_next_pos(const Pos center_pos, Pos &pos) const
	{
		if (pos < center_pos)
		{
			if (pos == nw(center_pos))
				pos = n(center_pos);
			else if (pos == n(center_pos))
				pos = ne(center_pos);
			else if (pos == ne(center_pos))
				pos = e(center_pos);
			else
				pos = nw(center_pos);
		}
		else
		{
			if (pos == e(center_pos))
                pos = se(center_pos);
			else if (pos == se(center_pos))
				pos = s(center_pos);
			else if (pos == s(center_pos))
				pos = sw(center_pos);
			else
				pos = w(center_pos);
		}
	}
	// Возвращает количество групп точек рядом с CenterPos.
	// InpChainPoints - возможные точки цикла, InpSurPoints - возможные окруженные точки.
	short get_input_points(const Pos center_pos, const PosValue enable_cond, Pos inp_chain_points[], Pos inp_sur_points[]) const;
	// Поставить начальные точки.
	void place_begin_pattern(BeginPattern pattern);
	// Изменение счета игроков.
	inline void add_sub_captured_freed(const Player player, const Score captured, const Score freed)
	{
		if (captured == -1)
		{
			_capture_count[next_player(player)]++;
		}
		else
		{
			_capture_count[player] += captured;
			_capture_count[next_player(player)] -= freed;
		}
	}
	// Изменяет Captured/Free в зависимости от того, захвачена или окружена точка.
	inline void check_captured_and_freed(const Pos pos, const Player player, Score &captured, Score &freed)
	{
		if (is_putted(pos))
		{
			if (get_player(pos) != player)
				captured++;
			else if (is_captured(pos))
				freed++;
		}
	}
	// Захватывает/освобождает окруженную точку.
	inline void set_capture_free_state(const Pos pos, const Player player)
	{
		if (is_putted(pos))
		{
			if (get_player(pos) != player)
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

	inline Score get_score(Player player) const { return _capture_count[player] - _capture_count[next_player(player)]; }
	inline Score get_prev_score(Player player) const { return _changes.back().captureCount[player] - _changes.back().captureCount[next_player(player)]; }
	inline Player get_last_player() const { return get_player(points_seq.back()); }
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
		if (putting_allow(pos))
		{
			do_unsafe_step(pos);
			return true;
		}
		return false;
	}
	// Поставить точку на поле.
	inline bool do_step(const Pos pos, const Player player)
	{
		if (putting_allow(pos))
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
		_changes.back().captureCount[0] = _capture_count[0];
		_changes.back().captureCount[1] = _capture_count[1];
		_changes.back().player = _player;

		// Добавляем в изменения поставленную точку.
		_changes.back().changes.push(pair<Pos, PosValue>(pos, _points[pos]));

		set_player_putted(pos, _player);

		points_seq.push_back(pos);

		check_closure(pos, _player);

		set_next_player();
	}
	// Поставить точку на поле следующего по очереди игрока максимально быстро (без дополнительных проверок).
	inline void do_unsafe_step(const Pos pos, const Player player)
	{
		_changes.push_back(BoardChange());
		_changes.back().captureCount[0] = _capture_count[0];
		_changes.back().captureCount[1] = _capture_count[1];
		_changes.back().player = _player;

		// Добавляем в изменения поставленную точку.
		_changes.back().changes.push(pair<Pos, PosValue>(pos, _points[pos]));

		set_player_putted(pos, player);

		points_seq.push_back(pos);

		check_closure(pos, player);
	}
	// Откат хода.
	inline void undo_step()
	{
		points_seq.pop_back();
		while (!_changes.back().changes.empty())
		{
			_points[_changes.back().changes.top().first] = _changes.back().changes.top().second;
			_changes.back().changes.pop();
		}
		_player = _changes.back().player;
		_capture_count[0] = _changes.back().captureCount[0];
		_capture_count[1] = _changes.back().captureCount[1];
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
		if (is_enable(n(center_pos), put_bit | player)  ||
			is_enable(s(center_pos), put_bit | player)  ||
			is_enable(w(center_pos), put_bit | player)  ||
			is_enable(e(center_pos), put_bit | player)  ||
			is_enable(nw(center_pos), put_bit | player) ||
			is_enable(ne(center_pos), put_bit | player) ||
			is_enable(sw(center_pos), put_bit | player) ||
			is_enable(se(center_pos), put_bit | player))
			return true;
		else
			return false;
	}
	// Возвращает количество точек рядом с center_pos цвета player.
	inline short number_near_points(const Pos center_pos, const Player player) const
	{
		short result = 0;
		if (is_enable(n(center_pos), put_bit | player))
			result++;
		if (is_enable(s(center_pos), put_bit | player))
			result++;
		if (is_enable(w(center_pos), put_bit | player))
			result++;
		if (is_enable(e(center_pos), put_bit | player))
			result++;
		if (is_enable(nw(center_pos), put_bit | player))
			result++;
		if (is_enable(ne(center_pos), put_bit | player))
			result++;
		if (is_enable(sw(center_pos), put_bit | player))
			result++;
		if (is_enable(se(center_pos), put_bit | player))
			result++;
		return result;
	}
	// Возвращает количество групп точек рядом с center_pos.
	inline short number_near_groups(const Pos center_pos, const Player player) const
	{
		short result = 0;
		if (is_not_enable(w(center_pos), player | put_bit) && (is_enable(nw(center_pos), player | put_bit) || is_enable(n(center_pos), player | put_bit)))
			result++;
		if (is_not_enable(s(center_pos), player | put_bit) && (is_enable(sw(center_pos), player | put_bit) || is_enable(w(center_pos), player | put_bit)))
			result++;
		if (is_not_enable(e(center_pos), player | put_bit) && (is_enable(se(center_pos), player | put_bit) || is_enable(s(center_pos), player | put_bit)))
			result++;
		if (is_not_enable(n(center_pos), player | put_bit) && (is_enable(ne(center_pos), player | put_bit) || is_enable(e(center_pos), player | put_bit)))
			result++;
		return result;
	}
	void wave(Pos start_pos, function<bool(Pos)> cond);
	bool is_point_inside_ring(const Pos pos, const list<Pos> &ring) const;
	// Проверяет поставленную точку на наличие созданных ею окружений, и окружает, если они есть.
	void check_closure(const Pos start_pos, Player player);
};
