#include <dll_func_typedefs.h>
#include "app.h"
#include "args.h"
#include "mode.h"
#include "memory.h"
#include "keys.h"
#include "lib_funcs.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <appapi.h>
#include <assert.h>

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
    //"-playback",
    //"-startwindowed -record",
    "-cachedir -width 800 -height 600 -bpp 32 -window -no8bit"
};


HMODULE hModule = NULL;

HINSTANCE g_hInstance;


HINSTANCE GetAppInstance()
{
    assert(g_hInstance);
    return g_hInstance;
}


AppAPI g_appApi = {
    .GetAppInstance = GetAppInstance,

    .Args_GetIndex = Args_GetIndex,
    .Args_GetByIndex = Args_GetByIndex,
    .Args_GetCount = Args_GetCount,
    .Args_Reset = Args_Reset,

    .Args_NoAudio = Args_NoAudio,
    .Args_Rogue = Args_Rogue,
    .Args_Hipnotic = Args_Hipnotic,
    .Args_BaseDir = Args_BaseDir,
    .Args_CacheDir = Args_CacheDir,

    .Mode_IsStandard = Mode_IsStandard,
    .Mode_IsRogue = Mode_IsRogue,
    .Mode_IsHipnotic = Mode_IsHipnotic,

    .Memory_GetData = Memory_GetData,
    .Memory_GetSize = Memory_GetSize,
    .Memory_GetMinimum = Memory_GetMinimum,

    .Key_GetDest = Key_GetDest,
    .Key_SetDest = Key_SetDest,
    .Key_GetBinding = Key_GetBinding,
    .Key_GetLastPress = Key_GetLastPress,
    .Key_GetCount = Key_GetCount,
    .Key_SetCount = Key_SetCount,
    .Key_GetEditLine = Key_GetEditLine,
    .Key_GetLinePos = Key_GetLinePos,
    .Key_SetLinePos = Key_SetLinePos,
    .Key_Event = Key_Event,
    .Key_ClearStates = Key_ClearStates,
    .Key_KeynumToString = Key_KeynumToString,
    .Key_SetBinding = Key_SetBinding,
    .Key_WriteBindings = Key_WriteBindings,
    .Key_SetTeamMessage = Key_SetTeamMessage,

};

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    BOOL res = TRUE;

    /* previous instances do not exist in Win32 */
    if (hPrevInstance)
        return 0;

    g_hInstance = hInstance;

    const VideoMode mode = gl;
    InitCommandLine(aArgs[mode] /*lpCmdLine*/);
    Mode_Init();
    Memory_Init();

    hModule = LoadLibrary(aLibFileName[mode]);

    if (hModule)
    {
        FromLib = GetProcAddress(hModule, "_FromLib@0");
        Initialize = GetProcAddress(hModule, "_Initialize@4");
        Cbuf_AddText = GetProcAddress(hModule, "Cbuf_AddText@4");
        Con_Printf = GetProcAddress(hModule, "Con_Printf@4");
        Cmd_AddCommand = GetProcAddress(hModule, "Con_Printf@4");
        M_Keydown = GetProcAddress(hModule, "Con_Printf@4");
        M_ToggleMenu_f = GetProcAddress(hModule, "Con_Printf@4");
        Cmd_Argc = GetProcAddress(hModule, "Con_Printf@4");
        Cmd_Argv = GetProcAddress(hModule, "Con_Printf@4");
        SCR_UpdateScreen = GetProcAddress(hModule, "Con_Printf@4");
        Cmd_CompleteCommand = GetProcAddress(hModule, "Con_Printf@4");
        Cvar_CompleteVariable = GetProcAddress(hModule, "Con_Printf@4");
        CL_IsStateDisconnected = GetProcAddress(hModule, "Con_Printf@4");
        Con_GetTotalLines = GetProcAddress(hModule, "Con_Printf@4");
        VID_GetHeight = GetProcAddress(hModule, "Con_Printf@4");
        Con_GetBackScroll = GetProcAddress(hModule, "Con_Printf@4");
        Con_SetBackScroll = GetProcAddress(hModule, "Con_Printf@4");
        CL_IsDemoPlayBack = GetProcAddress(hModule, "Con_Printf@4");
        Con_IsForcedUp = GetProcAddress(hModule, "Con_Printf@4");
    }

    if (Initialize)
    {
        Initialize(&g_appApi);

        if (FromLib)
            res = FromLib();
        else
            MessageBox(NULL, "FromLib not loaded", "FromLib not loaded!", 0);
    }
    else
        MessageBox(NULL, "Initialize not loaded", "Initialize not loaded!", 0);

    FromLib = NULL;
    Initialize = NULL;
    Cbuf_AddText = NULL;
    Con_Printf = NULL;
    Cmd_AddCommand = NULL;
    M_Keydown = NULL;
    M_ToggleMenu_f = NULL;
    Cmd_Argc = NULL;
    Cmd_Argv = NULL;
    SCR_UpdateScreen = NULL;
    Cmd_CompleteCommand = NULL;
    Cvar_CompleteVariable = NULL;
    CL_IsStateDisconnected = NULL;
    Con_GetTotalLines = NULL;
    VID_GetHeight = NULL;
    Con_GetBackScroll = NULL;
    Con_SetBackScroll = NULL;
    CL_IsDemoPlayBack = NULL;
    Con_IsForcedUp = NULL;

    if (hModule)
    {
        FreeLibrary(hModule);
        hModule = NULL;
    }

    return res;
}

