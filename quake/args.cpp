#include "args.h"

#include "app.h"

#include <ranges>
#include <sstream>
#include <string>
#include <vector>
#include <optional>
#include <cstdarg>

namespace
{

// TODO сдделать с этим что-нибудь
int Q_atoi(const char * str)
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

float Q_atof(const char* str)
{
    double val;
    int sign;
    int c;
    int decimal, total;

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
                val = (val * 16) + c - '0';
            else if (c >= 'a' && c <= 'f')
                val = (val * 16) + c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                val = (val * 16) + c - 'A' + 10;
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
    decimal = -1;
    total = 0;
    while (1)
    {
        c = *str++;
        if (c == '.')
        {
            decimal = total;
            continue;
        }
        if (c < '0' || c > '9')
            break;
        val = val * 10 + c - '0';
        total++;
    }

    if (decimal == -1)
        return val * sign;
    while (total > decimal)
    {
        val /= 10;
        total--;
    }

    return val * sign;
}

}

std::vector<std::string> args_;

void InitCommandLine(const char * line)
{
    // TODO не забыть сохранить аргументы в cvar

    std::istringstream ss(line);
    std::vector<std::string> args;
    std::ranges::copy(std::views::istream<std::string>(ss), std::back_inserter(args));
    args_ = std::move(args);

    if (Args_GetIndex("-safe"))
    {
        for (auto && str : {"-stdvid", "-nolan", "-nosound", "-nocdaudio", "-nojoy", "-nomouse", "-dibonly"})
            args_.emplace_back(str);
    }
}


void Args_Reset(const char ** ptr, int size)
{
    args_.clear();
    for (auto && i : std::views::iota(0, size))
    {
        args_.emplace_back(ptr[i]);
    }
}


int Args_GetIndex(const char * str)
{
    auto it = std::ranges::find(args_, std::string_view(str));
    return it != args_.end() ? std::distance(args_.begin(), it) + 1 : 0;
}

const char * Args_GetByIndex(int index)
{
    static const char empty[] = "";
    return index ? args_[index - 1].c_str() : empty;
}

int Args_GetCount()
{
    return args_.size() + 1;
}




std::optional<int> Args_GetInt(std::string_view arg)
{
    if (auto it = std::ranges::find(args_, arg); it != args_.end())
        if (it = std::next(it); it != args_.end())
            return Q_atoi(it->c_str());
    return {};
}


std::optional<double> Args_GetDouble(std::string_view arg)
{
    if (auto it = std::ranges::find(args_, arg); it != args_.end())
        if (it = std::next(it); it != args_.end())
            return Q_atof(it->c_str());
    return {};
}


int Args_NoAudio()
{
    return Args_GetIndex("-nocdaudio");
}

const char * Args_BaseDir()
{
    const char * res = nullptr;
    int i = Args_GetIndex("-basedir");
    if (i && i < Args_GetCount() - 1)
        res = Args_GetByIndex(i + 1);
    else
        res = App_GetBaseDir(); // TODO
    return res;
}


const char * Args_CacheDir()
{
    static const char empty[] = "";
    int i = Args_GetIndex("-cachedir");
    if (i && i < Args_GetCount() - 1)
    {
        auto next = Args_GetByIndex(i + 1);
        if (next[0] == '-')
            return empty;
        else
            return next;
    }
    else if (auto def = App_GetCacheDir())
        return def;
    else
        return empty;
}


int Args_Rogue()
{
    return Args_GetIndex("-rogue");
}


int Args_Hipnotic()
{
    return Args_GetIndex("-hipnotic");
}


int Args_Dedicated()
{
    return Args_GetIndex("-dedicated");
}
