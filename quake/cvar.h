#pragma once

#ifdef __cplusplus

#include <string>
#include <string_view>

#endif

#ifdef __cplusplus

namespace Cvar
{

    std::string CompleteVariable(std::string_view partial);
}

#endif


#ifdef __cplusplus
extern "C" {
#endif

    void Cvar_RegisterVariable(void * variable);
    void Cvar_Set(char * var_name, char * value);
    void Cvar_SetValue(char * var_name, float value);
    float Cvar_VariableValue(char * var_name);
    char * Cvar_VariableString(char * var_name);
    int Cvar_Command();
    void Cvar_WriteVariables(void * f);
    void * Cvar_FindVar(char * var_name);
    const char * Cvar_GetFirstServer(const char * prevCvarName);

#ifdef __cplusplus
}
#endif
