
// sv_edict.c -- entity dictionary

#include "quakedef.h"
#include <common/pak.h>
#include <common/progs.h>

globalvars_t * pr_global_struct;
float * pr_globals; // same as pr_global_struct

cvar_t nomonsters = {(char*)"nomonsters", (char*)"0"};
cvar_t gamecfg = {(char*)"gamecfg", (char*)"0"};
cvar_t scratch1 = {(char*)"scratch1", (char*)"0"};
cvar_t scratch2 = {(char*)"scratch2", (char*)"0"};
cvar_t scratch3 = {(char*)"scratch3", (char*)"0"};
cvar_t scratch4 = {(char*)"scratch4", (char*)"0"};
cvar_t savedgamecfg = {(char*)"savedgamecfg", (char*)"0", true};
cvar_t saved1 = {(char*)"saved1", (char*)"0", true};
cvar_t saved2 = {(char*)"saved2", (char*)"0", true};
cvar_t saved3 = {(char*)"saved3", (char*)"0", true};
cvar_t saved4 = {(char*)"saved4", (char*)"0", true};

struct StrCmp
{
    const char* name = nullptr;
    [[nodiscard]] bool operator()(int i) const { return !strcmp(Progs::FromStringOffset(i), name); }
};

struct SvCmp
{
    std::string_view name;
    [[nodiscard]] bool operator()(int i) const { return Progs::FromStringOffset(i) == name; }
};

namespace
{
    size_t GetTypeSize(Progs::FieldDef::Type type)
    {
        switch (type)
        {
        case Progs::FieldDef::Type::ev_void:
            return 1;
        case Progs::FieldDef::Type::ev_string:
            return sizeof(string_t) / 4;
        case Progs::FieldDef::Type::ev_float:
            return 1;
        case Progs::FieldDef::Type::ev_vector:
            return 3;
        case Progs::FieldDef::Type::ev_entity:
            return 1;
        case Progs::FieldDef::Type::ev_function:
            return sizeof(func_t) / 4;
        default:
            return 0;
        }
    }

    // Sets everything to NULL
    void ED_ClearEdict(edict_t& e)
    {
        memset(&e.v, 0, Progs::entityfields * 4);
        e.free = false;
    }

    // Returns a string describing *data in a type specific manner
    char* PR_ValueString(Progs::ddef_t::etype_t type, const eval_t& val)
    {
        static char line[256];

        switch (type)
        {
        case Progs::ddef_t::etype_t::ev_string:
            sprintf(line, "%s", Progs::FromStringOffset(val.string));
            break;
        case Progs::ddef_t::etype_t::ev_entity:
            sprintf(line, "entity %i", NUM_FOR_EDICT(PROG_TO_EDICT(val.edict)));
            break;
        case Progs::ddef_t::etype_t::ev_function:
        {
            auto&& f = Progs::GetFunctions()[val.function];
            sprintf(line, "%s()", f.s_name.data());
            break;
        }
        case Progs::ddef_t::etype_t::ev_field:
        {
            const auto ofs = val._int;
            auto defs = Progs::GetFieldDefs();
            if (auto it = std::ranges::find(defs, ofs, &Progs::FieldDef::ofs); it != defs.end())
            {
                sprintf(line, ".%s", it->name.data());
            }
            break;
        }
        case Progs::ddef_t::etype_t::ev_void:
            sprintf(line, "void");
            break;
        case Progs::ddef_t::etype_t::ev_float:
            sprintf(line, "%5.1f", val._float);
            break;
        case Progs::ddef_t::etype_t::ev_vector:
            sprintf(line, "'%5.1f %5.1f %5.1f'", val.vector[0], val.vector[1], val.vector[2]);
            break;
        default:
            sprintf(line, "bad type %i", type);
            break;
        }

        return line;
    }

    // Returns a string describing *data in a type specific manner
    char* PR_ValueString(Progs::FieldDef::Type type, const eval_t& val)
    {
        static char line[256];

        switch (type)
        {
        case Progs::FieldDef::Type::ev_string:
            sprintf(line, "%s", Progs::FromStringOffset(val.string));
            break;
        case Progs::FieldDef::Type::ev_entity:
            sprintf(line, "entity %i", NUM_FOR_EDICT(PROG_TO_EDICT(val.edict)));
            break;
        case Progs::FieldDef::Type::ev_function:
        {
            auto&& f = Progs::GetFunctions()[val.function];
            sprintf(line, "%s()", f.s_name.data());
            break;
        }
        case Progs::FieldDef::Type::ev_void:
            sprintf(line, "void");
            break;
        case Progs::FieldDef::Type::ev_float:
            sprintf(line, "%5.1f", val._float);
            break;
        case Progs::FieldDef::Type::ev_vector:
            sprintf(line, "'%5.1f %5.1f %5.1f'", val.vector[0], val.vector[1], val.vector[2]);
            break;
        default:
            sprintf(line, "bad type %i", type);
            break;
        }

        return line;
    }

    // Returns a string describing *data in a type specific manner Easier to parse than PR_ValueString
    char* PR_UglyValueString(Progs::ddef_t::etype_t type, const eval_t& val)
    {
        static char line[256];

        switch (type)
        {
        case Progs::ddef_t::etype_t::ev_string:
            sprintf(line, "%s", Progs::FromStringOffset(val.string));
            break;
        case Progs::ddef_t::etype_t::ev_entity:
            sprintf(line, "%i", NUM_FOR_EDICT(PROG_TO_EDICT(val.edict)));
            break;
        case Progs::ddef_t::etype_t::ev_function:
        {
            auto&& f = Progs::GetFunctions()[val.function];
            sprintf(line, "%s", f.s_name.data());
            break;
        }
        case Progs::ddef_t::etype_t::ev_field:
        {
            const auto ofs = val._int;
            auto defs = Progs::GetFieldDefs();
            if (auto it = std::ranges::find(defs, ofs, &Progs::FieldDef::ofs); it != defs.end())
            {
                sprintf(line, "%s", it->name.data());
            }
            break;
        }
        case Progs::ddef_t::etype_t::ev_void:
            sprintf(line, "void");
            break;
        case Progs::ddef_t::etype_t::ev_float:
            sprintf(line, "%f", val._float);
            break;
        case Progs::ddef_t::etype_t::ev_vector:
            sprintf(line, "%f %f %f", val.vector[0], val.vector[1], val.vector[2]);
            break;
        default:
            sprintf(line, "bad type %i", type);
            break;
        }

        return line;
    }

    // Returns a string describing *data in a type specific manner Easier to parse than PR_ValueString
    char* PR_UglyValueString(Progs::FieldDef::Type type, const eval_t& val)
    {
        static char line[256];

        switch (type)
        {
        case Progs::FieldDef::Type::ev_string:
            sprintf(line, "%s", Progs::FromStringOffset(val.string));
            break;
        case Progs::FieldDef::Type::ev_entity:
            sprintf(line, "%i", NUM_FOR_EDICT(PROG_TO_EDICT(val.edict)));
            break;
        case Progs::FieldDef::Type::ev_function:
        {
            auto&& f = Progs::GetFunctions()[val.function];
            sprintf(line, "%s", f.s_name.data());
            break;
        }
        case Progs::FieldDef::Type::ev_void:
            sprintf(line, "void");
            break;
        case Progs::FieldDef::Type::ev_float:
            sprintf(line, "%f", val._float);
            break;
        case Progs::FieldDef::Type::ev_vector:
            sprintf(line, "%f %f %f", val.vector[0], val.vector[1], val.vector[2]);
            break;
        default:
            sprintf(line, "bad type %i", type);
            break;
        }

        return line;
    }

    char* ED_NewString(char* string)
    {
        char* new_, * new_p;
        int i, l;

        l = strlen(string) + 1;
        new_ = (char*)Hunk_Alloc(l);
        new_p = new_;

        for (i = 0; i < l; i++)
        {
            if (string[i] == '\\' && i < l - 1)
            {
                i++;
                if (string[i] == 'n')
                    *new_p++ = '\n';
                else
                    *new_p++ = '\\';
            }
            else
                *new_p++ = string[i];
        }

        return new_;
    }

    // Can parse either fields or globals returns false if error
    bool ED_ParseEpair(entvars_t* base, const Progs::FieldDef& key, char* s)
    {
        void* d = (void*)((int*)base + key.ofs);

        switch (key.type)
        {
        case Progs::FieldDef::Type::ev_string:
            *(string_t*)d = Progs::ToStringOffset(ED_NewString(s));
            break;

        case Progs::FieldDef::Type::ev_float:
            *(float*)d = atof(s);
            break;

        case Progs::FieldDef::Type::ev_vector:
        {
            char string[128];
            strcpy(string, s);
            char* v = string;
            char* w = string;
            for (int i = 0; i < 3; i++)
            {
                while (*v && *v != ' ')
                    v++;
                *v = 0;
                ((float*)d)[i] = atof(w);
                w = v = v + 1;
            }
            break;
        }
        case Progs::FieldDef::Type::ev_entity:
            *(int*)d = EDICT_TO_PROG(EDICT_NUM(atoi(s)));
            break;

        case Progs::FieldDef::Type::ev_function:
        {
            auto funcs = Progs::GetFunctions();
            if (auto it = std::ranges::find(funcs, s, &Progs::dfunction_t::s_name); it != funcs.end())
            {
                *(func_t*)d = std::ranges::distance(funcs.begin(), it);
            }
            else
            {
                Con_Printf("Can't find function %s\n", s);
                return false;
            }
            break;
        }
        default:
            break;
        }
        return true;
    }

    // Can parse either fields or globals returns false if error
    bool ED_ParseEpair(float* base, const Progs::ddef_t& key, char* s)
    {
        void* d = (void*)((int*)base + key.ofs);

        switch (key.type)
        {
        case Progs::ddef_t::etype_t::ev_string:
            *(string_t*)d = Progs::ToStringOffset(ED_NewString(s));
            break;

        case Progs::ddef_t::etype_t::ev_float:
            *(float*)d = atof(s);
            break;

        case Progs::ddef_t::etype_t::ev_vector:
        {
            char string[128];
            strcpy(string, s);
            char* v = string;
            char* w = string;
            for (int i = 0; i < 3; i++)
            {
                while (*v && *v != ' ')
                    v++;
                *v = 0;
                ((float*)d)[i] = atof(w);
                w = v = v + 1;
            }
            break;
        }
        case Progs::ddef_t::etype_t::ev_entity:
            *(int*)d = EDICT_TO_PROG(EDICT_NUM(atoi(s)));
            break;

        case Progs::ddef_t::etype_t::ev_field:
        {
            auto defs = Progs::GetFieldDefs();
            if (auto it = std::ranges::find(defs, s, &Progs::FieldDef::name); it != defs.end())
            {
                *(int*)d = *(int*)(pr_globals + it->ofs);
            }
            else
            {
                Con_Printf("Can't find field %s\n", s);
                return false;
            }
            break;
        }
        case Progs::ddef_t::etype_t::ev_function:
        {
            auto funcs = Progs::GetFunctions();
            if (auto it = std::ranges::find(funcs, s, &Progs::dfunction_t::s_name); it != funcs.end())
            {
                *(func_t*)d = std::ranges::distance(funcs.begin(), it);
            }
            else
            {
                Con_Printf("Can't find function %s\n", s);
                return false;
            }
            break;
        }
        default:
            break;
        }
        return true;
    }
}

/*
=================
ED_Alloc

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t * ED_Alloc()
{
    int i;
    edict_t * e;

    for (i = svs.maxclients + 1; i < sv.num_edicts; i++)
    {
        e = EDICT_NUM(i);
        // the first couple seconds of server time can involve a lot of
        // freeing and allocating, so relax the replacement policy
        if (e->free && (e->freetime < 2 || sv.time - e->freetime > 0.5))
        {
            ED_ClearEdict(*e);
            return e;
        }
    }

    if (i == MAX_EDICTS)
        Sys_Error("ED_Alloc: no free edicts");

    sv.num_edicts++;
    e = EDICT_NUM(i);
    ED_ClearEdict(*e);

    return e;
}

/*
=================
ED_Free

Marks the edict as free
FIXME: walk all entities and NULL out references to this entity
=================
*/
void ED_Free(edict_t * ed)
{
    SV_UnlinkEdict(ed); // unlink from world bsp

    ed->free = true;
    ed->v.model = 0;
    ed->v.takedamage = 0;
    ed->v.modelindex = 0;
    ed->v.colormap = 0;
    ed->v.skin = 0;
    ed->v.frame = 0;
    VectorCopy(vec3_origin, ed->v.origin);
    VectorCopy(vec3_origin, ed->v.angles);
    ed->v.nextthink = -1;
    ed->v.solid = 0;

    ed->freetime = sv.time;
}

//===========================================================================

eval_t* GetEdictFieldValue(edict_t* ed, std::string_view field)
{
    static std::array<std::pair<std::string_view, const Progs::FieldDef*>, 2> cache = {};
    static int rep = 0;

    const Progs::FieldDef* def = nullptr;
    if (auto it = std::ranges::find(cache, field, &decltype(cache)::value_type::first); it != cache.end())
    {
        def = it->second;
    }
    else
    {
        auto defs = Progs::GetFieldDefs();
        if (auto it = std::ranges::find(defs, field, &Progs::FieldDef::name); it != defs.end())
        {
            def = &*it;
        }
        cache[rep] = { field,def };
        rep ^= 1;
    }

    return def ? (eval_t*)((char*)&ed->v + def->ofs * 4) : nullptr;
}

/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
char* PR_GlobalString(int ofs)
{
    static char line[128];

    auto gd = Progs::GetGlobalDefs();
    if (auto it = std::ranges::find(gd, ofs, &Progs::ddef_t::ofs); it != gd.end())
    {
        void* val = (void*)&pr_globals[ofs];
        char* s = PR_ValueString(it->type, *(eval_t*)val);
        sprintf(line, "%i(%s)%s", ofs, it->s_name.data(), s);
    }
    else
    {
        sprintf(line, "%i(???)", ofs);
    }

    int i = strlen(line);
    for (; i < 20; i++)
        strcat(line, " ");
    strcat(line, " ");

    return line;
}

char * PR_GlobalStringNoContents(int ofs)
{
    static char line[128];

    auto gd = Progs::GetGlobalDefs();
    if (auto it = std::ranges::find(gd, ofs, &Progs::ddef_t::ofs); it != gd.end())
    {
        sprintf(line, "%i(%s)", ofs, it->s_name.data());
    }
    else
    {
        sprintf(line, "%i(???)", ofs);
    }

    int i = strlen(line);
    for (; i < 20; i++)
        strcat(line, " ");
    strcat(line, " ");

    return line;
}


/*
=============
ED_Print

For debugging
=============
*/
void ED_Print(edict_t * ed)
{
    if (ed->free)
    {
        Con_Printf("FREE\n");
        return;
    }

    Con_Printf("\nEDICT %i:\n", NUM_FOR_EDICT(ed));
    for (auto&& def : Progs::GetFieldDefs() | std::views::drop(1))
    {
        auto&& name = def.name;
        if (name[name.size() - 2] == '_')
            continue; // skip _x, _y, _z vars

        int* v = (int *)((char *)&ed->v + def.ofs * 4);

        // if the value is still all 0, skip the field
        int j = 0;
        auto sz = GetTypeSize(def.type);
        for (; j < sz; j++)
            if (v[j])
                break;
        if (j == sz)
            continue;

        Con_Printf("%s", name);
        int l = name.size();
        while (l++ < 15)
            Con_Printf(" ");

        Con_Printf("%s\n", PR_ValueString(def.type, *(eval_t *)v));
    }
}

/*
=============
ED_Write

For savegames
=============
*/
void ED_Write(FILE * f, edict_t * ed)
{
    fprintf(f, "{\n");

    if (ed->free)
    {
        fprintf(f, "}\n");
        return;
    }

    for (auto&& def : Progs::GetFieldDefs() | std::views::drop(1))
    {
        auto&& name = def.name;
        if (name[name.size() - 2] == '_')
            continue; // skip _x, _y, _z vars

        int* v = (int *)((char *)&ed->v + def.ofs * 4);

        // if the value is still all 0, skip the field
        int j = 0;
        auto sz = GetTypeSize(def.type);
        for (; j < sz; j++)
            if (v[j])
                break;
        if (j == sz)
            continue;

        fprintf(f, "\"%s\" ", name.data());
        fprintf(f, "\"%s\"\n", PR_UglyValueString(def.type, *(eval_t *)v));
    }

    fprintf(f, "}\n");
}

void ED_PrintNum(int ent)
{
    ED_Print(EDICT_NUM(ent));
}

/*
=============
ED_PrintEdicts

For debugging, prints all the entities in the current server
=============
*/
void ED_PrintEdicts()
{
    int i;

    Con_Printf("%i entities\n", sv.num_edicts);
    for (i = 0; i < sv.num_edicts; i++)
        ED_PrintNum(i);
}

/*
=============
ED_PrintEdict_f

For debugging, prints a single edicy
=============
*/
void ED_PrintEdict_f()
{
    int i;

    i = std::atoi(Cmd_Argv(1));
    if (i >= sv.num_edicts)
    {
        Con_Printf("Bad edict number\n");
        return;
    }
    ED_PrintNum(i);
}

/*
=============
ED_Count

For debugging
=============
*/
void ED_Count()
{
    int i;
    edict_t * ent;
    int active, models, solid, step;

    active = models = solid = step = 0;
    for (i = 0; i < sv.num_edicts; i++)
    {
        ent = EDICT_NUM(i);
        if (ent->free)
            continue;
        active++;
        if (ent->v.solid)
            solid++;
        if (ent->v.model)
            models++;
        if (ent->v.movetype == MOVETYPE_STEP)
            step++;
    }

    Con_Printf("num_edicts:%3i\n", sv.num_edicts);
    Con_Printf("active :%3i\n", active);
    Con_Printf("view :%3i\n", models);
    Con_Printf("touch :%3i\n", solid);
    Con_Printf("step :%3i\n", step);

}

/*
==============================================================================

 ARCHIVING GLOBALS

FIXME: need to tag constants, doesn't really work
==============================================================================
*/

/*
=============
ED_WriteGlobals
=============
*/
void ED_WriteGlobals(FILE * f)
{
    fprintf(f, "{\n");
    for (auto&& def : Progs::GetGlobalDefs())
    {
        if (def.save_global)
        {
            if (def.type == Progs::ddef_t::etype_t::ev_string ||
                def.type == Progs::ddef_t::etype_t::ev_float ||
                def.type == Progs::ddef_t::etype_t::ev_entity)
            {
                fprintf(f, "\"%s\" ", def.s_name.data());
                fprintf(f, "\"%s\"\n", PR_UglyValueString(def.type, *(eval_t*)(pr_globals + def.ofs)));
            }
        }
    }
    fprintf(f, "}\n");
}

/*
=============
ED_ParseGlobals
=============
*/
void ED_ParseGlobals(char * data)
{
    char keyname[64];

    while (1)
    {
        // parse key
        data = COM_Parse(data);
        if (com_token[0] == '}')
            break;
        if (!data)
            Sys_Error("ED_ParseEntity: EOF without closing brace");

        strcpy(keyname, com_token);

        // parse value 
        data = COM_Parse(data);
        if (!data)
            Sys_Error("ED_ParseEntity: EOF without closing brace");

        if (com_token[0] == '}')
            Sys_Error("ED_ParseEntity: closing brace without data");

        auto defs = Progs::GetGlobalDefs();
        if (auto it = std::ranges::find(defs, keyname, &Progs::ddef_t::s_name); it != defs.end())
        {
            if (!ED_ParseEpair(pr_globals, *it, com_token))
                Host_Error("ED_ParseGlobals: parse error");
        }
        else
        {
            Con_Printf("'%s' is not a global\n", keyname);
        }
    }
}

//============================================================================

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
Used for initial level load and for savegames.
====================
*/
char * ED_ParseEdict(char * data, edict_t * ent)
{
    bool anglehack;
    bool init;
    char keyname[256];
    int n;

    init = false;

    // clear it
    if (ent != sv.edicts) // hack
        memset(&ent->v, 0, Progs::entityfields * 4);

    // go through all the dictionary pairs
    while (1)
    {
        // parse key
        data = COM_Parse(data);
        if (com_token[0] == '}')
            break;
        if (!data)
            Sys_Error("ED_ParseEntity: EOF without closing brace");

        // anglehack is to allow QuakeEd to write single scalar angles
        // and allow them to be turned into vectors. (FIXME...)
        if (!strcmp(com_token, "angle"))
        {
            strcpy(com_token, "angles");
            anglehack = true;
        }
        else
            anglehack = false;

        // FIXME: change light to _light to get rid of this hack
        if (!strcmp(com_token, "light"))
            strcpy(com_token, "light_lev"); // hack for single light def

        strcpy(keyname, com_token);

        // another hack to fix heynames with trailing spaces
        n = strlen(keyname);
        while (n && keyname[n - 1] == ' ')
        {
            keyname[n - 1] = 0;
            n--;
        }

        // parse value 
        data = COM_Parse(data);
        if (!data)
            Sys_Error("ED_ParseEntity: EOF without closing brace");

        if (com_token[0] == '}')
            Sys_Error("ED_ParseEntity: closing brace without data");

        init = true;

        // keynames with a leading underscore are used for utility comments,
        // and are immediately discarded by quake
        if (keyname[0] == '_')
            continue;

        auto defs = Progs::GetFieldDefs();
        if (auto it = std::ranges::find(defs, keyname, &Progs::FieldDef::name); it != defs.end())
        {
            if (anglehack)
            {
                char temp[32];
                strcpy(temp, com_token);
                sprintf(com_token, "0 %s 0", temp);
            }

            if (!ED_ParseEpair(&ent->v, *it, com_token))
                Host_Error("ED_ParseEdict: parse error");
        }
        else
        {
            Con_Printf("'%s' is not a field\n", keyname);
        }
    }

    if (!init)
        ent->free = true;

    return data;
}


/*
================
ED_LoadFromFile

The entities are directly placed in the array, rather than allocated with
ED_Alloc, because otherwise an error loading the map would have entity
number references out of order.

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.

Used for both fresh maps and savegame loads. A fresh map would also need
to call ED_CallSpawnFunctions () to let the objects initialize themselves.
================
*/
void ED_LoadFromFile(char * data)
{
    edict_t * ent;
    int inhibit;

    ent = NULL;
    inhibit = 0;
    Progs::GetGlobalStruct().time = sv.time;

    // parse ents
    while (1)
    {
        // parse the opening brace 
        data = COM_Parse(data);
        if (!data)
            break;
        if (com_token[0] != '{')
            Sys_Error("ED_LoadFromFile: found %s when expecting {", com_token);

        if (!ent)
            ent = EDICT_NUM(0);
        else
            ent = ED_Alloc();
        data = ED_ParseEdict(data, ent);

        // remove things from different skill levels or deathmatch
        if (deathmatch.value)
        {
            if (((int)ent->v.spawnflags & SPAWNFLAG_NOT_DEATHMATCH))
            {
                ED_Free(ent);
                inhibit++;
                continue;
            }
        }
        else if ((current_skill == 0 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_EASY))
                 || (current_skill == 1 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_MEDIUM))
                 || (current_skill >= 2 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_HARD)))
        {
            ED_Free(ent);
            inhibit++;
            continue;
        }

        //
        // immediately call spawn function
        //
        if (!ent->v.classname)
        {
            Con_Printf("No classname for:\n");
            ED_Print(ent);
            ED_Free(ent);
            continue;
        }
        auto funcs = Progs::GetFunctions();
        if (auto it = std::ranges::find(funcs, Progs::FromStringOffset(ent->v.classname), &Progs::dfunction_t::s_name); it != funcs.end())
        {
            Progs::GetGlobalStruct().self = EDICT_TO_PROG(ent);
            PR_ExecuteProgram(std::ranges::distance(funcs.begin(), it));
        }
        else
        {
            Con_Printf("No spawn function for:\n");
            ED_Print(ent);
            ED_Free(ent);
        }
    }

    Con_DPrintf("%i entities inhibited\n", inhibit);
}


/*
===============
PR_LoadProgs
===============
*/
void PR_LoadProgs()
{
    auto globals = Progs::GetGlobals();
    pr_global_struct = (globalvars_t*)globals.data();

    pr_globals = (float*)pr_global_struct;
}


/*
===============
PR_Init
===============
*/
void PR_Init()
{
    Cmd_AddCommand((char*)"edict", ED_PrintEdict_f);
    Cmd_AddCommand((char*)"edicts", ED_PrintEdicts);
    Cmd_AddCommand((char*)"edictcount", ED_Count);
    Cmd_AddCommand((char*)"profile", PR_Profile_f);
    Cvar_RegisterVariable(&nomonsters);
    Cvar_RegisterVariable(&gamecfg);
    Cvar_RegisterVariable(&scratch1);
    Cvar_RegisterVariable(&scratch2);
    Cvar_RegisterVariable(&scratch3);
    Cvar_RegisterVariable(&scratch4);
    Cvar_RegisterVariable(&savedgamecfg);
    Cvar_RegisterVariable(&saved1);
    Cvar_RegisterVariable(&saved2);
    Cvar_RegisterVariable(&saved3);
    Cvar_RegisterVariable(&saved4);
}



edict_t * EDICT_NUM(int n)
{
    if (n < 0 || n >= sv.max_edicts)
        Sys_Error("EDICT_NUM: bad number %i", n);
    return (edict_t *)((byte *)sv.edicts + (n)*Progs::edict_size);
}

int NUM_FOR_EDICT(edict_t * e)
{
    int b;

    b = (byte *)e - (byte *)sv.edicts;
    b = b / Progs::edict_size;

    if (b < 0 || b >= sv.num_edicts)
        Sys_Error("NUM_FOR_EDICT: bad pointer");
    return b;
}

edict_t* NEXT_EDICT(edict_t* e)
{
    return (edict_t*)((byte*)e + Progs::edict_size);
}

string_t Progs::ToStringOffset(char* str)
{
    return str - Progs::GetStrings().data();
}

char* Progs::FromStringOffset(string_t offset)
{
    return Progs::GetStrings().data() + offset;
}

globalvars_t& Progs::GetGlobalStruct()
{
    return *pr_global_struct;
}
