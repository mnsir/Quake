/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "host.h"
#include "console.h"
#include "server.h"
#include "sys.h"

#include <stdarg.h>
#include <string.h>

int		pr_argc;

/*
============
PR_RunError

Aborts the currently executing function
============
*/
void PR_RunError (const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,error);
	vsprintf (string,error,argptr);
	va_end (argptr);

	Con_Printf ("%s\n", string);
	
	Host_Error ("Program error");
}

/*
====================
PR_ExecuteProgram
====================
*/
void PR_ExecuteProgram (func_t fnum)
{
	dfunction_t* newf = &pr_functions[fnum];

	if (newf->first_statement < 0)
	{	// negative statements are built in functions
		int i = -newf->first_statement;
		if (i >= pr_numbuiltins)
			PR_RunError("Bad builtin call number");
		pr_builtins[i]();
	}
	else
	{
		// copy parameters
		int o = newf->parm_start;
		for (int i = 0; i < newf->numparms; i++)
		{
			for (int j = 0; j < newf->parm_size[i]; j++)
			{
				((int*)pr_globals)[o] = ((int*)pr_globals)[OFS_PARM0 + i * 3 + j];
				o++;
			}
		}

		int s = newf->first_statement;

		while (1)
		{
			dstatement_t* st = &pr_statements[s];
			eval_t* a = (eval_t*)&pr_globals[st->a];
			eval_t* b = (eval_t*)&pr_globals[st->b];
			eval_t* c = (eval_t*)&pr_globals[st->c];

			switch (st->op)
			{
			case OP_ADD_F:
				c->_float = a->_float + b->_float;
				break;
			case OP_ADD_V:
				c->vector[0] = a->vector[0] + b->vector[0];
				c->vector[1] = a->vector[1] + b->vector[1];
				c->vector[2] = a->vector[2] + b->vector[2];
				break;

			case OP_SUB_F:
				c->_float = a->_float - b->_float;
				break;
			case OP_SUB_V:
				c->vector[0] = a->vector[0] - b->vector[0];
				c->vector[1] = a->vector[1] - b->vector[1];
				c->vector[2] = a->vector[2] - b->vector[2];
				break;

			case OP_MUL_F:
				c->_float = a->_float * b->_float;
				break;
			case OP_MUL_V:
				c->_float = a->vector[0] * b->vector[0]
					+ a->vector[1] * b->vector[1]
					+ a->vector[2] * b->vector[2];
				break;
			case OP_MUL_FV:
				c->vector[0] = a->_float * b->vector[0];
				c->vector[1] = a->_float * b->vector[1];
				c->vector[2] = a->_float * b->vector[2];
				break;
			case OP_MUL_VF:
				c->vector[0] = b->_float * a->vector[0];
				c->vector[1] = b->_float * a->vector[1];
				c->vector[2] = b->_float * a->vector[2];
				break;

			case OP_DIV_F:
				c->_float = a->_float / b->_float;
				break;

			case OP_BITAND:
				c->_float = (int)a->_float & (int)b->_float;
				break;

			case OP_BITOR:
				c->_float = (int)a->_float | (int)b->_float;
				break;


			case OP_GE:
				c->_float = a->_float >= b->_float;
				break;
			case OP_LE:
				c->_float = a->_float <= b->_float;
				break;
			case OP_GT:
				c->_float = a->_float > b->_float;
				break;
			case OP_LT:
				c->_float = a->_float < b->_float;
				break;
			case OP_AND:
				c->_float = a->_float && b->_float;
				break;
			case OP_OR:
				c->_float = a->_float || b->_float;
				break;

			case OP_NOT_F:
				c->_float = !a->_float;
				break;
			case OP_NOT_V:
				c->_float = !a->vector[0] && !a->vector[1] && !a->vector[2];
				break;
			case OP_NOT_S:
				c->_float = !a->string || !pr_strings[a->string];
				break;
			case OP_NOT_FNC:
				c->_float = !a->function;
				break;
			case OP_NOT_ENT:
				c->_float = (PROG_TO_EDICT(a->edict) == sv.edicts);
				break;

			case OP_EQ_F:
				c->_float = a->_float == b->_float;
				break;
			case OP_EQ_V:
				c->_float = (a->vector[0] == b->vector[0]) &&
					(a->vector[1] == b->vector[1]) &&
					(a->vector[2] == b->vector[2]);
				break;
			case OP_EQ_S:
				c->_float = !strcmp(pr_strings + a->string, pr_strings + b->string);
				break;
			case OP_EQ_E:
				c->_float = a->_int == b->_int;
				break;
			case OP_EQ_FNC:
				c->_float = a->function == b->function;
				break;


			case OP_NE_F:
				c->_float = a->_float != b->_float;
				break;
			case OP_NE_V:
				c->_float = (a->vector[0] != b->vector[0]) ||
					(a->vector[1] != b->vector[1]) ||
					(a->vector[2] != b->vector[2]);
				break;
			case OP_NE_S:
				c->_float = strcmp(pr_strings + a->string, pr_strings + b->string);
				break;
			case OP_NE_E:
				c->_float = a->_int != b->_int;
				break;
			case OP_NE_FNC:
				c->_float = a->function != b->function;
				break;

				//==================
			case OP_STORE_F:
			case OP_STORE_ENT:
			case OP_STORE_FLD:		// integers
			case OP_STORE_S:
			case OP_STORE_FNC:		// pointers
				b->_int = a->_int;
				break;
			case OP_STORE_V:
				b->vector[0] = a->vector[0];
				b->vector[1] = a->vector[1];
				b->vector[2] = a->vector[2];
				break;

			case OP_STOREP_F:
			case OP_STOREP_ENT:
			case OP_STOREP_FLD:		// integers
			case OP_STOREP_S:
			case OP_STOREP_FNC:		// pointers
			{
				eval_t* ptr = (eval_t*)((byte*)sv.edicts + b->_int);
				ptr->_int = a->_int;
				break;
			}
			case OP_STOREP_V:
			{
				eval_t* ptr = (eval_t*)((byte*)sv.edicts + b->_int);
				ptr->vector[0] = a->vector[0];
				ptr->vector[1] = a->vector[1];
				ptr->vector[2] = a->vector[2];
				break;
			}
			case OP_ADDRESS:
			{
				edict_t* ed = PROG_TO_EDICT(a->edict);
#ifdef PARANOID
				NUM_FOR_EDICT(ed);		// make sure it's in range
#endif
				if (ed == (edict_t*)sv.edicts && sv.state == ss_active)
					PR_RunError("assignment to world entity");
				c->_int = (byte*)((int*)&ed->v + b->_int) - (byte*)sv.edicts;
				break;

			}
			case OP_LOAD_F:
			case OP_LOAD_FLD:
			case OP_LOAD_ENT:
			case OP_LOAD_S:
			case OP_LOAD_FNC:
			{
				edict_t* ed = PROG_TO_EDICT(a->edict);
#ifdef PARANOID
				NUM_FOR_EDICT(ed);		// make sure it's in range
#endif
				a = (eval_t*)((int*)&ed->v + b->_int);
				c->_int = a->_int;
				break;
			}
			case OP_LOAD_V:
			{
				edict_t* ed = PROG_TO_EDICT(a->edict);
#ifdef PARANOID
				NUM_FOR_EDICT(ed);		// make sure it's in range
#endif
				a = (eval_t*)((int*)&ed->v + b->_int);
				c->vector[0] = a->vector[0];
				c->vector[1] = a->vector[1];
				c->vector[2] = a->vector[2];
				break;
			}
			case OP_IFNOT:
				if (!a->_int)
					s += st->b - 1;	// offset the s++
				break;

			case OP_IF:
				if (a->_int)
					s += st->b - 1;	// offset the s++
				break;

			case OP_GOTO:
				s += st->a - 1;	// offset the s++
				break;

			case OP_CALL0:
			case OP_CALL1:
			case OP_CALL2:
			case OP_CALL3:
			case OP_CALL4:
			case OP_CALL5:
			case OP_CALL6:
			case OP_CALL7:
			case OP_CALL8:
				pr_argc = st->op - OP_CALL0;
				if (!a->function)
					PR_RunError("NULL function");

				PR_ExecuteProgram(a->function);
				break;

			case OP_DONE:
			case OP_RETURN:
				pr_globals[OFS_RETURN] = pr_globals[st->a];
				pr_globals[OFS_RETURN + 1] = pr_globals[st->a + 1];
				pr_globals[OFS_RETURN + 2] = pr_globals[st->a + 2];
return;		// all done
				break;

			case OP_STATE:
			{
				edict_t* ed = PROG_TO_EDICT(pr_global_struct->self);
#ifdef FPS_20
				ed->v.nextthink = pr_global_struct->time + 0.05;
#else
				ed->v.nextthink = pr_global_struct->time + 0.1;
#endif
				if (a->_float != ed->v.frame)
				{
					ed->v.frame = a->_float;
				}
				ed->v.think = b->function;
				break;
			}
			default:
				PR_RunError("Bad opcode %i", st->op);
			}
			s++;	// next statement
		}
	}
}
