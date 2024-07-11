#include "cvar.h"

#include <cstring>
#include <string>
#include <map>
#include <charconv>
#include <optional>

typedef struct cvar_s
{
    char * name;
    char * string;
    bool archive; // set to true to cause it to be saved to vars.rc
    bool server; // notifies players when changed
    float value;
    struct cvar_s * next;
} cvar_t;


cvar_t * cvar_vars;
const char cvar_null_string[] = "";

namespace
{
std::map<std::string, std::string> data;

cvar_t * FindVar(const char * var_name)
{
    for (cvar_t * var = cvar_vars; var; var = var->next)
        if (!std::strcmp(var_name, var->name))
            return var;

    return nullptr;
}

std::optional<double> FromString(std::string_view str)
{
    double out = 0.0;
    auto&& [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), out);
    if (ec == std::errc{} && ptr == str.data() + str.size())
        return out;
    return{};
}

}

void * Cvar_FindVar(const char * var_name)
{
    return FindVar(var_name);
}

const char * Cvar_GetFirstServer(const char * prevCvarName)
{
    static const char empty[] = "";
    cvar_t * var = NULL;
    if (*prevCvarName)
    {
        var = FindVar(prevCvarName);
        if (!var)
            return NULL;
        var = var->next;
    }
    else
        var = cvar_vars;

    // search for the next server cvar
    while (var)
    {
        if (var->server)
            break;
        var = var->next;
    }
    return var ? var->name : empty;
}

/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValue(const char * var_name)
{
    cvar_t * var = FindVar(var_name);
    if (!var)
        return 0;
    return std::atof(var->string);
}


/*
============
Cvar_VariableString
============
*/
const char * Cvar_VariableString(const char * var_name)
{
    cvar_t * var = FindVar(var_name);
    if (!var)
        return cvar_null_string;
    return var->string;
}


/*
============
Cvar_CompleteVariable
============
*/
std::string Cvar::CompleteVariable(std::string_view partial)
{
    if (partial.empty())
        return NULL;

    // check functions
    for (cvar_t * cvar = cvar_vars; cvar; cvar = cvar->next)
        if (!std::strncmp(partial.data(), cvar->name, partial.size()))
            return cvar->name;

    return NULL;
}


/*
============
Cvar_Set
============
*/
void Cvar_Set(const char * var_name, const char * value)
{
    cvar_t * var = FindVar(var_name);
    if (!var)
    { // there is an error in C code if this happens
        //Con_Printf("Cvar_Set: variable %s not found\n", var_name);
        return;
    }

    bool changed = std::strcmp(var->string, value);

    auto && ref = data[var->name];
    ref = value;
    var->string = ref.data();
    
    auto val = FromString(var->string);
    if (val)
        var->value = *val;
    else
    {
        int i = 0;
        ++i;
    }

    if (var->server && changed)
    {
        //if (sv.active)
        //    SV_BroadcastPrintf("\"%s\" changed to \"%s\"\n", var->name, var->string);
    }
}

/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue(const char * var_name, float value)
{
    char val[32];

    sprintf(val, "%f", value);
    Cvar_Set(var_name, val);
}


/*
============
Cvar_RegisterVariable

Adds a freestanding variable to the variable list.
============
*/
void Cvar_RegisterVariable(void * var)
{
    auto * variable = (cvar_t*)var;

    // first check to see if it has allready been defined
    if (FindVar(variable->name))
    {
        //Con_Printf("Can't register variable %s, allready defined\n", variable->name);
        return;
    }

    // check for overlap with a command
    //if (Cmd_Exists(variable->name))
    //{
    //    Con_Printf("Cvar_RegisterVariable: %s is a command\n", variable->name);
    //    return;
    //}

    // copy the value off, because future sets will Z_Free it
    auto && ref = data[variable->name];
    ref = variable->string;
    variable->string = ref.data();

    auto val = FromString(variable->string);
    if (val)
        variable->value = *val;
    else
    {
        int i = 0;
        ++i;
    }

    // link the variable in
    variable->next = cvar_vars;
    cvar_vars = variable;
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
int Cvar_Command(const char * arg1, const char * arg2, int argc)
{
    // check variables
    cvar_t * v = FindVar(arg1);
    if (!v)
        return false;

    // perform a variable print or set
    if (argc == 1)
    {
        //Con_Printf("\"%s\" is \"%s\"\n", v->name, v->string);
        return true;
    }

    Cvar_Set(v->name, arg2);
    return true;
}


/*
============
Cvar_WriteVariables

Writes lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables(void * f)
{
    for (cvar_t * var = cvar_vars; var; var = var->next)
        if (var->archive)
            fprintf((FILE*)f, "%s \"%s\"\n", var->name, var->string);
}

