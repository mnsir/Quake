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
// view.h
#pragma once

#include "typedef_byte.h"

#define CSHIFT_CONTENTS 0
#define CSHIFT_DAMAGE 1
#define CSHIFT_BONUS 2
#define CSHIFT_POWERUP 3

extern cvar_t v_gamma;

extern byte gammatable[256]; // palette is sent through this
extern byte ramps[3][256];
extern float v_blend[4];

extern cvar_t lcd_x;


void V_Init(void);
void V_RenderView(void);
float V_CalcRoll(vec3_t angles, vec3_t velocity);
void V_UpdatePalette(void);

//
// view
//
void V_StartPitchDrift(void);
void V_StopPitchDrift(void);
void V_RenderView(void);
void V_UpdatePalette(void);
void V_ParseDamage(void);
void V_SetContentsColor(int contents);
