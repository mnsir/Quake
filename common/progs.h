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

    mutable int profile = 0; // runtime

    std::string_view s_name;
    std::string_view s_file; // source file defined in

    int numparms = 0;
    std::array<unsigned char, 8> parm_size = {};
};

enum class Op : unsigned short
{
    OP_DONE,
    OP_MUL_F,
    OP_MUL_V,
    OP_MUL_FV,
    OP_MUL_VF,
    OP_DIV_F,
    OP_ADD_F,
    OP_ADD_V,
    OP_SUB_F,
    OP_SUB_V,

    OP_EQ_F,
    OP_EQ_V,
    OP_EQ_S,
    OP_EQ_E,
    OP_EQ_FNC,

    OP_NE_F,
    OP_NE_V,
    OP_NE_S,
    OP_NE_E,
    OP_NE_FNC,

    OP_LE,
    OP_GE,
    OP_LT,
    OP_GT,

    OP_LOAD_F,
    OP_LOAD_V,
    OP_LOAD_S,
    OP_LOAD_ENT,
    OP_LOAD_FLD,
    OP_LOAD_FNC,

    OP_ADDRESS,

    OP_STORE_F,
    OP_STORE_V,
    OP_STORE_S,
    OP_STORE_ENT,
    OP_STORE_FLD,
    OP_STORE_FNC,

    OP_STOREP_F,
    OP_STOREP_V,
    OP_STOREP_S,
    OP_STOREP_ENT,
    OP_STOREP_FLD,
    OP_STOREP_FNC,

    OP_RETURN,
    OP_NOT_F,
    OP_NOT_V,
    OP_NOT_S,
    OP_NOT_ENT,
    OP_NOT_FNC,
    OP_IF,
    OP_IFNOT,
    OP_CALL0,
    OP_CALL1,
    OP_CALL2,
    OP_CALL3,
    OP_CALL4,
    OP_CALL5,
    OP_CALL6,
    OP_CALL7,
    OP_CALL8,
    OP_STATE,
    OP_GOTO,
    OP_AND,
    OP_OR,

    OP_BITAND,
    OP_BITOR,
};


struct dstatement_t
{
    Op op;
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
    };

    bool save_global;
    etype_t type;
    unsigned short ofs;
    std::string_view s_name;
};

struct FieldDef
{
    enum class Type : uint8_t
    {
        ev_void,
        ev_string,
        ev_float,
        ev_vector,
        ev_entity,
        ev_function,
    };

    Type type;
    unsigned short ofs;
    std::string_view name;
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

std::span<const dfunction_t> GetFunctions();
std::span<char> GetStrings();
std::span<const FieldDef> GetFieldDefs();
std::span<const ddef_t> GetGlobalDefs();
std::span<const dstatement_t> GetStatements();
std::span<globalvars_t> GetGlobals();
void GetGlobals2(std::span<globalvars_t> v, size_t& i);
void GetGlobals3(std::span<globalvars_t> v, size_t& i);
void GetGlobals4(std::span<globalvars_t> v, size_t& i);
void GetGlobals5(std::span<globalvars_t> v, size_t& i);
}
