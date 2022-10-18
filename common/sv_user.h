#pragma once

#include "progs.h"

extern edict_t* sv_player;

void SV_SetIdealPitch(void);
void SV_ClientThink(void);
void SV_RunClients(void);