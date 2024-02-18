#include "args.h"

#include <ranges>
#include <sstream>
#include <string>
#include <vector>


std::vector<std::string> args_;

void InitCommandLine(const char* line)
{
	// TODO не забыть сохранить аргументы в cvar

	std::istringstream ss(line);
	std::vector<std::string> args;
	std::ranges::copy(std::views::istream<std::string>(ss), std::back_inserter(args));
	args_ = std::move(args);

	if (Args_GetIndex("-safe"))
	{
		for (auto&& str : { "-stdvid", "-nolan", "-nosound", "-nocdaudio", "-nojoy", "-nomouse", "-dibonly" })
			args_.emplace_back(str);
	}
}


void Args_Reset(const char** ptr, int size)
{
	args_.clear();
	for (auto&& i : std::views::iota(0, size))
	{
		args_.emplace_back(ptr[i]);
	}
}


int Args_GetIndex(const char* str)
{
	auto it = std::ranges::find(args_, std::string_view(str));
	return it != args_.end() ? std::distance(args_.begin(), it) + 1 : 0;
}

const char* Args_GetByIndex(int index)
{
	static const char empty[] = "";
	return index ? args_[index - 1].c_str() : empty;
}

int Args_GetCount()
{
	return args_.size() + 1;
}