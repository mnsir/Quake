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
#pragma once

#include <stdio.h>

#include "progdefs.h"
#include "progs.h"
#include "pr_comp.h"


extern dprograms_t* progs;
extern dfunction_t* pr_functions;
extern char* pr_strings;
extern ddef_t* pr_globaldefs;
extern ddef_t* pr_fielddefs;
extern dstatement_t* pr_statements;
extern globalvars_t* pr_global_struct;
extern float* pr_globals; // same as pr_global_struct

extern int pr_edict_size; // in bytes

//============================================================================

void PR_Init(void);
void PR_LoadProgs(void);
edict_t* ED_Alloc(void);
void ED_Free(edict_t* ed);
// returns a copy of the string allocated from the server's string heap
char* ED_NewString(char* string);
void ED_Print(edict_t* ed);
void ED_Write(FILE* f, edict_t* ed);
char* ED_ParseEdict(char* data, edict_t* ent);
void ED_WriteGlobals(FILE* f);
void ED_ParseGlobals(char* data);
void ED_LoadFromFile(char* data);

edict_t* EDICT_NUM(int n);
int NUM_FOR_EDICT(edict_t* e);

extern int type_size[8];
extern unsigned short pr_crc;
void ED_PrintEdicts(void);

void ED_PrintNum(int ent);

eval_t* GetEdictFieldValue(edict_t* ed, char* field);
