

#include "quakedef.h"


/*

*/

typedef struct
{
    int s;
    const Progs::dfunction_t * f;
} prstack_t;

#define MAX_STACK_DEPTH 32
prstack_t pr_stack[MAX_STACK_DEPTH];
int pr_depth;

#define LOCALSTACK_SIZE 2048
int localstack[LOCALSTACK_SIZE];
int localstack_used;


bool pr_trace;
const Progs::dfunction_t * pr_xfunction;
int pr_xstatement;


int pr_argc;

char * PR_GlobalString(int ofs);
char * PR_GlobalStringNoContents(int ofs);

namespace
{
    void PR_PrintStatement(const Progs::dstatement_t& s)
    {
        using namespace std::string_view_literals;
        static constinit const auto pr_opnames = std::to_array({
            "DONE"sv,

            "MUL_F"sv,
            "MUL_V"sv,
            "MUL_FV"sv,
            "MUL_VF"sv,

            "DIV"sv,

            "ADD_F"sv,
            "ADD_V"sv,

            "SUB_F"sv,
            "SUB_V"sv,

            "EQ_F"sv,
            "EQ_V"sv,
            "EQ_S"sv,
            "EQ_E"sv,
            "EQ_FNC"sv,

            "NE_F"sv,
            "NE_V"sv,
            "NE_S"sv,
            "NE_E"sv,
            "NE_FNC"sv,

            "LE"sv,
            "GE"sv,
            "LT"sv,
            "GT"sv,

            "INDIRECT"sv,
            "INDIRECT"sv,
            "INDIRECT"sv,
            "INDIRECT"sv,
            "INDIRECT"sv,
            "INDIRECT"sv,

            "ADDRESS"sv,

            "STORE_F"sv,
            "STORE_V"sv,
            "STORE_S"sv,
            "STORE_ENT"sv,
            "STORE_FLD"sv,
            "STORE_FNC"sv,

            "STOREP_F"sv,
            "STOREP_V"sv,
            "STOREP_S"sv,
            "STOREP_ENT"sv,
            "STOREP_FLD"sv,
            "STOREP_FNC"sv,

            "RETURN"sv,

            "NOT_F"sv,
            "NOT_V"sv,
            "NOT_S"sv,
            "NOT_ENT"sv,
            "NOT_FNC"sv,

            "IF"sv,
            "IFNOT"sv,

            "CALL0"sv,
            "CALL1"sv,
            "CALL2"sv,
            "CALL3"sv,
            "CALL4"sv,
            "CALL5"sv,
            "CALL6"sv,
            "CALL7"sv,
            "CALL8"sv,

            "STATE"sv,

            "GOTO"sv,

            "AND"sv,
            "OR"sv,

            "BITAND"sv,
            "BITOR"sv,
        });

        using Underlying = std::underlying_type_t<decltype(s.op)>;

        if (static_cast<Underlying>(s.op) < pr_opnames.size())
        {
            auto&& name = pr_opnames[static_cast<Underlying>(s.op)];
            Con_Printf((char*)"%s ", name.data());
            for (size_t i = name.size(); i < 10; i++)
                Con_Printf((char*)" ");
        }

        if (s.op == Progs::Op::OP_IF || s.op == Progs::Op::OP_IFNOT)
        {
            Con_Printf((char*)"%sbranch %i", PR_GlobalString(s.a), s.b);
        }
        else if (s.op == Progs::Op::OP_GOTO)
        {
            Con_Printf((char*)"branch %i", s.a);
        }
        else if ((static_cast<Underlying>(s.op) - static_cast<Underlying>(Progs::Op::OP_STORE_F)) < 6)
        {
            Con_Printf((char*)"%s", PR_GlobalString(s.a));
            Con_Printf((char*)"%s", PR_GlobalStringNoContents(s.b));
        }
        else
        {
            if (s.a)
                Con_Printf((char*)"%s", PR_GlobalString(s.a));
            if (s.b)
                Con_Printf((char*)"%s", PR_GlobalString(s.b));
            if (s.c)
                Con_Printf((char*)"%s", PR_GlobalStringNoContents(s.c));
        }
        Con_Printf((char*)"\n");
    }
}
/*
============
PR_StackTrace
============
*/
void PR_StackTrace()
{
    if (pr_depth == 0)
    {
        Con_Printf((char*)"<NO STACK>\n");
        return;
    }

    pr_stack[pr_depth].f = pr_xfunction;
    for (int i = pr_depth; i >= 0; i--)
    {
        auto* f = pr_stack[i].f;

        if (!f)
        {
            Con_Printf((char*)"<NO FUNCTION>\n");
        }
        else
            Con_Printf((char*)"%12s : %s\n", f->s_file.data(), f->s_name.data());
    }
}


/*
============
PR_Profile_f

============
*/
void PR_Profile_f()
{
    const Progs::dfunction_t * best;
    int num = 0;
    do
    {
        int max = 0;
        best = nullptr;
        for (auto&& func : Progs::GetFunctions())
        {
            if (func.profile > max)
            {
                max = func.profile;
                best = &func;
            }
        }
        if (best)
        {
            if (num < 10)
                Con_Printf((char*)"%7i %s\n", best->profile, best->s_name.data());
            num++;
            best->profile = 0;
        }
    } while (best);
}


/*
============
PR_RunError

Aborts the currently executing function
============
*/
void PR_RunError(char * error, ...)
{
    va_list argptr;
    char string[1024];

    va_start(argptr, error);
    vsprintf(string, error, argptr);
    va_end(argptr);

    PR_PrintStatement(Progs::GetStatements()[pr_xstatement]);
    PR_StackTrace();
    Con_Printf((char*)"%s\n", string);

    pr_depth = 0; // dump the stack so host_error can shutdown functions

    Host_Error((char*)"Program error");
}

/*
============================================================================
PR_ExecuteProgram

The interpretation main loop
============================================================================
*/

//Returns the new program statement counter
int PR_EnterFunction(const Progs::dfunction_t& f)
{
    int i, j, c, o;

    pr_stack[pr_depth].s = pr_xstatement;
    pr_stack[pr_depth].f = pr_xfunction;
    pr_depth++;
    if (pr_depth >= MAX_STACK_DEPTH)
        PR_RunError((char*)"stack overflow");

    // save off any locals that the new function steps on
    c = f.locals;
    if (localstack_used + c > LOCALSTACK_SIZE)
        PR_RunError((char*)"PR_ExecuteProgram: locals stack overflow\n");

    for (i = 0; i < c; i++)
        localstack[localstack_used + i] = ((int *)pr_globals)[f.parm_start + i];
    localstack_used += c;

    // copy parameters
    o = f.parm_start;
    for (i = 0; i < f.numparms; i++)
    {
        for (j = 0; j < f.parm_size[i]; j++)
        {
            ((int *)pr_globals)[o] = ((int *)pr_globals)[OFS_PARM0 + i * 3 + j];
            o++;
        }
    }

    pr_xfunction = &f;
    return f.first_statement - 1; // offset the s++
}

/*
====================
PR_LeaveFunction
====================
*/
int PR_LeaveFunction()
{
    int i, c;

    if (pr_depth <= 0)
        Sys_Error((char*)"prog stack underflow");

    // restore locals from the stack
    c = pr_xfunction->locals;
    localstack_used -= c;
    if (localstack_used < 0)
        PR_RunError((char*)"PR_ExecuteProgram: locals stack underflow\n");

    for (i = 0; i < c; i++)
        ((int *)pr_globals)[pr_xfunction->parm_start + i] = localstack[localstack_used + i];

    // up stack
    pr_depth--;
    pr_xfunction = pr_stack[pr_depth].f;
    return pr_stack[pr_depth].s;
}


/*
====================
PR_ExecuteProgram
====================
*/
void PR_ExecuteProgram(func_t fnum)
{
    eval_t * a, * b, * c;
    int s;
    int runaway;
    int i;
    edict_t * ed;
    int exitdepth;
    eval_t * ptr;

    auto funcs = Progs::GetFunctions();
    if (!fnum || fnum >= funcs.size())
    {
        if (pr_global_struct->self)
            ED_Print(PROG_TO_EDICT(pr_global_struct->self));
        Host_Error((char*)"PR_ExecuteProgram: NULL function");
    }

    auto&& f = funcs[fnum];

    runaway = 100000;
    pr_trace = false;

    // make a stack frame
    exitdepth = pr_depth;

    s = PR_EnterFunction(f);

    while (1)
    {
        s++; // next statement

        auto&& st = Progs::GetStatements()[s];
        a = (eval_t *)&pr_globals[st.a];
        b = (eval_t *)&pr_globals[st.b];
        c = (eval_t *)&pr_globals[st.c];

        if (!--runaway)
            PR_RunError((char*)"runaway loop error");

        pr_xfunction->profile++;
        pr_xstatement = s;

        if (pr_trace)
            PR_PrintStatement(st);

        switch (st.op)
        {
        case Progs::Op::OP_ADD_F:
            c->_float = a->_float + b->_float;
            break;
        case Progs::Op::OP_ADD_V:
            c->vector[0] = a->vector[0] + b->vector[0];
            c->vector[1] = a->vector[1] + b->vector[1];
            c->vector[2] = a->vector[2] + b->vector[2];
            break;

        case Progs::Op::OP_SUB_F:
            c->_float = a->_float - b->_float;
            break;
        case Progs::Op::OP_SUB_V:
            c->vector[0] = a->vector[0] - b->vector[0];
            c->vector[1] = a->vector[1] - b->vector[1];
            c->vector[2] = a->vector[2] - b->vector[2];
            break;

        case Progs::Op::OP_MUL_F:
            c->_float = a->_float * b->_float;
            break;
        case Progs::Op::OP_MUL_V:
            c->_float = a->vector[0] * b->vector[0]
                + a->vector[1] * b->vector[1]
                + a->vector[2] * b->vector[2];
            break;
        case Progs::Op::OP_MUL_FV:
            c->vector[0] = a->_float * b->vector[0];
            c->vector[1] = a->_float * b->vector[1];
            c->vector[2] = a->_float * b->vector[2];
            break;
        case Progs::Op::OP_MUL_VF:
            c->vector[0] = b->_float * a->vector[0];
            c->vector[1] = b->_float * a->vector[1];
            c->vector[2] = b->_float * a->vector[2];
            break;

        case Progs::Op::OP_DIV_F:
            c->_float = a->_float / b->_float;
            break;

        case Progs::Op::OP_BITAND:
            c->_float = (int)a->_float & (int)b->_float;
            break;

        case Progs::Op::OP_BITOR:
            c->_float = (int)a->_float | (int)b->_float;
            break;


        case Progs::Op::OP_GE:
            c->_float = a->_float >= b->_float;
            break;
        case Progs::Op::OP_LE:
            c->_float = a->_float <= b->_float;
            break;
        case Progs::Op::OP_GT:
            c->_float = a->_float > b->_float;
            break;
        case Progs::Op::OP_LT:
            c->_float = a->_float < b->_float;
            break;
        case Progs::Op::OP_AND:
            c->_float = a->_float && b->_float;
            break;
        case Progs::Op::OP_OR:
            c->_float = a->_float || b->_float;
            break;

        case Progs::Op::OP_NOT_F:
            c->_float = !a->_float;
            break;
        case Progs::Op::OP_NOT_V:
            c->_float = !a->vector[0] && !a->vector[1] && !a->vector[2];
            break;
        case Progs::Op::OP_NOT_S:
            c->_float = !a->string || !pr_strings[a->string];
            break;
        case Progs::Op::OP_NOT_FNC:
            c->_float = !a->function;
            break;
        case Progs::Op::OP_NOT_ENT:
            c->_float = (PROG_TO_EDICT(a->edict) == sv.edicts);
            break;

        case Progs::Op::OP_EQ_F:
            c->_float = a->_float == b->_float;
            break;
        case Progs::Op::OP_EQ_V:
            c->_float = (a->vector[0] == b->vector[0]) &&
                (a->vector[1] == b->vector[1]) &&
                (a->vector[2] == b->vector[2]);
            break;
        case Progs::Op::OP_EQ_S:
            c->_float = !strcmp(pr_strings + a->string, pr_strings + b->string);
            break;
        case Progs::Op::OP_EQ_E:
            c->_float = a->_int == b->_int;
            break;
        case Progs::Op::OP_EQ_FNC:
            c->_float = a->function == b->function;
            break;


        case Progs::Op::OP_NE_F:
            c->_float = a->_float != b->_float;
            break;
        case Progs::Op::OP_NE_V:
            c->_float = (a->vector[0] != b->vector[0]) ||
                (a->vector[1] != b->vector[1]) ||
                (a->vector[2] != b->vector[2]);
            break;
        case Progs::Op::OP_NE_S:
            c->_float = strcmp(pr_strings + a->string, pr_strings + b->string);
            break;
        case Progs::Op::OP_NE_E:
            c->_float = a->_int != b->_int;
            break;
        case Progs::Op::OP_NE_FNC:
            c->_float = a->function != b->function;
            break;

            //==================
        case Progs::Op::OP_STORE_F:
        case Progs::Op::OP_STORE_ENT:
        case Progs::Op::OP_STORE_FLD: // integers
        case Progs::Op::OP_STORE_S:
        case Progs::Op::OP_STORE_FNC: // pointers
            b->_int = a->_int;
            break;
        case Progs::Op::OP_STORE_V:
            b->vector[0] = a->vector[0];
            b->vector[1] = a->vector[1];
            b->vector[2] = a->vector[2];
            break;

        case Progs::Op::OP_STOREP_F:
        case Progs::Op::OP_STOREP_ENT:
        case Progs::Op::OP_STOREP_FLD: // integers
        case Progs::Op::OP_STOREP_S:
        case Progs::Op::OP_STOREP_FNC: // pointers
            ptr = (eval_t *)((byte *)sv.edicts + b->_int);
            ptr->_int = a->_int;
            break;
        case Progs::Op::OP_STOREP_V:
            ptr = (eval_t *)((byte *)sv.edicts + b->_int);
            ptr->vector[0] = a->vector[0];
            ptr->vector[1] = a->vector[1];
            ptr->vector[2] = a->vector[2];
            break;

        case Progs::Op::OP_ADDRESS:
            ed = PROG_TO_EDICT(a->edict);
            if (ed == (edict_t *)sv.edicts && sv.state == ss_active)
                PR_RunError((char*)"assignment to world entity");
            c->_int = (byte *)((int *)&ed->v + b->_int) - (byte *)sv.edicts;
            break;

        case Progs::Op::OP_LOAD_F:
        case Progs::Op::OP_LOAD_FLD:
        case Progs::Op::OP_LOAD_ENT:
        case Progs::Op::OP_LOAD_S:
        case Progs::Op::OP_LOAD_FNC:
            ed = PROG_TO_EDICT(a->edict);
            a = (eval_t *)((int *)&ed->v + b->_int);
            c->_int = a->_int;
            break;

        case Progs::Op::OP_LOAD_V:
            ed = PROG_TO_EDICT(a->edict);
            a = (eval_t *)((int *)&ed->v + b->_int);
            c->vector[0] = a->vector[0];
            c->vector[1] = a->vector[1];
            c->vector[2] = a->vector[2];
            break;

            //==================

        case Progs::Op::OP_IFNOT:
            if (!a->_int)
                s += st.b - 1; // offset the s++
            break;

        case Progs::Op::OP_IF:
            if (a->_int)
                s += st.b - 1; // offset the s++
            break;

        case Progs::Op::OP_GOTO:
            s += st.a - 1; // offset the s++
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
            pr_argc = (unsigned)st.op - (unsigned)Progs::Op::OP_CALL0;
            if (!a->function)
                PR_RunError((char*)"NULL function");

            auto&& newf = Progs::GetFunctions()[a->function];

            if (newf.first_statement < 0)
            { // negative statements are built in functions
                i = -newf.first_statement;
                if (i >= pr_numbuiltins)
                    PR_RunError((char*)"Bad builtin call number");
                pr_builtins[i]();
                break;
            }

            s = PR_EnterFunction(newf);
            break;
        }
        case Progs::Op::OP_DONE:
        case Progs::Op::OP_RETURN:
            pr_globals[OFS_RETURN] = pr_globals[st.a];
            pr_globals[OFS_RETURN + 1] = pr_globals[st.a + 1];
            pr_globals[OFS_RETURN + 2] = pr_globals[st.a + 2];

            s = PR_LeaveFunction();
            if (pr_depth == exitdepth)
                return; // all done
            break;

        case Progs::Op::OP_STATE:
            ed = PROG_TO_EDICT(pr_global_struct->self);
            ed->v.nextthink = pr_global_struct->time + 0.1;
            if (a->_float != ed->v.frame)
            {
                ed->v.frame = a->_float;
            }
            ed->v.think = b->function;
            break;

        default:
            PR_RunError((char*)"Bad opcode %i", st.op);
        }
    }

}
