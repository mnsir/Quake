#include "quakedef.h"
#include <appapi.h>


cvar_t * Cvar_FindVar(char * var_name)
{
    return g_pAppApi->Cvar_FindVar(var_name);
}

const char * Cvar_GetFirstServer(const char * prevCvarName)
{
    return g_pAppApi->Cvar_GetFirstServer(prevCvarName);
}

float Cvar_VariableValue(char * var_name)
{
    g_pAppApi->Cvar_VariableValue(var_name);
}

void Cvar_Set(char * var_name, char * value)
{
    g_pAppApi->Cvar_Set(var_name, value);
}

void Cvar_SetValue(char * var_name, float value)
{
    g_pAppApi->Cvar_SetValue(var_name, value);
}

void Cvar_RegisterVariable(cvar_t * variable)
{
    g_pAppApi->Cvar_RegisterVariable(variable);
}

bool Cvar_Command()
{
    return g_pAppApi->Cvar_Command();
}

void Cvar_WriteVariables(FILE * f)
{
    g_pAppApi->Cvar_WriteVariables(f);
}
