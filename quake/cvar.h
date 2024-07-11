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
    void Cvar_Set(const char * var_name, const char * value);
    void Cvar_SetValue(const char * var_name, float value);
    float Cvar_VariableValue(const char * var_name);
    const char * Cvar_VariableString(const char * var_name);
    int Cvar_Command(const char * arg1, const char * arg2, int argc);
    void Cvar_WriteVariables(void * f);
    void * Cvar_FindVar(const char * var_name);
    const char * Cvar_GetFirstServer(const char * prevCvarName);

#ifdef __cplusplus
}
#endif
