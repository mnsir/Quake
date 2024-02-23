#include "lib_funcs.h"


FromLibFunc FromLib = NULL;
InitializeFunc Initialize = NULL;

Cbuf_AddTextFunc Cbuf_AddText = NULL;
Con_PrintfFunc Con_Printf = NULL;
Cmd_AddCommandFunc Cmd_AddCommand = NULL;
M_KeydownFunc M_Keydown = NULL;
M_ToggleMenu_fFunc M_ToggleMenu_f = NULL;
Cmd_ArgcFunc Cmd_Argc = NULL;
Cmd_ArgvFunc Cmd_Argv = NULL;
SCR_UpdateScreenFunc SCR_UpdateScreen = NULL;
Cmd_CompleteCommandFunc Cmd_CompleteCommand = NULL;
Cvar_CompleteVariableFunc Cvar_CompleteVariable = NULL;
CL_IsStateDisconnectedFunc CL_IsStateDisconnected = NULL;
