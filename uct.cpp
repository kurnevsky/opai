#include "config.h"
#include "basic_types.h"
#include "uct.h"
#include "player.h"
#include "field.h"
#include "time.h"
#include <limits>
#include <queue>
#include <vector>
#include <algorithm>
#include <omp.h>

using namespace std;
using namespace boost;

short play_random_game(Field* field, mt* gen, vector<Pos>* possible_moves)
{
	vector<Pos> moves(possible_moves->size());
	size_t putted = 0;
	Player result;

	moves[0] = (*possible_moves)[0];
	for (size_t i = 1; i < possible_moves->size(); i++)
	{
		random::uniform_int_distribution<size_t> dist(0, i);
		size_t j = dist(*gen);
		moves[i] = moves[j];
		moves[j] = (*possible_moves)[i];
	}

	for (auto i = moves.begin(); i < moves.end(); i++)
		if (field->puttingAllow(*i))
		{
			field->do_unsafe_step(*i);
			putted++;
		}

	if (field->getScore(player_red) > 0)
		result = player_red;
	else if (field->getScore(player_black) > 0)
		result = player_black;
	else
		result = -1;

	for (size_t i = 0; i < putted; i++)
		field->undo_step();

	return result;
}

void create_children(Field* field, vector<Pos>* possible_moves, uct_node* n)
{
	uct_node** cur_child = &n->child;

	for (auto i = possible_moves->begin(); i < possible_moves->end(); i++)
		if (field->puttingAllow(*i))
		{
			*cur_child = new uct_node();
			(*cur_child)->move = *i;
			cur_child = &(*cur_child)->sibling;
		}
}

uct_node* uct_select(mt* gen, uct_node* n)
{
	double bestuct = 0, winrate, uct, uctvalue;
	uct_node* result = NULL;
	uct_node* next = n->child;
	while (next != NULL)
	{
		if (next->visits > 0)
		{
			winrate = static_cast<double>(next->wins)/next->visits;
			uct = UCTK * sqrt(log(static_cast<double>(n->visits)) / (5 * next->visits));
			uctvalue = winrate + uct;
		}
		else
		{
			random::uniform_int_distribution<int> dist(0, 999);
			uctvalue = 10000 + dist(*gen);
		}

		if (uctvalue > bestuct)
		{
			bestuct = uctvalue;
			result = next;
		}

		next = next->sibling;
	}

	return result;
}

short play_simulation(Field* field, mt* gen, vector<Pos>* possible_moves, uct_node* n)
{
	short randomresult;

	if (n->visits == 0)
	{
		randomresult = play_random_game(field, gen, possible_moves);
	}
	else
	{
		if (n->child == NULL)
			create_children(field, possible_moves, n);

		uct_node* next = uct_select(gen, n);

		if (next == NULL)
		{
			n->visits = numeric_limits<ulong>::max();
			if (field->getScore(next_player(field->getPlayer())) > 0)
				n->wins = numeric_limits<ulong>::max();

			if (field->getScore(player_red) > 0)
				return player_red;
			else if (field->getScore(player_black) > 0)
				return player_black;
			else
				return -1;
		}

		field->do_unsafe_step(next->move);

		randomresult = play_simulation(field, gen, possible_moves, next);

		field->undo_step();
	}

	n->visits++;
	if (randomresult == next_player(field->getPlayer()))
		n->wins++;

	return randomresult;
}

template<typename _Cont> void generate_possible_moves(Field* field, _Cont* possible_moves)
{
	ushort* r_field = new ushort[field->getLength()];
	fill_n(r_field, field->getLength(), 0);
	std::queue<Pos> q;

	possible_moves->clear();
	for (Pos i = field->min_pos(); i <= field->max_pos(); i++)
		if (field->isPutted(i)) //TODO: Класть соседей, а не сами точки.
			q.push(i);

	while (!q.empty())
	{
		if (field->puttingAllow(q.front())) //TODO: Убрать условие.
			possible_moves->push_back(q.front());
		if (r_field[q.front()] < UCT_RADIUS)
		{
			if (field->puttingAllow(field->n(q.front())) && r_field[field->n(q.front())] == 0)
			{
				r_field[field->n(q.front())] = r_field[q.front()] + 1;
				q.push(field->n(q.front()));
			}
			if (field->puttingAllow(field->s(q.front())) && r_field[field->s(q.front())] == 0)
			{
				r_field[field->s(q.front())] = r_field[q.front()] + 1;
				q.push(field->s(q.front()));
			}
			if (field->puttingAllow(field->w(q.front())) && r_field[field->w(q.front())] == 0)
			{
				r_field[field->w(q.front())] = r_field[q.front()] + 1;
				q.push(field->w(q.front()));
			}
			if (field->puttingAllow(field->e(q.front())) && r_field[field->e(q.front())] == 0)
			{
				r_field[field->e(q.front())] = r_field[q.front()] + 1;
				q.push(field->e(q.front()));
			}
		}
		q.pop();
	}
	delete r_field;
}

void final_uct(uct_node* n)
{
	if (n->child != NULL)
		final_uct(n->child);
	if (n->sibling != NULL)
		final_uct(n->sibling);
	delete n;
}

Pos uct(Field* field, mt* gen, size_t max_simulations)
{
	// Список всех возможных ходов для UCT.
	vector<Pos> moves;
	double best_estimate = 0;
	Pos result = -1;

	generate_possible_moves(field, &moves);

	if (static_cast<size_t>(omp_get_max_threads()) > moves.size())
		omp_set_num_threads(moves.size());
	#pragma omp parallel
	{
		uct_node n;
		Field* local_field = new Field(*field);
		random::uniform_int_distribution<size_t> local_dist(numeric_limits<size_t>::min(), numeric_limits<size_t>::max());
		mt* local_gen;
		#pragma omp critical
		{
			local_gen = new mt(local_dist(*gen));
		}

		uct_node** cur_child = &n.child;
		for (auto i = moves.begin() + omp_get_thread_num(); i < moves.end(); i += omp_get_num_threads())
		{
			*cur_child = new uct_node();
			(*cur_child)->move = *i;
			cur_child = &(*cur_child)->sibling;
		}

		#pragma omp for
		for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(max_simulations); i++)
			play_simulation(local_field, local_gen, &moves, &n);

		#pragma omp critical
		{
			uct_node* next = n.child;
			while (next != NULL)
			{
				double cur_estimate = static_cast<double>(next->wins) / next->visits;
				if (cur_estimate > best_estimate)
				{
					best_estimate = cur_estimate;
					result = next->move;
				}
				next = next->sibling;
			}
		}

		if (n.child != NULL)
			final_uct(n.child);
		delete local_gen;
		delete local_field;
	}

	return result;
}

Pos uct_with_time(Field* field, mt* gen, Time time)
{
	// Список всех возможных ходов для UCT.
	vector<Pos> moves;
	auto best_estimate = 0;
	auto result = -1;
	Timer t;

	generate_possible_moves(field, &moves);

	if (static_cast<size_t>(omp_get_max_threads()) > moves.size())
		omp_set_num_threads(moves.size());
	#pragma omp parallel
	{
		uct_node n;
		Field* local_field = new Field(*field);
		random::uniform_int_distribution<size_t> local_dist(numeric_limits<size_t>::min(), numeric_limits<size_t>::max());
		mt* local_gen;
		#pragma omp critical
		{
			local_gen = new mt(local_dist(*gen));
		}

		uct_node** cur_child = &n.child;
		for (auto i = moves.begin() + omp_get_thread_num(); i < moves.end(); i += omp_get_num_threads())
		{
			*cur_child = new uct_node();
			(*cur_child)->move = *i;
			cur_child = &(*cur_child)->sibling;
		}

		while (t.get() < time)
			for (size_t i = 0; i < UCT_ITERATIONS_BEFORE_CHECK_TIME; i++)
				play_simulation(local_field, local_gen, &moves, &n);

		#pragma omp critical
		{
			uct_node* next = n.child;
			while (next != NULL)
			{
				double cur_estimate = static_cast<double>(next->wins) / next->visits;
				if (cur_estimate > best_estimate)
				{
					best_estimate = cur_estimate;
					result = next->move;
				}
				next = next->sibling;
			}
		}

		if (n.child != NULL)
			final_uct(n.child);
		delete local_gen;
		delete local_field;
	}

	return result;
}

