#pragma once

#include "common.h"
#include "mathlib.h"
#include "progs.h"

qboolean SV_CheckBottom(edict_t* ent);
qboolean SV_movestep(edict_t* ent, vec3_t move, qboolean relink);
void SV_MoveToGoal(void);