#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <appapi.h>


typedef int (CALLBACK* FromLibFunc)();
typedef void (CALLBACK* InitializeFunc)(AppAPI* pAppApi);

typedef void (CALLBACK* Cbuf_AddTextFunc)(const char* text);
typedef void (CALLBACK* Con_PrintfFunc)(const char* fmt, ...);
typedef void (CALLBACK* Cmd_AddCommandFunc)(const char* cmd_name, void(*function)());
typedef void (CALLBACK* M_KeydownFunc)(int key);
typedef void (CALLBACK* M_ToggleMenu_fFunc)();
typedef int (CALLBACK* Cmd_ArgcFunc)();
typedef const char* (CALLBACK* Cmd_ArgvFunc)(int arg);
typedef void (CALLBACK* SCR_UpdateScreenFunc)();
typedef const char* (CALLBACK* Cmd_CompleteCommandFunc)(const char* partial);
typedef const char* (CALLBACK* Cvar_CompleteVariableFunc)(const char* partial);
typedef int (CALLBACK* CL_IsStateDisconnectedFunc)();
typedef int (CALLBACK* Con_GetTotalLinesFunc)();
typedef int (CALLBACK* VID_GetHeightFunc)();
typedef int (CALLBACK * Con_GetBackScrollFunc)();
typedef void (CALLBACK * Con_SetBackScrollFunc)(int backscroll);
typedef int (CALLBACK * CL_IsDemoPlayBackFunc)();
typedef int (CALLBACK * Con_IsForcedUpFunc)();