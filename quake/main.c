#include <dll_func_typedefs.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define LIBNAME_HW "QuakeHW"
#define LIBNAME_GL "QuakeGL"

#pragma comment(lib, LIBNAME_HW ".lib")
#pragma comment(lib, LIBNAME_GL ".lib")


typedef enum
{
    hw = 0,
    gl,
} VideoMode;


const char * const aLibFileName[] = {
    LIBNAME_HW ".dll",
    LIBNAME_GL ".dll",
};


const char * const aArgs[] = {
    "-startwindowed",
    "-width 800 -height 600 -bpp 32 -window -no8bit"
};


HMODULE hModule = NULL;
FromLibFunc FromLib = NULL;


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    BOOL res = TRUE;

    const VideoMode mode = gl;

    hModule = LoadLibrary(aLibFileName[mode]);

    if (hModule)
        FromLib = GetProcAddress(hModule, "_FromLib@16");

    if (FromLib)
        res = FromLib(hInstance, hPrevInstance, aArgs[mode], nShowCmd);
    else
        MessageBox(NULL, "FromLib not loaded", "FromLib not loaded!", 0);

    FromLib = NULL;
    if (hModule)
    {
        FreeLibrary(hModule);
        hModule = NULL;
    }

    return res;
}

