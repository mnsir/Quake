#include "app.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdexcept>


const char * App_GetBaseDir()
{
    static char cwd[1024] = {0};
    if (!cwd[0] && !GetCurrentDirectory(sizeof(cwd), cwd))
    {
        throw std::runtime_error("Couldn't determine current directory");
    }

    //if (cwd[Q_strlen(cwd) - 1] == '/')
    //    cwd[Q_strlen(cwd) - 1] = 0;

    return cwd;
}


const char * App_GetCacheDir()
{
    // for development over ISDN lines
    return nullptr;
}
