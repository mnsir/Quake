#pragma once

#include "progs.h"

extern edict_t* sv_player;

void SV_SetIdealPitch();
void SV_ClientThink();
void SV_RunClients();