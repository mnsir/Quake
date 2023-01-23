#pragma once

#include "cvar.h"
#include "mathlib.h"


typedef struct
{
	vec3_t viewangles;

	// intended velocities
	float forwardmove;
	float sidemove;
	float upmove;
} usercmd_t;


typedef struct
{
	int down[2]; // key nums holding it down
	int state; // low bit is down state
} kbutton_t;


extern cvar_t cl_upspeed;
extern cvar_t cl_forwardspeed;
extern cvar_t cl_backspeed;
extern cvar_t cl_sidespeed;
extern cvar_t cl_movespeedkey;
extern cvar_t cl_yawspeed;
extern cvar_t cl_pitchspeed;
extern cvar_t cl_anglespeedkey;

extern kbutton_t in_mlook;
extern kbutton_t in_klook;
extern kbutton_t in_strafe;
extern kbutton_t in_speed;


void CL_InitInput();
void CL_SendMove(usercmd_t* cmd);
void CL_BaseMove(usercmd_t* cmd);
float CL_KeyState(kbutton_t* key);

