

#include "quakedef.h"
#include <numeric>
#include <variant>

/*
============
PR_RunError

Aborts the currently executing function
============
*/
void PR_RunError(const char* error, ...)
{
    va_list argptr;
    char string[1024];

    va_start(argptr, error);
    vsprintf(string, error, argptr);
    va_end(argptr);

    Con_Printf("%s\n", string);

    Host_Error("Program error");
}

std::pair<const Progs::ProgsVariant, const Progs::ProgsVariant> ToVariant(const Progs::dstatement_t& st)
{
    auto func = [](const Progs::dstatement_t& st) -> std::pair<Progs::ProgsVariant, Progs::ProgsVariant>
        {
            constexpr Progs::ProgsVariant asNone;
            constexpr Progs::ProgsVariant asFloat = 0.0f;
            constexpr Progs::ProgsVariant asVector = std::array<float, 3>{};
            constexpr Progs::ProgsVariant asString = Progs::VarString{};
            constexpr Progs::ProgsVariant asFunction = Progs::VarFunction{};
            constexpr Progs::ProgsVariant asEntity = Progs::VarEntity{};
            constexpr Progs::ProgsVariant asInt = 0;

            switch (st.op)
            {
            case Progs::Op::OP_ADD_F: return { asFloat, asFloat };
            case Progs::Op::OP_ADD_V: return { asVector, asVector };
            case Progs::Op::OP_SUB_F: return { asFloat, asFloat };
            case Progs::Op::OP_SUB_V: return { asVector, asVector };
            case Progs::Op::OP_MUL_F: return { asFloat, asFloat };
            case Progs::Op::OP_MUL_V: return { asVector, asVector };
            case Progs::Op::OP_MUL_FV: return { asFloat, asVector };
            case Progs::Op::OP_MUL_VF: return { asVector, asFloat };
            case Progs::Op::OP_DIV_F: return { asFloat, asFloat };
            case Progs::Op::OP_BITAND: return { asFloat, asFloat };
            case Progs::Op::OP_BITOR: return { asFloat, asFloat };
            case Progs::Op::OP_GE: return { asFloat, asFloat };
            case Progs::Op::OP_LE: return { asFloat, asFloat };
            case Progs::Op::OP_GT: return { asFloat, asFloat };
            case Progs::Op::OP_LT: return { asFloat, asFloat };
            case Progs::Op::OP_AND: return { asFloat, asFloat };
            case Progs::Op::OP_OR: return { asFloat, asFloat };

            case Progs::Op::OP_NOT_F: return { asFloat, asNone };
            case Progs::Op::OP_NOT_V: return { asVector, asNone };
            case Progs::Op::OP_NOT_S: return { asString, asNone };
            case Progs::Op::OP_NOT_FNC: return { asFunction, asNone };
            case Progs::Op::OP_NOT_ENT: return { asEntity, asNone };

            case Progs::Op::OP_EQ_F: return { asFloat, asFloat };
            case Progs::Op::OP_EQ_V: return { asVector, asVector };
            case Progs::Op::OP_EQ_S: return { asString, asString };
            case Progs::Op::OP_EQ_E: return { asInt, asInt };
            case Progs::Op::OP_EQ_FNC: return { asFunction, asFunction };

            case Progs::Op::OP_NE_F: return { asFloat, asFloat };
            case Progs::Op::OP_NE_V: return { asVector, asVector };
            case Progs::Op::OP_NE_S: return { asString, asString };
            case Progs::Op::OP_NE_E: return { asInt, asInt };
            case Progs::Op::OP_NE_FNC: return { asFunction, asFunction };

            case Progs::Op::OP_STORE_F:
            case Progs::Op::OP_STORE_ENT:
            case Progs::Op::OP_STORE_FLD: // integers
            case Progs::Op::OP_STORE_S:
            case Progs::Op::OP_STORE_FNC: return { asInt, asInt };
            case Progs::Op::OP_STORE_V: return { asVector, asVector };

            case Progs::Op::OP_STOREP_F:
            case Progs::Op::OP_STOREP_ENT:
            case Progs::Op::OP_STOREP_FLD: // integers
            case Progs::Op::OP_STOREP_S:
            case Progs::Op::OP_STOREP_FNC: return { asInt, asInt };
            case Progs::Op::OP_STOREP_V: return { asVector, asInt };
            case Progs::Op::OP_ADDRESS: return { asEntity, asInt };
            case Progs::Op::OP_LOAD_F:
            case Progs::Op::OP_LOAD_FLD:
            case Progs::Op::OP_LOAD_ENT:
            case Progs::Op::OP_LOAD_S:
            case Progs::Op::OP_LOAD_FNC: return { asEntity, asInt };
            case Progs::Op::OP_LOAD_V: return { asEntity, asInt };
            case Progs::Op::OP_IFNOT: return { asFloat, asNone };
            case Progs::Op::OP_IF: return { asFloat, asNone };
            case Progs::Op::OP_GOTO: return { asNone, asNone };
            case Progs::Op::OP_CALL0:
            case Progs::Op::OP_CALL1:
            case Progs::Op::OP_CALL2:
            case Progs::Op::OP_CALL3:
            case Progs::Op::OP_CALL4:
            case Progs::Op::OP_CALL5:
            case Progs::Op::OP_CALL6:
            case Progs::Op::OP_CALL7:
            case Progs::Op::OP_CALL8: return { asFunction, asNone };
            case Progs::Op::OP_DONE:
            case Progs::Op::OP_RETURN: return { asNone, asNone };
            case Progs::Op::OP_STATE: return { asFloat, asFunction };
            default: return { asNone, asNone };
                   return { asNone, asNone };
            }
        };

    auto func1 = [](Progs::ProgsVariant& a, const eval_t& out)
        {
            if (auto p = std::get_if<float>(&a))
            {
                *p = out._float;
            }
            else if (auto p = std::get_if<Progs::VarVector>(&a))
            {
                (*p)[0] = out.vector[0];
                (*p)[1] = out.vector[1];
                (*p)[2] = out.vector[2];
            }
            else if (auto p = std::get_if<Progs::VarString>(&a))
            {
                p->data = out.string;
            }
            else if (auto p = std::get_if<Progs::VarFunction>(&a))
            {
                p->data = out.function;
            }
            else if (auto p = std::get_if<Progs::VarEntity>(&a))
            {
                p->data = out.edict;
            }
            else if (auto p = std::get_if<int>(&a))
            {
                *p = out._int;
            }
        };

    auto&& [a, b] = func(st);
    eval_t* a_ = reinterpret_cast<eval_t*>(&pr_globals[st.a]);
    eval_t* b_ = reinterpret_cast<eval_t*>(&pr_globals[st.b]);
    func1(a, *a_);
    func1(b, *b_);
    return { a, b };
}

void FromVariant(const Progs::ProgsVariant& c, const Progs::dstatement_t& st)
{
    auto func = [](const Progs::ProgsVariant& a, eval_t& out)
        {
            if (auto p = std::get_if<float>(&a))
            {
                out._float = *p;
            }
            else if (auto p = std::get_if<Progs::VarVector>(&a))
            {
                out.vector[0] = (*p)[0];
                out.vector[1] = (*p)[1];
                out.vector[2] = (*p)[2];
            }
            else if (auto p = std::get_if<Progs::VarString>(&a))
            {
                out.string = p->data;
            }
            else if (auto p = std::get_if<Progs::VarFunction>(&a))
            {
                out.function = p->data;
            }
            else if (auto p = std::get_if<Progs::VarEntity>(&a))
            {
                out.edict = p->data;
            }
            else if (auto p = std::get_if<int>(&a))
            {
                out._int = *p;
            }
        };
    eval_t* c_ = reinterpret_cast<eval_t*>(&pr_globals[st.c]);
    func(c, *c_);
}

/*
====================
PR_ExecuteProgram
====================
*/
void PR_ExecuteProgram(func_t fnum)
{
    auto&& func = Progs::GetFunctions()[fnum];

    if (func.first_statement < 0)
    { // negative statements are built in functions
        int i = -func.first_statement;
        auto&& builtIns = Progs::GetBuiltIns();
        if (i >= builtIns.size())
            PR_RunError("Bad builtin call number");
        builtIns[i]();
    }
    else
    {
        // copy parameters
        int o = func.parm_start;
        for (int i = 0; i < func.numparms; i++)
        {
            for (int j = 0; j < func.parm_size[i]; j++)
            {
                ((int*)pr_globals)[o] = ((int*)pr_globals)[OFS_PARM0 + i * 3 + j];
                o++;
            }
        }

        int statement_num = 0;

        while (1)
        {
            auto&& st = func.statements[statement_num];
            auto&& [a, b] = ToVariant(st);
            Progs::ProgsVariant c;

            switch (st.op)
            {
            case Progs::Op::OP_ADD_F:
            {
                c = std::get<float>(a) + std::get<float>(b);
            }

            break;
            case Progs::Op::OP_ADD_V:
            {
                auto&& v1 = std::get<Progs::VarVector>(a);
                auto&& v2 = std::get<Progs::VarVector>(b);
                c = Progs::VarVector{ v1[0] + v2[0], v1[1] + v2[1], v1[2] + v2[2] };
            }
            break;

            case Progs::Op::OP_SUB_F:
            {
                c = std::get<float>(a) - std::get<float>(b);
            }
            break;
            case Progs::Op::OP_SUB_V:
            {
                auto&& v1 = std::get<Progs::VarVector>(a);
                auto&& v2 = std::get<Progs::VarVector>(b);
                c = Progs::VarVector{ v1[0] - v2[0], v1[1] - v2[1], v1[2] - v2[2] };
            }
            break;

            case Progs::Op::OP_MUL_F:
            {
                c = std::get<float>(a) * std::get<float>(b);
            }
            break;
            case Progs::Op::OP_MUL_V:
            {
                auto&& v1 = std::get<Progs::VarVector>(a);
                auto&& v2 = std::get<Progs::VarVector>(b);
                c = static_cast<float>(v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2]);
            }
            break;
            case Progs::Op::OP_MUL_FV:
            {
                auto&& v1 = std::get<float>(a);
                auto&& v2 = std::get<Progs::VarVector>(b);
                c = Progs::VarVector{ v1 * v2[0], v1 * v2[1], v1 * v2[2] };
            }
            break;
            case Progs::Op::OP_MUL_VF:
            {
                auto&& v1 = std::get<Progs::VarVector>(a);
                auto&& v2 = std::get<float>(b);
                c = Progs::VarVector{ v2 * v1[0], v2 * v1[1], v2 * v1[2] };
            }
            break;

            case Progs::Op::OP_DIV_F:
            {
                c = std::get<float>(a) / std::get<float>(b);
            }
            break;

            case Progs::Op::OP_BITAND:
            {
                c = static_cast<float>(static_cast<int>(std::get<float>(a)) & static_cast<int>(std::get<float>(b)));
            }
            break;
            case Progs::Op::OP_BITOR:
            {
                c = static_cast<float>(static_cast<int>(std::get<float>(a)) | static_cast<int>(std::get<float>(b)));
            }
            break;

            case Progs::Op::OP_GE:
            {
                c = static_cast<float>(std::get<float>(a) >= std::get<float>(b));
            }
            break;
            case Progs::Op::OP_LE:
            {
                c = static_cast<float>(std::get<float>(a) <= std::get<float>(b));
            }
            break;
            case Progs::Op::OP_GT:
            {
                c = static_cast<float>(std::get<float>(a) > std::get<float>(b));
            }
            break;
            case Progs::Op::OP_LT:
            {
                c = static_cast<float>(std::get<float>(a) < std::get<float>(b));
            }
            break;
            case Progs::Op::OP_AND:
            {
                c = static_cast<float>(std::get<float>(a) && std::get<float>(b));
            }
            break;
            case Progs::Op::OP_OR:
            {
                c = static_cast<float>(std::get<float>(a) || std::get<float>(b));
            }
            break;

            case Progs::Op::OP_NOT_F:
            {
                c = static_cast<float>(!std::get<float>(a));
            }
            break;
            case Progs::Op::OP_NOT_V:
            {
                c = static_cast<float>(!std::get<Progs::VarVector>(a)[0] && !std::get<Progs::VarVector>(a)[1] && !std::get<Progs::VarVector>(a)[2]);
            }
            break;
            case Progs::Op::OP_NOT_S:
            {
                c = static_cast<float>(!std::get<Progs::VarString>(a).data || !*Progs::FromStringOffset(std::get<Progs::VarString>(a).data));
            }
            break;
            case Progs::Op::OP_NOT_FNC:
            {
                c = static_cast<float>(!std::get<Progs::VarFunction>(a).data);
            }
            break;
            case Progs::Op::OP_NOT_ENT:
            {
                c = static_cast<float>(PROG_TO_EDICT(std::get<Progs::VarEntity>(a).data) == sv.edicts);
            }
            break;

            case Progs::Op::OP_EQ_F:
            {
                c = static_cast<float>(std::get<float>(a) == std::get<float>(b));
            }
            break;
            case Progs::Op::OP_EQ_V:
            {
                c = static_cast<float>(std::get<Progs::VarVector>(a) == std::get<Progs::VarVector>(b));
            }
            break;
            case Progs::Op::OP_EQ_S:
            {
                c = static_cast<float>(!strcmp(Progs::FromStringOffset(std::get<Progs::VarString>(a).data), Progs::FromStringOffset(std::get<Progs::VarString>(b).data)));
            }
            break;
            case Progs::Op::OP_EQ_E:
            {
                c = static_cast<float>(std::get<int>(a) == std::get<int>(b));
            }
            break;
            case Progs::Op::OP_EQ_FNC:
            {
                c = static_cast<float>(std::get<Progs::VarFunction>(a).data == std::get<Progs::VarFunction>(b).data);
            }
            break;


            case Progs::Op::OP_NE_F:
            {
                c = static_cast<float>(std::get<float>(a) != std::get<float>(b));
            }
            break;
            case Progs::Op::OP_NE_V:
            {
                c = static_cast<float>(std::get<Progs::VarVector>(a) != std::get<Progs::VarVector>(b));
            }
            break;
            case Progs::Op::OP_NE_S:
            {
                c = static_cast<float>(strcmp(Progs::FromStringOffset(std::get<Progs::VarString>(a).data), Progs::FromStringOffset(std::get<Progs::VarString>(b).data)));
            }
            break;
            case Progs::Op::OP_NE_E:
            {
                c = static_cast<float>(std::get<int>(a) != std::get<int>(b));
            }
            break;
            case Progs::Op::OP_NE_FNC:
            {
                c = static_cast<float>(std::get<Progs::VarFunction>(a).data != std::get<Progs::VarFunction>(b).data);
            }
            break;

            //==================
            case Progs::Op::OP_STORE_F:
            case Progs::Op::OP_STORE_ENT:
            case Progs::Op::OP_STORE_FLD:
            case Progs::Op::OP_STORE_S:
            case Progs::Op::OP_STORE_FNC:
            case Progs::Op::OP_STORE_V:
            {
                c = a;
            }
            break;

            case Progs::Op::OP_STOREP_F:
            case Progs::Op::OP_STOREP_ENT:
            case Progs::Op::OP_STOREP_FLD: // integers
            case Progs::Op::OP_STOREP_S:
            case Progs::Op::OP_STOREP_FNC: // pointers
            {
                eval_t* ptr = (eval_t*)((byte*)sv.edicts + std::get<int>(b));
                ptr->_int = std::get<int>(a);
            }
            break;
            case Progs::Op::OP_STOREP_V:
            {
                eval_t* ptr = (eval_t*)((byte*)sv.edicts + std::get<int>(b));
                ptr->vector[0] = std::get<Progs::VarVector>(a)[0];
                ptr->vector[1] = std::get<Progs::VarVector>(a)[1];
                ptr->vector[2] = std::get<Progs::VarVector>(a)[2];
            }
            break;
            case Progs::Op::OP_ADDRESS:
            {
                edict_t* ed = PROG_TO_EDICT(std::get<Progs::VarEntity>(a).data);
                if (ed == (edict_t*)sv.edicts && sv.state == ss_active)
                    PR_RunError("assignment to world entity");
                c = (byte*)((int*)&ed->entvars + std::get<int>(b)) - (byte*)sv.edicts;
            }
            break;
            case Progs::Op::OP_LOAD_F:
            case Progs::Op::OP_LOAD_FLD:
            case Progs::Op::OP_LOAD_ENT:
            case Progs::Op::OP_LOAD_S:
            case Progs::Op::OP_LOAD_FNC:
            {
                edict_t* ed = PROG_TO_EDICT(std::get<Progs::VarEntity>(a).data);
                eval_t* ptr = (eval_t*)((int*)&ed->entvars + std::get<int>(b));

                if (std::get<int>(b) > 194)
                    ptr->_int = 111000111;

                c = ptr->_int;
            }
            break;
            case Progs::Op::OP_LOAD_V:
            {
                edict_t* ed = PROG_TO_EDICT(std::get<Progs::VarEntity>(a).data);
                eval_t* ptr = (eval_t*)((int*)&ed->entvars + std::get<int>(b));
                c = Progs::VarVector{ ptr->vector[0], ptr->vector[1], ptr->vector[2] };
            }
            break;
            case Progs::Op::OP_IFNOT:
            {
                if (!std::get<float>(a))
                    statement_num += st.b - 1; // offset the s++
            }
            break;

            case Progs::Op::OP_IF:
            {
                if (std::get<float>(a))
                    statement_num += st.b - 1; // offset the s++
            }
            break;

            case Progs::Op::OP_GOTO:
                statement_num += st.a - 1; // offset the s++
                break;

            case Progs::Op::OP_CALL0:
            case Progs::Op::OP_CALL1:
            case Progs::Op::OP_CALL2:
            case Progs::Op::OP_CALL3:
            case Progs::Op::OP_CALL4:
            case Progs::Op::OP_CALL5:
            case Progs::Op::OP_CALL6:
            case Progs::Op::OP_CALL7:
            case Progs::Op::OP_CALL8:
            {
                if (!std::get<Progs::VarFunction>(a).data)
                    PR_RunError("NULL function");

                PR_ExecuteProgram(std::get<Progs::VarFunction>(a).data);
            }
            break;
            case Progs::Op::OP_DONE:
            case Progs::Op::OP_RETURN:
                pr_globals[OFS_RETURN] = pr_globals[st.a];
                pr_globals[OFS_RETURN + 1] = pr_globals[st.a + 1];
                pr_globals[OFS_RETURN + 2] = pr_globals[st.a + 2];

                return;
                break;

            case Progs::Op::OP_STATE:
            {
                edict_t* ed = PROG_TO_EDICT(Progs::GetGlobalStruct().self);
                ed->entvars.nextthink = Progs::GetGlobalStruct().time + 0.1;
                if (std::get<float>(a) != ed->entvars.frame)
                {
                    ed->entvars.frame = std::get<float>(a);
                }
                ed->entvars.think = std::get<Progs::VarFunction>(b).data;

            }
            break;
            default:
                PR_RunError("Bad opcode %i", st.op);
            }
            FromVariant(c, st);
            statement_num++;
        }
    }
}
