#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <appapi.h>


#ifdef __cplusplus
extern "C" {
#endif

	typedef void (CALLBACK* SetupFunc)();
	typedef double (CALLBACK* LoopFunc)(double old_time);
	typedef void (CALLBACK* InitializeFunc)(AppAPI* pAppApi);

	typedef void (CALLBACK* Cbuf_AddTextFunc)(const char* text);
	typedef int (CALLBACK* CL_IsDemoPlayBackFunc)();
	typedef int (CALLBACK* CL_IsStateDisconnectedFunc)();
	typedef void (CALLBACK* Cmd_AddCommandFunc)(const char* cmd_name, void(*function)());
	typedef int (CALLBACK* Cmd_ArgcFunc)();
	typedef const char* (CALLBACK* Cmd_ArgvFunc)(int arg);
	typedef const char* (CALLBACK* Cmd_CompleteCommandFunc)(const char* partial);
	typedef int (CALLBACK* Con_GetBackScrollFunc)();
	typedef int (CALLBACK* Con_GetTotalLinesFunc)();
	typedef int (CALLBACK* Con_IsForcedUpFunc)();
	typedef void (CALLBACK* Con_SetBackScrollFunc)(int backscroll);
	typedef const char* (CALLBACK* Cvar_CompleteVariableFunc)(const char* partial);
	typedef void (CALLBACK* Lib_Con_PrintfFunc)(const char* str);
	typedef void (CALLBACK* Lib_M_ToggleMenu_fFunc)();
	typedef void (CALLBACK* M_KeydownFunc)(int key);
	typedef void (CALLBACK* SCR_UpdateScreenFunc)();
	typedef int (CALLBACK* VID_GetHeightFunc)();
	typedef long (CALLBACK* MainWndProcDllFunc)(void* hWnd, unsigned uMsg, unsigned wParam, long lParam);

	typedef struct
	{
		SetupFunc Setup;
		LoopFunc Loop;
		InitializeFunc Initialize;

		Cbuf_AddTextFunc Cbuf_AddText;
		CL_IsDemoPlayBackFunc CL_IsDemoPlayBack;
		CL_IsStateDisconnectedFunc CL_IsStateDisconnected;
		Cmd_AddCommandFunc Cmd_AddCommand;
		Cmd_ArgcFunc Cmd_Argc;
		Cmd_ArgvFunc Cmd_Argv;
		Cmd_CompleteCommandFunc Cmd_CompleteCommand;
		Con_GetBackScrollFunc Con_GetBackScroll;
		Con_GetTotalLinesFunc Con_GetTotalLines;
		Con_IsForcedUpFunc Con_IsForcedUp;
		Con_SetBackScrollFunc Con_SetBackScroll;
		Cvar_CompleteVariableFunc Cvar_CompleteVariable;
		Lib_Con_PrintfFunc Lib_Con_Printf;
		Lib_M_ToggleMenu_fFunc Lib_M_ToggleMenu_f;
		M_KeydownFunc M_Keydown;
		SCR_UpdateScreenFunc SCR_UpdateScreen;
		VID_GetHeightFunc VID_GetHeight;
		MainWndProcDllFunc MainWndProcDll;
	} Dll;

	extern Dll dll;

#ifdef __cplusplus
}
#endif
