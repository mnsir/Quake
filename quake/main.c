#include "app.h"
#include "args.h"
#include "mode.h"
#include "memory.h"
#include "keys.h"
#include "sys.h"
#include "cvar.h"
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

    .Cvar_RegisterVariable = Cvar_RegisterVariable,
    .Cvar_Set = Cvar_Set,
    .Cvar_SetValue = Cvar_SetValue,
    .Cvar_VariableValue = Cvar_VariableValue,
    .Cvar_VariableString = Cvar_VariableString,
    .Cvar_Command = Cvar_Command,
    .Cvar_WriteVariables = Cvar_WriteVariables,
    .Cvar_FindVar = Cvar_FindVar,
    .Cvar_GetFirstServer = Cvar_GetFirstServer,
};


Dll dll;


unsigned char scantokey[128] =
{
    // 0 1 2 3 4 5 6 7 
    // 8 9 A B C D E F 
    0, 27, '1', '2', '3', '4', '5', '6',
    '7', '8', '9', '0', '-', '=', K_BACKSPACE, 9, // 0 
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', 13, K_CTRL, 'a', 's', // 1 
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', K_SHIFT, '\\', 'z', 'x', 'c', 'v', // 2 
    'b', 'n', 'm', ',', '.', '/', K_SHIFT, '*',
    K_ALT, ' ', 0, K_F1, K_F2, K_F3, K_F4, K_F5, // 3 
    K_F6, K_F7, K_F8, K_F9, K_F10, K_PAUSE, 0, K_HOME,
    K_UPARROW, K_PGUP, '-', K_LEFTARROW, '5', K_RIGHTARROW, '+', K_END, //4 
    K_DOWNARROW, K_PGDN, K_INS, K_DEL, 0, 0, 0, K_F11,
    K_F12, 0, 0, 0, 0, 0, 0, 0, // 5 
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, // 6 
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0 // 7 
};

int MapKey(int key)
{
    key = (key >> 16) & 255;
    if (key > 127)
        return 0;
    if (scantokey[key] == 0)
    {
        //TODO Con_DPrintf("key 0x%02x has no translation\n", key);
    }
    return scantokey[key];
}

LONG WINAPI MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LONG retVal = 0;
    switch (uMsg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        Key_Event(MapKey(lParam), 1);
        break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        Key_Event(MapKey(lParam), 0);
        break;

        // JACK: This is the mouse wheel with the Intellimouse
        // Its delta is either positive or neg, and we generate the proper
        // Event.
    case WM_MOUSEWHEEL:
        if ((short)HIWORD(wParam) > 0)
        {
            Key_Event(K_MWHEELUP, 1);
            Key_Event(K_MWHEELUP, 0);
        }
        else
        {
            Key_Event(K_MWHEELDOWN, 1);
            Key_Event(K_MWHEELDOWN, 0);
        }
        break;

    default:
        retVal = dll.MainWndProcDll(hWnd, uMsg, wParam, lParam);
        break;
    }
    return retVal;
}

void WinInit()
{
    WNDCLASS wc;

    /* Register the frame class */
    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC)MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_hInstance;
    wc.hIcon = 0;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = 0;
    wc.lpszClassName = "WinQuake";

    if (!RegisterClass(&wc))
    {
        //Sys_Error("Couldn't register window class");
    }
}


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
        dll.Lib_Con_Printf = GetProcAddress(hModule, "_Lib_Con_Printf@4");
        dll.Lib_M_ToggleMenu_f = GetProcAddress(hModule, "_Lib_M_ToggleMenu_f@0");
        dll.M_Keydown = GetProcAddress(hModule, "_M_Keydown@4");
        dll.SCR_UpdateScreen = GetProcAddress(hModule, "_SCR_UpdateScreen@0");
        dll.VID_GetHeight = GetProcAddress(hModule, "_VID_GetHeight@0");

        dll.MainWndProcDll = GetProcAddress(hModule, "_MainWndProcDll@16");
    }

    if (dll.Initialize)
    {
        dll.Initialize(&g_appApi);

        if (dll.Setup && dll.Loop)
        {
            WinInit();
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
    dll.CL_IsStateDisconnected = NULL;
    dll.Con_GetTotalLines = NULL;
    dll.VID_GetHeight = NULL;
    dll.Con_GetBackScroll = NULL;
    dll.Con_SetBackScroll = NULL;
    dll.CL_IsDemoPlayBack = NULL;
    dll.Con_IsForcedUp = NULL;
    dll.MainWndProcDll = NULL;
    if (hModule)
    {
        FreeLibrary(hModule);
        hModule = NULL;
    }

    return res;
}

