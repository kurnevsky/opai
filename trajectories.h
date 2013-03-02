#pragma once

#include "field.h"
#include "trajectory.h"
#include <list>
#include <vector>
#include <algorithm>

class Trajectories
{
private:
	size_t _depth[2];
	Field* _field;
	list<Trajectory> _trajectories[2];
	int* _trajectories_board;
	bool _trajectories_board_owner;
	Zobrist* _zobrist;
	list<Pos> _moves[2];
	list<Pos> _all_moves;

private:
	template<typename _InIt>
	void add_trajectory(_InIt begin, _InIt end, Player player)
	{
		Hash hash = 0;

		// Эвристические проверки.
		// Каждая точка траектории должна окружать что-либо и иметь рядом хотя бы 2 группы точек.
		// Если нет - не добавляем эту траекторию.
		for (auto i = begin; i < end; i++)
			if (!_field->isBaseBound(*i) || (_field->number_near_groups(*i, player) < 2))
				return;

		// Высчитываем хеш траектории и сравниваем с уже существующими для исключения повторов.
		for (auto i = begin; i < end; i++)
			hash ^= _zobrist->getHash(*i);
		for (auto i = _trajectories[player].begin(); i != _trajectories[player].end(); i++)
			if (hash == i->getHash())
				return; // В теории возможны коллизии. Неплохо было бы сделать точную проверку.

		_trajectories[player].push_back(Trajectory(begin, end, _zobrist, hash));
	}
	inline void add_trajectory(Trajectory* trajectory, Player player)
	{
		_trajectories[player].push_back(Trajectory(*trajectory));
	}
	inline void add_trajectory(Trajectory* trajectory, Pos pos, Player player)
	{
		if (trajectory->size() == 1)
			return;

		_trajectories[player].push_back(Trajectory(_zobrist));
		for (auto i = trajectory->begin(); i != trajectory->end(); i++)
			if (*i != pos)
				_trajectories[player].back().pushBack(*i);
	}
	void build_trajectories_recursive(size_t depth, Player player)
	{
		for (Pos pos = _field->min_pos(); pos <= _field->max_pos(); pos++)
		{
			if (_field->puttingAllow(pos) && _field->is_near_points(pos, player))
			{
				if (_field->isInEmptyBase(pos)) // Если поставили в пустую базу (свою или нет), то дальше строить траекторию нет нужды.
				{
					_field->do_unsafe_step(pos, player);
					if (_field->get_d_score(player) > 0)
						add_trajectory(_field->pointsSeq.end() - (_depth[player] - depth), _field->pointsSeq.end(), player);
					_field->undo_step();
				}
				else
				{
					_field->do_unsafe_step(pos, player);

#if SUR_COND == 1
					if (_field->is_base_bound(pos) && _field->get_d_score(player) == 0)
					{
						_field->undo_step();
						continue;
					}
#endif

					if (_field->get_d_score(player) > 0)
						add_trajectory(_field->pointsSeq.end() - (_depth[player] - depth), _field->pointsSeq.end(), player);
					else if (depth > 0)
						build_trajectories_recursive(depth - 1, player);

					_field->undo_step();
				}
			}
		}
	}
	inline void project(Trajectory* trajectory)
	{
		for (auto j = trajectory->begin(); j != trajectory->end(); j++)
			_trajectories_board[*j]++;
	}
	// Проецирует траектории на доску TrajectoriesBoard (для каждой точки Pos очередной траектории инкрементирует TrajectoriesBoard[Pos]).
	// Для оптимизации в данной реализации функции не проверяются траектории на исключенность (поле Excluded).
	inline void project(Player player)
	{
		for (auto i = _trajectories[player].begin(); i != _trajectories[player].end(); i++)
			if (!i->excluded())
				project(&(*i));
	}
	inline void project()
	{
		project(player_red);
		project(player_black);
	}
	inline void unproject(Trajectory* trajectory)
	{
		for (auto j = trajectory->begin(); j != trajectory->end(); j++)
			_trajectories_board[*j]--;
	}
	// Удаляет проекцию траекторий с доски TrajectoriesBoard.
	inline void unproject(Player player)
	{
		for (auto i = _trajectories[player].begin(); i != _trajectories[player].end(); i++)
			if (!i->excluded())
				unproject(&(*i));
	}
	inline void unproject()
	{
		unproject(player_red);
		unproject(player_black);
	}
	inline void include_all_trajectories(Player player)
	{
		for (auto i = _trajectories[player].begin(); i != _trajectories[player].end(); i++)
			i->include();
	}
	inline void include_all_trajectories()
	{
		include_all_trajectories(player_red);
		include_all_trajectories(player_black);
	}
	// Возвращает хеш Зобриста пересечения двух траекторий.
	inline Hash get_intersect_hash(Trajectory* t1, Trajectory* t2)
	{
		Hash result_hash = t1->getHash();
		for (auto i = t2->begin(); i != t2->end(); i++)
			if (find(t1->begin(), t1->end(), *i) == t1->end())
				result_hash ^= _zobrist->getHash(*i);
		return result_hash;
	}
	bool exclude_unnecessary_trajectories(Player player)
	{
		bool need_exclude = false;

		for (auto i = _trajectories[player].begin(); i != _trajectories[player].end(); i++)
		{
			if (i->excluded())
				continue;
			// Считаем в c количество точек, входящих только в эту траекторию.
			uint c = 0;
			for (auto j = i->begin(); j != i->end(); j++)
				if (_trajectories_board[*j] == 1)
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
	inline void exclude_unnecessary_trajectories()
	{
		while (exclude_unnecessary_trajectories(player_red) || exclude_unnecessary_trajectories(player_black));
	}
	// Исключает составные траектории.
	void exclude_composite_trajectories(Player player)
	{
		list<Trajectory>::iterator i, j, k;

		for (k = _trajectories[player].begin(); k != _trajectories[player].end(); k++)
			for (i = _trajectories[player].begin(); i != --_trajectories[player].end(); i++)
				if (k->size() > i->size())
					for (j = i, j++; j != _trajectories[player].end(); j++)
						if (k->size() > j->size() && k->getHash() == get_intersect_hash(&(*i), &(*j)))
							k->exclude();
	}
	inline void exclude_composite_trajectories()
	{
		exclude_composite_trajectories(player_red);
		exclude_composite_trajectories(player_black);
	}
	inline void get_points(list<Pos>* moves, Player player)
	{
		for (auto i = _trajectories[player].begin(); i != _trajectories[player].end(); i++)
			if (!i->excluded())
				for (auto j = i->begin(); j != i->end(); j++)
					if (find(moves->begin(), moves->end(), *j) == moves->end())
						moves->push_back(*j);
	}
	Score calculate_max_score(Player player, size_t depth)
	{
		Score result = _field->get_score(player);
		if (depth > 0)
		{
			for (auto i = _moves[player].begin(); i != _moves[player].end(); i++)
				if (_field->puttingAllow(*i))
				{
					_field->do_unsafe_step(*i, player);
					if (_field->get_d_score(player) >= 0)
					{
						Score cur_score = calculate_max_score(player, depth - 1);
						if (cur_score > result)
							result = cur_score;
					}
					_field->undo_step();
				}
		}
		return result;
	}

public:
	inline Trajectories(Field* field, int* empty_board = NULL, size_t depth = 0)
	{
		_field = field;
		_depth[get_cur_player()] = (depth + 1) / 2;
		_depth[get_enemy_player()] = depth / 2;
		if (empty_board == NULL)
		{
			_trajectories_board = new int[field->getLength()];
			fill_n(_trajectories_board, _field->getLength(), 0);
			_trajectories_board_owner = true;
		}
		else
		{
			_trajectories_board = empty_board;
			_trajectories_board_owner = false;
		}
		_zobrist = &field->get_zobrist();
	}
	~Trajectories()
	{
		if (_trajectories_board_owner)
			delete _trajectories_board;
	}
	inline Player get_cur_player()
	{
		return _field->get_player();
	}
	inline Player get_enemy_player()
	{
		return next_player(_field->get_player());
	}
	inline void clear(Player player)
	{
		_trajectories[player].clear();
	}
	inline void clear()
	{
		clear(player_red);
		clear(player_black);
	}
	inline void build_trajectories(Player player)
	{
		if (_depth[player] > 0)
			build_trajectories_recursive(_depth[player] - 1, player);
	}
	void calculate_moves()
	{
		exclude_composite_trajectories();
		// Проецируем неисключенные траектории на доску.
		project();
		// Исключаем те траектории, у которых имеется более одной точки, принадлежащей только ей.
		exclude_unnecessary_trajectories();
		// Получаем список точек, входящих в оставшиеся неисключенные траектории.
		get_points(&_moves[player_red], player_red);
		get_points(&_moves[player_black], player_black);
		_all_moves.assign(_moves[player_red].begin(), _moves[player_red].end());
		for (auto i = _moves[player_black].begin(); i != _moves[player_black].end(); i++)
			if (find(_all_moves.begin(), _all_moves.end(), *i) == _all_moves.end())
				_all_moves.push_back(*i);
#if ALPHABETA_SORT
		_all_moves.sort([&](pos x, pos y){ return _trajectories_board[x] < _trajectories_board[y]; });
#endif
		// Очищаем доску от проекций.
		unproject();
		// После получения списка ходов обратно включаем в рассмотрение все траектории (для следующего уровня рекурсии).
		include_all_trajectories();
	}
	inline void build_trajectories()
	{
		build_trajectories(get_cur_player());
		build_trajectories(get_enemy_player());
		calculate_moves();
	}
	void build_trajectories(Trajectories* last, Pos pos)
	{
		_depth[get_cur_player()] = last->_depth[get_cur_player()];
		_depth[get_enemy_player()] = last->_depth[get_enemy_player()] - 1;

		if (_depth[get_cur_player()] > 0)
			build_trajectories_recursive(_depth[get_cur_player()] - 1, get_cur_player());

		if (_depth[get_enemy_player()] > 0)
			for (auto i = last->_trajectories[get_enemy_player()].begin(); i != last->_trajectories[get_enemy_player()].end(); i++)
				if ((i->size() <= _depth[get_enemy_player()] || (i->size() == _depth[get_enemy_player()] + 1 && find(i->begin(), i->end(), pos) != i->end())) && i->isValid(_field, pos))
					add_trajectory(&(*i), pos, get_enemy_player());

		calculate_moves();
	}
	// Строит траектории с учетом предыдущих траекторий и того, что последний ход был сделан не на траектории (или не сделан вовсе).
	void build_trajectories(Trajectories* last)
	{
		_depth[get_cur_player()] = last->_depth[get_cur_player()];
		_depth[get_enemy_player()] = last->_depth[get_enemy_player()] - 1;

		if (_depth[get_cur_player()] > 0)
			for (auto i = last->_trajectories[get_cur_player()].begin(); i != last->_trajectories[get_cur_player()].end(); i++)
				add_trajectory(&(*i), get_cur_player());

		if (_depth[get_enemy_player()] > 0)
			for (auto i = last->_trajectories[get_enemy_player()].begin(); i != last->_trajectories[get_enemy_player()].end(); i++)
				if (i->size() <= _depth[get_enemy_player()])
					add_trajectory(&(*i), get_enemy_player());

		calculate_moves();
	}
	inline void sort_moves(vector<Pos>* moves) const
	{
		// Сортируем точки по убыванию количества траекторий, в которые они входят.
		sort(moves->begin(), moves->end(), [&](Pos x, Pos y){ return _trajectories_board[x] < _trajectories_board[y]; });
	}
	inline void sort_moves(list<Pos>* moves) const
	{
		// Сортируем точки по убыванию количества траекторий, в которые они входят.
		moves->sort([&](Pos x, Pos y){ return _trajectories_board[x] < _trajectories_board[y]; });
	}
	// Получить список ходов.
	inline list<Pos>* get_points()
	{
		return &_all_moves;
	}
	inline Score get_max_score(Player player)
	{
		return calculate_max_score(player, _depth[player]) + _depth[next_player(player)];
	}
};
