#pragma once

#include "common.h"
#include "mathlib.h"
#include "progs.h"

bool SV_CheckBottom(edict_t* ent);
bool SV_movestep(edict_t* ent, const vec3_t& move, bool relink);
void SV_MoveToGoal();