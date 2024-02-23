#pragma once

#include "dll_func_typedefs.h"


extern FromLibFunc FromLib;
extern InitializeFunc Initialize;

extern Cbuf_AddTextFunc Cbuf_AddText;
extern Con_PrintfFunc Con_Printf;
extern Cmd_AddCommandFunc Cmd_AddCommand;
extern M_KeydownFunc M_Keydown;
extern M_ToggleMenu_fFunc M_ToggleMenu_f;
extern Cmd_ArgcFunc Cmd_Argc;
extern Cmd_ArgvFunc Cmd_Argv;
extern SCR_UpdateScreenFunc SCR_UpdateScreen;
extern Cmd_CompleteCommandFunc Cmd_CompleteCommand;
extern Cvar_CompleteVariableFunc Cvar_CompleteVariable;
extern CL_IsStateDisconnectedFunc CL_IsStateDisconnected;