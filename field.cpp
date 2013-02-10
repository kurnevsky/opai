#include "field.h"

using namespace std;

short Field::get_input_points(const Pos center_pos, const PosValue enable_cond, Pos inp_chain_points[], Pos inp_sur_points[]) const
{
	short result = 0;

	if (is_not_enable(w(center_pos), enable_cond))
	{
		if (is_enable(nw(center_pos), enable_cond))
		{
			inp_chain_points[0] = nw(center_pos);
			inp_sur_points[0] = w(center_pos);
			result++;
		}
		else if (is_enable(n(center_pos), enable_cond))
		{
			inp_chain_points[0] = n(center_pos);
			inp_sur_points[0] = w(center_pos);
			result++;
		}
	}

	if (is_not_enable(s(center_pos), enable_cond))
	{
		if (is_enable(sw(center_pos), enable_cond))
		{
			inp_chain_points[result] = sw(center_pos);
			inp_sur_points[result] = s(center_pos);
			result++;
		}
		else if (is_enable(w(center_pos), enable_cond))
		{
			inp_chain_points[result] = w(center_pos);
			inp_sur_points[result] = s(center_pos);
			result++;
		}
	}

	if (is_not_enable(e(center_pos), enable_cond))
	{
		if (is_enable(se(center_pos), enable_cond))
		{
			inp_chain_points[result] = se(center_pos);
			inp_sur_points[result] = e(center_pos);
			result++;
		}
		else if (is_enable(s(center_pos), enable_cond))
		{
			inp_chain_points[result] = s(center_pos);
			inp_sur_points[result] = e(center_pos);
			result++;
		}
	}

	if (is_not_enable(n(center_pos), enable_cond))
	{
		if (is_enable(ne(center_pos), enable_cond))
		{
			inp_chain_points[result] = ne(center_pos);
			inp_sur_points[result] = n(center_pos);
			result++;
		}
		else if (is_enable(e(center_pos), enable_cond))
		{
			inp_chain_points[result] = e(center_pos);
			inp_sur_points[result] = n(center_pos);
			result++;
		}
	}

	return result;
}

void Field::place_begin_pattern(BeginPattern pattern)
{
	switch (pattern)
	{
	case (BEGIN_PATTERN_CROSSWIRE):
		do_step(to_pos(_width / 2 - 1, _height / 2 - 1));
		do_step(to_pos(_width / 2, _height / 2 - 1));
		do_step(to_pos(_width / 2, _height / 2));
		do_step(to_pos(_width / 2 - 1, _height / 2));
		break;
	case (BEGIN_PATTERN_SQUARE):
		do_step(to_pos(_width / 2 - 1, _height / 2 - 1));
		do_step(to_pos(_width / 2, _height / 2 - 1));
		do_step(to_pos(_width / 2 - 1, _height / 2));
		do_step(to_pos(_width / 2, _height / 2));
		break;
    case (BEGIN_PATTERN_CLEAN):
        break;
	}
}

void Field::remove_empty_base(const Pos start_pos)
{
	wave(	start_pos,
			[&](Pos pos)->bool
			{
				if (is_in_empty_base(pos))
				{
					_changes.back().changes.push(pair<Pos, PosValue>(pos, _points[pos] & !tag_bit));
					clear_empty_base(pos);
					return true;
				}
				else
				{
					return false;
				}
			});
}

bool Field::build_chain(const Pos start_pos, const PosValue enable_cond, const Pos direction_pos, list<Pos> &chain)
{
	chain.clear();
	chain.push_back(start_pos);
	Pos pos = direction_pos;
	Pos center_pos = start_pos;
	// Площадь базы.
	int TempSquare = square(center_pos, pos);
	do
	{
		if (is_tagged(pos))
		{
			while (chain.back() != pos)
			{
				clear_tag(chain.back());
				chain.pop_back();
			}
		}
		else
		{
			set_tag(pos);
			chain.push_back(pos);
		}
		swap(pos, center_pos);
		get_first_next_pos(center_pos, pos);
		while (is_not_enable(pos, enable_cond))
			get_next_pos(center_pos, pos);
		TempSquare += square(center_pos, pos);
	}
	while (pos != start_pos);

	for (auto i = chain.begin(); i != chain.end(); i++)
		clear_tag(*i);

	return (TempSquare < 0 && chain.size() > 2);
}

void Field::find_surround(list<Pos> &chain, Pos inside_point, Player player)
{
	// Количество захваченных точек.
	int cur_capture_count = 0;
	// Количество захваченных пустых полей.
	int cur_freed_count = 0;

	list<Pos> sur_points;

	// Помечаем точки цепочки.
	for (auto i = chain.begin(); i != chain.end(); i++)
		set_tag(*i);

	wave(	inside_point,
			[&, player](Pos pos)->bool
			{
				if (is_not_bound(pos, player | put_bit | bound_bit))
				{
					check_captured_and_freed(pos, player, cur_capture_count, cur_freed_count);
					sur_points.push_back(pos);
					return true;
				}
				else
				{
					return false;
				}
			});
	// Изменение счета игроков.
	add_sub_captured_freed(player, cur_capture_count, cur_freed_count);

#if SUR_COND != 1
	if (cur_capture_count != 0) // Если захватили точки.
#endif
	{
		for (auto i = chain.begin(); i != chain.end(); i++)
		{
			clear_tag(*i);
			// Добавляем в список изменений точки цепочки.
			_changes.back().changes.push(pair<Pos, PosValue>(*i, _points[*i]));
			// Помечаем точки цепочки.
			set_base_bound(*i);
		}

		for (auto i = sur_points.begin(); i != sur_points.end(); i++)
		{
			_changes.back().changes.push(pair<Pos, PosValue>(*i, _points[*i]));

			set_capture_free_state(*i, player);
		}
	}
	else // Если ничего не захватили.
	{
		for (auto i = chain.begin(); i != chain.end(); i++)
			clear_tag(*i);

		for (auto i = sur_points.begin(); i != sur_points.end(); i++)
		{
			_changes.back().changes.push(pair<Pos, PosValue>(*i, _points[*i]));

			if (!is_putted(*i))
				set_empty_base(*i);
		}
	}
}

Field::Field(const Coord width, const Coord height, const BeginPattern begin_pattern, zobrist* zobr)
{
	_width = width;
	_height = height;
	_player = player_red;
	_capture_count[player_red] = 0;
	_capture_count[player_black] = 0;

	_points = new PosValue[length()];
	fill_n(_points, length(), 0);

	for (Coord x = -1; x <= width; x++)
	{
		set_bad(to_pos(x, -1));
		set_bad(to_pos(x, height));
	}
	for (Coord y = -1; y <= height; y++)
	{
		set_bad(to_pos(-1, y));
		set_bad(to_pos(width, y));
	}

	_changes.reserve(length());
	points_seq.reserve(length());

	_zobrist = zobr;
	_hash = 0;

	place_begin_pattern(begin_pattern);
}

Field::Field(const Field &orig)
{
	_width = orig._width;
	_height = orig._height;
	_player = orig._player;
	_capture_count[player_red] = orig._capture_count[player_red];
	_capture_count[player_black] = orig._capture_count[player_black];

	_points = new PosValue[length()];
	copy_n(orig._points, length(), _points);

	_changes.reserve(length());
	points_seq.reserve(length());

	_changes.assign(orig._changes.begin(), orig._changes.end());
	points_seq.assign(orig.points_seq.begin(), orig.points_seq.end());

	_zobrist = orig._zobrist;
	_hash = orig._hash;
}

Field::~Field()
{
	delete _points;
}

void Field::wave(Pos start_pos, function<bool(Pos)> cond)
{
	// Очередь для волнового алгоритма (обхода в ширину).
	list<Pos> q;

	if (!cond(start_pos))
		return;

	q.push_back(start_pos);
	set_tag(start_pos);
	auto it = q.begin();
	while (it != q.end())
	{
		if (!is_tagged(w(*it)) && cond(w(*it)))
		{
			q.push_back(w(*it));
			set_tag(w(*it));
		}
		if (!is_tagged(n(*it)) && cond(n(*it)))
		{
			q.push_back(n(*it));
			set_tag(n(*it));
		}
		if (!is_tagged(e(*it)) && cond(e(*it)))
		{
			q.push_back(e(*it));
			set_tag(e(*it));
		}
		if (!is_tagged(s(*it)) && cond(s(*it)))
		{
			q.push_back(s(*it));
			set_tag(s(*it));
		}
		it++;
	}

	for (it = q.begin(); it != q.end(); it++)
		clear_tag(*it);
}

bool Field::is_point_inside_ring(const Pos pos, const list<Pos> &ring) const
{
	int intersections = 0;

	IntersectionState state = INTERSECTION_STATE_NONE;

	for (auto i = ring.begin(); i != ring.end(); i++)
	{
		switch (get_intersection_state(pos, *i))
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
		IntersectionState begin_state = get_intersection_state(pos, *i);
		while (begin_state == INTERSECTION_STATE_TARGET)
		{
			i++;
			begin_state = get_intersection_state(pos, *i);
		}
		if ((state == INTERSECTION_STATE_UP && begin_state == INTERSECTION_STATE_DOWN) ||
			(state == INTERSECTION_STATE_DOWN && begin_state == INTERSECTION_STATE_UP))
			intersections++;
	}

	return intersections % 2 == 1;
}

void Field::check_closure(const Pos start_pos, Player player)
{
	short inp_points_count;
	Pos inp_chain_points[4], inp_sur_points[4];

	list<Pos> chain;

	if (is_in_empty_base(start_pos)) // Если точка поставлена в пустую базу.
	{
		// Проверяем, в чью пустую базу поставлена точка.
		Pos pos = start_pos - 1;
		while (!is_putted(pos))
			pos--;

		if (get_player(pos) == get_player(start_pos)) // Если поставили в свою пустую базу.
		{
			clear_empty_base(start_pos);
			return;
		}

#if SUR_COND != 2 // Если приоритет не всегда у врага.
		inp_points_count = get_input_points(start_pos, player | put_bit, inp_chain_points, inp_sur_points);
		if (inp_points_count > 1)
		{
			short chains_count = 0;
			for (short i = 0; i < inp_points_count; i++)
				if (build_chain(start_pos, get_player(start_pos) | put_bit, inp_chain_points[i], chain))
				{
					find_surround(chain, inp_sur_points[i], player);
					chains_count++;
					if (chains_count == inp_points_count - 1)
						break;
				}
				if (is_base_bound(start_pos))
				{
					remove_empty_base(start_pos);
					return;
				}
		}
#endif

		pos++;
		do
		{
			pos--;
			while (!is_enable(pos, next_player(player) | put_bit))
				pos--;
			inp_points_count = get_input_points(pos, next_player(player) | put_bit, inp_chain_points, inp_sur_points);
			for (short i = 0; i < inp_points_count; i++)
				if (build_chain(pos, next_player(player) | put_bit, inp_chain_points[i], chain))
					if (is_point_inside_ring(start_pos, chain))
					{
						find_surround(chain, inp_sur_points[i], next_player(player));
						break;
					}
		} while (!is_captured(start_pos));
	}
	else
	{
		inp_points_count = get_input_points(start_pos, player | put_bit, inp_chain_points, inp_sur_points);
		if (inp_points_count > 1)
		{
			short chains_count = 0;
			for (short i = 0; i < inp_points_count; i++)
				if (build_chain(start_pos, player | put_bit, inp_chain_points[i], chain))
				{
					find_surround(chain, inp_sur_points[i], player);
					chains_count++;
					if (chains_count == inp_points_count - 1)
						break;
				}
		}
	}
}

//bool Field::check_closure(const Pos start_pos, const Pos checked_pos, Player player)
//{
//	Pos InpChainPoints[4], InpSurPoints[4];
//
//	list<Pos> chain;
//
//	short InpPointsCount = get_input_points(start_pos, cur_player | put_bit, InpChainPoints, InpSurPoints);
//	if (InpPointsCount > 1)
//	{
//		for (ushort i = 0; i < InpPointsCount; i++)
//			if (build_chain(start_pos, get_player(start_pos) | put_bit, InpChainPoints[i], chain))
//				if (is_point_inside_ring(checked_pos, chain))
//				{
//					for (auto j = chain.begin(); j != chain.end(); j++)
//					{
//						// Добавляем в список изменений точки цепочки.
//						_changes.back().changes.push(pair<pos, value>(*j, _points[*j]));
//						// Помечаем точки цепочки.
//						set_base_bound(*j);
//					}
//					return true;
//				}
//	}
//
//	return false;
//}
