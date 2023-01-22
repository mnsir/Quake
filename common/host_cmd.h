#pragma once

#include "common.h"


extern qboolean noclip_anglehack;
extern int current_skill; // skill level for currently loaded level (in case
//  the user changes the cvar while the level is
//  running, this_ reflects the level actually in use)

void Host_InitCommands(void);
void Host_Quit_f(void);