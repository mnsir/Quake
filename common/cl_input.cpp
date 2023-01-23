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
along with this_ program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl.input.c  -- builds an intended movement command to send to the server

// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.

#include "cl_input.h"

#include <cstdlib>

#include "client.h"
#include "cl_main.h"
#include "cmd.h"
#include "console.h"
#include "host.h"
#include "protocol.h"
#include "view.h"


/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition

===============================================================================
*/


kbutton_t in_mlook, in_klook;
kbutton_t in_left, in_right, in_forward, in_back;
kbutton_t in_lookup, in_lookdown, in_moveleft, in_moveright;
kbutton_t in_strafe, in_speed, in_use, in_jump, in_attack;
kbutton_t in_up, in_down;

int in_impulse;


void KeyDown(kbutton_t* b)
{
	int k;

	char* c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
		k = -1; // typed manually at the console for continuous down

	if (k == b->down[0] || k == b->down[1])
		return; // repeating key

	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else
	{
		Con_Printf((char*)"Three keys down for a button!\n");
		return;
	}

	if (b->state & 1)
		return; // still down
	b->state |= 1 + 2; // down + impulse down
}

void KeyUp(kbutton_t* b)
{
	int k;

	char* c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
	{
		// typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4; // impulse up
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return; // key up without coresponding down (menu pass through)
	if (b->down[0] || b->down[1])
		return; // some other key is still holding it down

	if (!(b->state & 1))
		return; // still up (this_ should not happen)
	b->state &= ~1; // now up
	b->state |= 4; // impulse up
}

void IN_KLookDown() { KeyDown(&in_klook); }
void IN_KLookUp() { KeyUp(&in_klook); }
void IN_MLookDown() { KeyDown(&in_mlook); }

void IN_MLookUp()
{
	KeyUp(&in_mlook);
	if (!(in_mlook.state & 1) && lookspring.value)
		V_StartPitchDrift();
}

void IN_UpDown() { KeyDown(&in_up); }
void IN_UpUp() { KeyUp(&in_up); }
void IN_DownDown() { KeyDown(&in_down); }
void IN_DownUp() { KeyUp(&in_down); }
void IN_LeftDown() { KeyDown(&in_left); }
void IN_LeftUp() { KeyUp(&in_left); }
void IN_RightDown() { KeyDown(&in_right); }
void IN_RightUp() { KeyUp(&in_right); }
void IN_ForwardDown() { KeyDown(&in_forward); }
void IN_ForwardUp() { KeyUp(&in_forward); }
void IN_BackDown() { KeyDown(&in_back); }
void IN_BackUp() { KeyUp(&in_back); }
void IN_LookupDown() { KeyDown(&in_lookup); }
void IN_LookupUp() { KeyUp(&in_lookup); }
void IN_LookdownDown() { KeyDown(&in_lookdown); }
void IN_LookdownUp() { KeyUp(&in_lookdown); }
void IN_MoveleftDown() { KeyDown(&in_moveleft); }
void IN_MoveleftUp() { KeyUp(&in_moveleft); }
void IN_MoverightDown() { KeyDown(&in_moveright); }
void IN_MoverightUp() { KeyUp(&in_moveright); }

void IN_SpeedDown() { KeyDown(&in_speed); }
void IN_SpeedUp() { KeyUp(&in_speed); }
void IN_StrafeDown() { KeyDown(&in_strafe); }
void IN_StrafeUp() { KeyUp(&in_strafe); }

void IN_AttackDown() { KeyDown(&in_attack); }
void IN_AttackUp() { KeyUp(&in_attack); }

void IN_UseDown() { KeyDown(&in_use); }
void IN_UseUp() { KeyUp(&in_use); }
void IN_JumpDown() { KeyDown(&in_jump); }
void IN_JumpUp() { KeyUp(&in_jump); }

void IN_Impulse() { in_impulse = Q_atoi(Cmd_Argv(1)); }

/*
===============
CL_KeyState

Returns 0.25 if a key was pressed and released during the frame,
0.5 if it was pressed and held
0 if held then released, and
1.0 if held for the entire time
===============
*/
float CL_KeyState(kbutton_t* key)
{
	bool impulsedown = key->state & 2;
	bool impulseup = key->state & 4;
	bool down = key->state & 1;
	float val = 0;

	if (impulsedown && !impulseup)
		if (down)
			val = 0.5; // pressed and held this_ frame
		else
			val = 0; // I_Error ();
	if (impulseup && !impulsedown)
		if (down)
			val = 0; // I_Error ();
		else
			val = 0; // released this_ frame
	if (!impulsedown && !impulseup)
		if (down)
			val = 1.0; // held the entire frame
		else
			val = 0; // up the entire frame
	if (impulsedown && impulseup)
		if (down)
			val = 0.75; // released and re-pressed this_ frame
		else
			val = 0.25; // pressed and released this_ frame

	key->state &= 1; // clear impulses

	return val;
}


//==========================================================================

cvar_t cl_upspeed = {(char*)"cl_upspeed", (char*)"200"};
cvar_t cl_forwardspeed = {(char*)"cl_forwardspeed", (char*)"200", true};
cvar_t cl_backspeed = {(char*)"cl_backspeed", (char*)"200", true};
cvar_t cl_sidespeed = {(char*)"cl_sidespeed", (char*)"350"};

cvar_t cl_movespeedkey = {(char*)"cl_movespeedkey", (char*)"2.0"};

cvar_t cl_yawspeed = {(char*)"cl_yawspeed", (char*)"140"};
cvar_t cl_pitchspeed = {(char*)"cl_pitchspeed", (char*)"150"};

cvar_t cl_anglespeedkey = {(char*)"cl_anglespeedkey", (char*)"1.5"};


/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles()
{
	float speed;

	if (in_speed.state & 1)
		speed = host_frametime * cl_anglespeedkey.value;
	else
		speed = host_frametime;

	if (!(in_strafe.state & 1))
	{
		cl.viewangles[YAW] -= speed * cl_yawspeed.value * CL_KeyState(&in_right);
		cl.viewangles[YAW] += speed * cl_yawspeed.value * CL_KeyState(&in_left);
		cl.viewangles[YAW] = anglemod(cl.viewangles[YAW]);
	}
	if (in_klook.state & 1)
	{
		V_StopPitchDrift();
		cl.viewangles[PITCH] -= speed * cl_pitchspeed.value * CL_KeyState(&in_forward);
		cl.viewangles[PITCH] += speed * cl_pitchspeed.value * CL_KeyState(&in_back);
	}

	float up = CL_KeyState(&in_lookup);
	float down = CL_KeyState(&in_lookdown);

	cl.viewangles[PITCH] -= speed * cl_pitchspeed.value * up;
	cl.viewangles[PITCH] += speed * cl_pitchspeed.value * down;

	if (up || down)
		V_StopPitchDrift();

	if (cl.viewangles[PITCH] > 80)
		cl.viewangles[PITCH] = 80;
	if (cl.viewangles[PITCH] < -70)
		cl.viewangles[PITCH] = -70;

	if (cl.viewangles[ROLL] > 50)
		cl.viewangles[ROLL] = 50;
	if (cl.viewangles[ROLL] < -50)
		cl.viewangles[ROLL] = -50;
}

/*
================
CL_BaseMove

Send the intended movement message to the server
================
*/
void CL_BaseMove(usercmd_t* cmd)
{
	if (cls.signon != SIGNONS)
		return;

	CL_AdjustAngles();

	Q_memset(cmd, 0, sizeof(*cmd));

	if (in_strafe.state & 1)
	{
		cmd->sidemove += cl_sidespeed.value * CL_KeyState(&in_right);
		cmd->sidemove -= cl_sidespeed.value * CL_KeyState(&in_left);
	}

	cmd->sidemove += cl_sidespeed.value * CL_KeyState(&in_moveright);
	cmd->sidemove -= cl_sidespeed.value * CL_KeyState(&in_moveleft);

	cmd->upmove += cl_upspeed.value * CL_KeyState(&in_up);
	cmd->upmove -= cl_upspeed.value * CL_KeyState(&in_down);

	if (!(in_klook.state & 1))
	{
		cmd->forwardmove += cl_forwardspeed.value * CL_KeyState(&in_forward);
		cmd->forwardmove -= cl_backspeed.value * CL_KeyState(&in_back);
	}

	//
	// adjust for speed key
	//
	if (in_speed.state & 1)
	{
		cmd->forwardmove *= cl_movespeedkey.value;
		cmd->sidemove *= cl_movespeedkey.value;
		cmd->upmove *= cl_movespeedkey.value;
	}
	
}


/*
==============
CL_SendMove
==============
*/
void CL_SendMove(usercmd_t* cmd)
{
	sizebuf_t buf;
	uint8_t data[128];

	buf.maxsize = 128;
	buf.cursize = 0;
	buf.data = data;

	cl.cmd = *cmd;

	//
	// send the movement message
	//
	MSG_WriteByte(&buf, clc_move);

	MSG_WriteFloat(&buf, cl.mtime[0]); // so server can get ping times

	for (int i = 0; i < 3; i++)
		MSG_WriteAngle(&buf, cl.viewangles[i]);

	MSG_WriteShort(&buf, cmd->forwardmove);
	MSG_WriteShort(&buf, cmd->sidemove);
	MSG_WriteShort(&buf, cmd->upmove);

	//
	// send button bits
	//
	int bits = 0;

	if (in_attack.state & 3)
		bits |= 1;
	in_attack.state &= ~2;

	if (in_jump.state & 3)
		bits |= 2;
	in_jump.state &= ~2;

	MSG_WriteByte(&buf, bits);

	MSG_WriteByte(&buf, in_impulse);
	in_impulse = 0;
	
	//
	// deliver the message
	//
	if (cls.demoplayback)
		return;

	//
	// allways dump the first two message, because it may contain leftover inputs
	// from the last level
	//
	if (++cl.movemessages <= 2)
		return;

	if (NET_SendUnreliableMessage(cls.netcon, &buf) == -1)
	{
		Con_Printf((char*)"CL_SendMove: lost server connection\n");
		CL_Disconnect();
	}
}

/*
============
CL_InitInput
============
*/
void CL_InitInput()
{
	Cmd_AddCommand((char*)"+moveup", IN_UpDown);
	Cmd_AddCommand((char*)"-moveup", IN_UpUp);
	Cmd_AddCommand((char*)"+movedown", IN_DownDown);
	Cmd_AddCommand((char*)"-movedown", IN_DownUp);
	Cmd_AddCommand((char*)"+left", IN_LeftDown);
	Cmd_AddCommand((char*)"-left", IN_LeftUp);
	Cmd_AddCommand((char*)"+right", IN_RightDown);
	Cmd_AddCommand((char*)"-right", IN_RightUp);
	Cmd_AddCommand((char*)"+forward", IN_ForwardDown);
	Cmd_AddCommand((char*)"-forward", IN_ForwardUp);
	Cmd_AddCommand((char*)"+back", IN_BackDown);
	Cmd_AddCommand((char*)"-back", IN_BackUp);
	Cmd_AddCommand((char*)"+lookup", IN_LookupDown);
	Cmd_AddCommand((char*)"-lookup", IN_LookupUp);
	Cmd_AddCommand((char*)"+lookdown", IN_LookdownDown);
	Cmd_AddCommand((char*)"-lookdown", IN_LookdownUp);
	Cmd_AddCommand((char*)"+strafe", IN_StrafeDown);
	Cmd_AddCommand((char*)"-strafe", IN_StrafeUp);
	Cmd_AddCommand((char*)"+moveleft", IN_MoveleftDown);
	Cmd_AddCommand((char*)"-moveleft", IN_MoveleftUp);
	Cmd_AddCommand((char*)"+moveright", IN_MoverightDown);
	Cmd_AddCommand((char*)"-moveright", IN_MoverightUp);
	Cmd_AddCommand((char*)"+speed", IN_SpeedDown);
	Cmd_AddCommand((char*)"-speed", IN_SpeedUp);
	Cmd_AddCommand((char*)"+attack", IN_AttackDown);
	Cmd_AddCommand((char*)"-attack", IN_AttackUp);
	Cmd_AddCommand((char*)"+use", IN_UseDown);
	Cmd_AddCommand((char*)"-use", IN_UseUp);
	Cmd_AddCommand((char*)"+jump", IN_JumpDown);
	Cmd_AddCommand((char*)"-jump", IN_JumpUp);
	Cmd_AddCommand((char*)"impulse", IN_Impulse);
	Cmd_AddCommand((char*)"+klook", IN_KLookDown);
	Cmd_AddCommand((char*)"-klook", IN_KLookUp);
	Cmd_AddCommand((char*)"+mlook", IN_MLookDown);
	Cmd_AddCommand((char*)"-mlook", IN_MLookUp);
}
