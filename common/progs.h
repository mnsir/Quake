#pragma once
#include <array>
#include <span>

namespace Progs
{

    constexpr int entityfields = 195;
    

struct dfunction_t
{
    int first_statement = 0; // negative numbers are builtins
    int parm_start = 0;
    int locals = 0; // total ints of parms + locals

    int profile = 0; // runtime

    std::string_view s_name;
    std::string_view s_file; // source file defined in

    int numparms = 0;
    std::array<unsigned char, 8> parm_size = {};
};

struct dstatement_t
{
    unsigned short op;
    short a;
    short b;
    short c;
};

struct ddef_t
{
    enum class etype_t : uint8_t
    {
        ev_void,
        ev_string,
        ev_float,
        ev_vector,
        ev_entity,
        ev_field,
        ev_function,
        ev_pointer,
    };

    bool save_global;
    etype_t type;
    unsigned short ofs;
    std::string_view s_name;
};

struct globalvars_t
{
    int pad[28] = { 0 };
    int self = 0;
    int other = 0;
    int world = 0;
    float time = 0.0f;
    float frametime = 0.0f;
    float force_retouch = 0.0f;
    int/*string_t*/ mapname = 0;
    float deathmatch = 0.0f;
    float coop = 0.0f;
    float teamplay = 0.0f;
    float serverflags = 0.0f;
    float total_secrets = 0.0f;
    float total_monsters = 0.0f;
    float found_secrets = 0.0f;
    float killed_monsters = 0.0f;
    float parm1 = 0.0f;
    float parm2 = 0.0f;
    float parm3 = 0.0f;
    float parm4 = 0.0f;
    float parm5 = 0.0f;
    float parm6 = 0.0f;
    float parm7 = 0.0f;
    float parm8 = 0.0f;
    float parm9 = 0.0f;
    float parm10 = 0.0f;
    float parm11 = 0.0f;
    float parm12 = 0.0f;
    float parm13 = 0.0f;
    float parm14 = 0.0f;
    float parm15 = 0.0f;
    float parm16 = 0.0f;
    float/*vec3_t*/ v_forward[3] = {0};
    float/*vec3_t*/ v_up[3] = {0};
    float/*vec3_t*/ v_right[3] = {0};
    float trace_allsolid = 0.0f;
    float trace_startsolid = 0.0f;
    float trace_fraction = 0.0f;
    float/*vec3_t*/ trace_endpos[3] = {0};
    float/*vec3_t*/ trace_plane_normal[3] = {0};
    float trace_plane_dist = 0.0f;
    int trace_ent = 0;
    float trace_inopen = 0.0f;
    float trace_inwater = 0.0f;
    int msg_entity = 0;
    int/*func_t*/ main = 0;
    int/*func_t*/ StartFrame = 0;
    int/*func_t*/ PlayerPreThink = 0;
    int/*func_t*/ PlayerPostThink = 0;
    int/*func_t*/ ClientKill = 0;
    int/*func_t*/ ClientConnect = 0;
    int/*func_t*/ PutClientInServer = 0;
    int/*func_t*/ ClientDisconnect = 0;
    int/*func_t*/ SetNewParms = 0;
    int/*func_t*/ SetChangeParms = 0;
};

std::span<dfunction_t> GetFunctions();
std::span<char> GetStrings();
std::span<const ddef_t> GetFieldDefs();
std::span<const ddef_t> GetGlobalDefs();
std::span<dstatement_t> GetStatements();
std::span<globalvars_t> GetGlobals();
void GetGlobals2(std::span<globalvars_t> v, size_t& i);
void GetGlobals3(std::span<globalvars_t> v, size_t& i);
void GetGlobals4(std::span<globalvars_t> v, size_t& i);
void GetGlobals5(std::span<globalvars_t> v, size_t& i);
}
