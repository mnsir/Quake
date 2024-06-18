#include "app.h"
#include "args.h"
#include "mode.h"
#include "memory.h"
#include "keys.h"
#include "sys.h"
#include "dll.h"

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
    .Key_Get_LinePos = Key_Get_LinePos,
    .Key_Event = Key_Event,
    .Key_ClearStates = Key_ClearStates,
    .Key_KeynumToString = Key_KeynumToString,
    .Key_SetBinding = Key_SetBinding,
    .Key_WriteBindings = Key_WriteBindings,
    .Key_SetTeamMessage = Key_SetTeamMessage,
    .Key_GetChatBuffer = Key_GetChatBuffer,
    .Key_Init = Key_Init,
    .Key_ClearAnyTyping = Key_ClearAnyTyping,

    .Sys_FloatTime = Sys_FloatTime,
};


Dll dll;


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    BOOL res = TRUE;

    /* previous instances do not exist in Win32 */
    if (hPrevInstance)
        return 0;

    g_hInstance = hInstance;

    const VideoMode mode = gl;
    InitCommandLine(aArgs[mode] /*lpCmdLine*/);

    const int isDedicated = Args_Dedicated();

    Mode_Init();
    Memory_Init();

    hModule = LoadLibrary(aLibFileName[mode]);

    if (hModule)
    {
        dll.Setup = GetProcAddress(hModule, isDedicated ? "_Setup_Dedicated@0" : "_Setup@0");
        dll.Loop = GetProcAddress(hModule, isDedicated ? "_Loop_Dedicated@8" : "_Loop@8");
        dll.Initialize = GetProcAddress(hModule, "_Initialize@4");

        dll.CL_IsDemoPlayBack = GetProcAddress(hModule, "_CL_IsDemoPlayBack@0");
        dll.CL_IsStateDisconnected = GetProcAddress(hModule, "_CL_IsStateDisconnected@0");
        dll.Cbuf_AddText = GetProcAddress(hModule, "_Cbuf_AddText@4");
        dll.Cmd_AddCommand = GetProcAddress(hModule, "_Cmd_AddCommand@8");
        dll.Cmd_Argc = GetProcAddress(hModule, "_Cmd_Argc@0");
        dll.Cmd_Argv = GetProcAddress(hModule, "_Cmd_Argv@4");
        dll.Cmd_CompleteCommand = GetProcAddress(hModule, "_Cmd_CompleteCommand@4");
        dll.Con_GetBackScroll = GetProcAddress(hModule, "_Con_GetBackScroll@0");
        dll.Con_GetTotalLines = GetProcAddress(hModule, "_Con_GetTotalLines@0");
        dll.Con_IsForcedUp = GetProcAddress(hModule, "_Con_IsForcedUp@0");
        dll.Con_SetBackScroll = GetProcAddress(hModule, "_Con_SetBackScroll@4");
        dll.Cvar_CompleteVariable = GetProcAddress(hModule, "_Cvar_CompleteVariable@4");
        dll.Lib_Con_Printf = GetProcAddress(hModule, "_Lib_Con_Printf@4");
        dll.Lib_M_ToggleMenu_f = GetProcAddress(hModule, "_Lib_M_ToggleMenu_f@0");
        dll.M_Keydown = GetProcAddress(hModule, "_M_Keydown@4");
        dll.SCR_UpdateScreen = GetProcAddress(hModule, "_SCR_UpdateScreen@0");
        dll.VID_GetHeight = GetProcAddress(hModule, "_VID_GetHeight@0");
    }

    if (dll.Initialize)
    {
        dll.Initialize(&g_appApi);

        if (dll.Setup && dll.Loop)
        {
            dll.Setup();
            double old_time = Sys_FloatTime();

            while (1)
            {
                old_time = dll.Loop(old_time);
            }
            res = 1;
        }
        else
            MessageBox(NULL, "Setup or Loop not loaded", "Setup or Loop not loaded!", 0);
    }
    else
        MessageBox(NULL, "Initialize not loaded", "Initialize not loaded!", 0);

    dll.Setup = NULL;
    dll.Loop = NULL;
    dll.Initialize = NULL;
    dll.Cbuf_AddText = NULL;
    dll.Lib_Con_Printf = NULL;
    dll.Cmd_AddCommand = NULL;
    dll.M_Keydown = NULL;
    dll.Lib_M_ToggleMenu_f = NULL;
    dll.Cmd_Argc = NULL;
    dll.Cmd_Argv = NULL;
    dll.SCR_UpdateScreen = NULL;
    dll.Cmd_CompleteCommand = NULL;
    dll.Cvar_CompleteVariable = NULL;
    dll.CL_IsStateDisconnected = NULL;
    dll.Con_GetTotalLines = NULL;
    dll.VID_GetHeight = NULL;
    dll.Con_GetBackScroll = NULL;
    dll.Con_SetBackScroll = NULL;
    dll.CL_IsDemoPlayBack = NULL;
    dll.Con_IsForcedUp = NULL;

    if (hModule)
    {
        FreeLibrary(hModule);
        hModule = NULL;
    }

    return res;
}

