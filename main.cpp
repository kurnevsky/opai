#include "config.h"
#include "field.h"
#include "basic_types.h"
#include "bot.h"
#include <iostream>
#include <string>
#include <map>

Bot *bot;

void gen_move(int id)
{
	Player player;
	Coord x, y;

	cin >> player;
	if (bot == NULL)
	{
		cout << "?" << " " << id << " " << "gen_move" << endl;
	}
	else
	{
		bot->setPlayer(player);
		bot->get(x, y);
		if (x == -1 || y == -1)
			cout << "?" << " " << id << " " << "gen_move" << endl;
		else
			cout << "=" << " " << id << " " << "gen_move" << " " << x << " " << y << " " << player << endl;
	}
}

void gen_move_with_complexity(int id)
{
	Player player;
	Coord x, y;
	int complexity;

	cin >> player >> complexity;
	if (bot == NULL)
	{
		cout << "?" << " " << id << " " << "gen_move_with_complexity" << endl;
	}
	else
	{
		bot->setPlayer(player);
		bot->getWithComplexity(x, y, complexity);
		if (x == -1 || y == -1)
			cout << "?" << " " << id << " " << "gen_move_with_complexity" << endl;
		else
			cout << "=" << " " << id << " " << "gen_move_with_complexity" << " " << x << " " << y << " " << player << endl;
	}
}

void gen_move_with_time(int id)
{
	Player player;
	Coord x, y;
	Time time;

	cin >> player >> time;
	if (bot == NULL)
	{
		cout << "?" << " " << id << " " << "gen_move_with_time" << endl;
	}
	else
	{
		bot->setPlayer(player);
		bot->getWithTime(x, y, time);
		if (x == -1 || y == -1)
			cout << "?" << " " << id << " " << "gen_move_with_time" << endl;
		else
			cout << "=" << " " << id << " " << "gen_move_with_time" << " " << x << " " << y << " " << player << endl;
	}
}

void init(int id)
{
	Coord x, y;
	ptrdiff_t seed;

	cin >> x >> y >> seed;
	// Если существовало поле - удаляем его.
	if (bot != NULL)
		delete bot;
	bot = new Bot(x, y, BEGIN_PATTERN_CLEAN, seed);
	cout << "=" << " " << id << " " << "init" << endl;
}

void list_commands(int id)
{
	cout << "=" << " " << id << " " << "list_commands" << " " << "gen_move gen_move_with_complexity gen_move_with_time init list_commands name play quit undo version" << endl;
}

void name(int id)
{
	cout << "=" << " " << id << " " << "name" << " " << "Open Points Artifical Intelligence" << endl;
}

void play(int id)
{
	Coord x, y;
	Player player;

	cin >> x >> y >> player;
	if (bot == NULL || !bot->doStep(x, y, player))
		cout << "?" << " " << id << " " << "play" << endl;
	else
		cout << "=" << " " << id << " " << "play" << " " << x << " " << y << " " << player << endl;
}

void quit(int id)
{
	if (bot != NULL)
		delete bot;
	cout << "=" << " " << id << " " << "quit" << endl;
	exit(0);
}

void undo(int id)
{
	if (bot == NULL || !bot->undoStep())
		cout << "?" << " " << id << " " << "undo" << endl;
	else
		cout << "=" << " " << id << " " << "undo" << endl;
}

void version(int id)
{
	cout << "=" << " " << id << " " << "version" << " " << "2013.2.10" << endl;
}

inline void fill_codes(map<string, void(*)(int)> &codes)
{
	codes["gen_move"] = gen_move;
	codes["gen_move_with_complexity"] = gen_move_with_complexity;
	codes["gen_move_with_time"] = gen_move_with_time;
	codes["init"] = init;
	codes["list_commands"] = list_commands;
	codes["name"] = name;
	codes["play"] = play;
	codes["quit"] = quit;
	codes["undo"] = undo;
	codes["version"] = version;
}

int main()
{
	string s;
	int id;
	map<string, void(*)(int)> codes;

	bot = NULL;
	fill_codes(codes);

	while (true)
	{
		cin >> id >> s;
		auto i = codes.find(s);
		if (i != codes.end())
			i->second(id);
		else
			cout << "?" << " " << id << " " << s << endl;
		// Очищаем cin до конца строки.
		while (cin.get() != 10) {}
	}
}
