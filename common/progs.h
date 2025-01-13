#pragma once
#include <array>
#include <span>
#include <vector>
#include <variant>
#include "progs_globals.h"

namespace Progs
{

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

using VarVector = std::array<float, 3>;
struct VarString { int data; };
struct VarFunction { int data; };
struct VarEntity { int data; };
using ProgsVariant = std::variant<std::monostate, float, VarVector, VarString, VarFunction, VarEntity, int>;

struct StackVar
{
    int offset;
    std::string_view name;
    ProgsVariant data;
};

struct dfunction_t
{
    int first_statement = 0; // negative numbers are builtins
    std::vector<dstatement_t> statements;
    int parm_start = 0;
    int locals = 0; // total ints of parms + locals

    mutable int profile = 0; // runtime

    std::string_view s_name;
    std::string_view s_file; // source file defined in

    int numparms = 0;
    std::array<unsigned char, 8> parm_size = {};
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

std::span<const dfunction_t> GetFunctions();
std::span<char> GetStrings();
std::span<const FieldDef> GetFieldDefs();
std::span<const ddef_t> GetGlobalDefs();
Progs::Globals& GetGlobalStruct();
}
