#include "args.h"

#include <ranges>
#include <sstream>
#include <string>
#include <vector>
#include <optional>


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


// TODO сдделать с этим что-нибудь
int Q_atoi(const char* str)
{
    int val;
    int sign;
    int c;

    if (*str == '-')
    {
        sign = -1;
        str++;
    }
    else
        sign = 1;

    val = 0;

    //
    // check for hex
    //
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
    {
        str += 2;
        while (1)
        {
            c = *str++;
            if (c >= '0' && c <= '9')
                val = (val << 4) + c - '0';
            else if (c >= 'a' && c <= 'f')
                val = (val << 4) + c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                val = (val << 4) + c - 'A' + 10;
            else
                return val * sign;
        }
    }

    //
    // check for character
    //
    if (str[0] == '\'')
    {
        return sign * str[1];
    }

    //
    // assume decimal
    //
    while (1)
    {
        c = *str++;
        if (c < '0' || c > '9')
            return val * sign;
        val = val * 10 + c - '0';
    }

    return 0;
}


std::optional<int> Args_GetInt(std::string_view arg)
{
	if (auto it = std::ranges::find(args_, arg); it != args_.end())
		if (it = std::next(it); it != args_.end())
			return Q_atoi(it->c_str());
	return {};
}
