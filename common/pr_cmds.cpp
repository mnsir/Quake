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

#include "pr_cmds.h"

#include <cmath>
#include <cstring>
#include <format>
#include <numbers>

#include "cmd.h"
#include "common.h"
#include "console.h"
#include "host.h"
#include "progdefs.h"
#include "progs.h"
#include "protocol.h"
#include "pr_comp.h"
#include "pr_edict.h"
#include "pr_exec.h"
#include "sv_main.h"
#include "sv_move.h"
#include "sys.h"
#include "world.h"


#define RETURN_EDICT(e) (((int *)pr_globals)[OFS_RETURN] = EDICT_TO_PROG(e))

/*
===============================================================================

 BUILT-IN FUNCTIONS

===============================================================================
*/

char* PF_VarString(int first)
{
	static char out[256];

	out[0] = 0;
	for (int i = first; i < pr_argc; i++)
	{
		strcat(out, G_STRING((OFS_PARM0 + i * 3)));
	}
	return out;
}


/*
=================
PF_errror

This is a TERMINAL error, which will kill off the entire server.
Dumps self.

error(value)
=================
*/
void PF_error()
{
	char* s = PF_VarString(0);
	Con_Printf((char*)"======SERVER ERROR in %s:\n%s\n"
	           , pr_strings + pr_xfunction->s_name, s);
	edict_t* ed = PROG_TO_EDICT(pr_global_struct->self);
	ED_Print(ed);

	Host_Error((char*)"Program error");
}

/*
=================
PF_objerror

Dumps out self, then an error message.  The program is aborted and self is
removed, but the level can continue.

objerror(value)
=================
*/
void PF_objerror()
{
	char* s = PF_VarString(0);
	Con_Printf((char*)"======OBJECT ERROR in %s:\n%s\n"
	           , pr_strings + pr_xfunction->s_name, s);
	edict_t* ed = PROG_TO_EDICT(pr_global_struct->self);
	ED_Print(ed);
	ED_Free(ed);

	Host_Error((char*)"Program error");
}


/*
==============
PF_makevectors

Writes new_ values for v_forward, v_up, and v_right based on angles
makevectors(vector)
==============
*/
void PF_makevectors()
{
	AngleVectors(G_VECTOR(OFS_PARM0), pr_global_struct->v_forward, pr_global_struct->v_right, pr_global_struct->v_up);
}

/*
=================
PF_setorigin

This is the only valid way to move an object without using the physics of the world (setting velocity and waiting).  Directly changing origin will not set internal links correctly, so clipping would be messed up.  This should be called when an object is spawned, and then only if it is teleported.

setorigin (entity, origin)
=================
*/
void PF_setorigin()
{
	edict_t* e = G_EDICT(OFS_PARM0);
	float* org = G_VECTOR(OFS_PARM1);
	VectorCopy(org, e->v.origin);
	SV_LinkEdict(e, false);
}


void SetMinMaxSize(edict_t* e, float* min, float* max, bool rotate)
{
	vec3_t rmin, rmax;
	float bounds[2][3];
	float xvector[2], yvector[2];
	vec3_t base, transformed;
	int i;

	for (i = 0; i < 3; i++)
		if (min[i] > max[i])
			PR_RunError((char*)"backwards mins/maxs");

	rotate = false; // FIXME: implement rotation properly again

	if (!rotate)
	{
		VectorCopy(min, rmin);
		VectorCopy(max, rmax);
	}
	else
	{
		// find min / max for rotations
		float* angles = e->v.angles;

		float a = DEG2RAD(angles[1]);

		xvector[0] = cos(a);
		xvector[1] = sin(a);
		yvector[0] = -sin(a);
		yvector[1] = cos(a);

		VectorCopy(min, bounds[0]);
		VectorCopy(max, bounds[1]);

		rmin[0] = rmin[1] = rmin[2] = 9999;
		rmax[0] = rmax[1] = rmax[2] = -9999;

		for (i = 0; i <= 1; i++)
		{
			base[0] = bounds[i][0];
			for (int j = 0; j <= 1; j++)
			{
				base[1] = bounds[j][1];
				for (int k = 0; k <= 1; k++)
				{
					base[2] = bounds[k][2];

					// transform the point
					transformed[0] = xvector[0] * base[0] + yvector[0] * base[1];
					transformed[1] = xvector[1] * base[0] + yvector[1] * base[1];
					transformed[2] = base[2];

					for (int l = 0; l < 3; l++)
					{
						if (transformed[l] < rmin[l])
							rmin[l] = transformed[l];
						if (transformed[l] > rmax[l])
							rmax[l] = transformed[l];
					}
				}
			}
		}
	}

	// set derived values
	VectorCopy(rmin, e->v.mins);
	VectorCopy(rmax, e->v.maxs);
	VectorSubtract(max, min, e->v.size);

	SV_LinkEdict(e, false);
}

/*
=================
PF_setsize

the size box is rotated by the current angle

setsize (entity, minvector, maxvector)
=================
*/
void PF_setsize()
{
	edict_t* e = G_EDICT(OFS_PARM0);
	float* min = G_VECTOR(OFS_PARM1);
	float* max = G_VECTOR(OFS_PARM2);
	SetMinMaxSize(e, min, max, false);
}


/*
=================
PF_setmodel

setmodel(entity, model)
=================
*/
void PF_setmodel()
{
	char**check;
	int i;

	edict_t* e = G_EDICT(OFS_PARM0);
	char* m = G_STRING(OFS_PARM1);

	// check to see if model was properly precached
	for (i = 0, check = sv.model_precache; *check; i++, check++)
		if (!strcmp(*check, m))
			break;

	if (!*check)
		PR_RunError((char*)"no precache: %s\n", m);


	e->v.model = m - pr_strings;
	e->v.modelindex = i; //SV_ModelIndex (m);

	model_t* mod = sv.models[(int)e->v.modelindex]; // Mod_ForName (m, true);

	if (mod)
		SetMinMaxSize(e, mod->mins, mod->maxs, true);
	else
		SetMinMaxSize(e, vec3_origin, vec3_origin, true);
}

/*
=================
PF_bprint

broadcast print to everyone on server

bprint(value)
=================
*/
void PF_bprint()
{
	char* s = PF_VarString(0);
	SV_BroadcastPrintf((char*)"%s", s);
}

/*
=================
PF_sprint

single print to a specific client

sprint(clientent, value)
=================
*/
void PF_sprint()
{
	int entnum = G_EDICTNUM(OFS_PARM0);
	char* s = PF_VarString(1);

	if (entnum < 1 || entnum > svs.maxclients)
	{
		Con_Printf((char*)"tried to sprint to a non-client\n");
		return;
	}

	client_t* client = &svs.clients[entnum - 1];

	MSG_WriteChar(&client->message, svc_print);
	MSG_WriteString(&client->message, s);
}


/*
=================
PF_centerprint

single print to a specific client

centerprint(clientent, value)
=================
*/
void PF_centerprint()
{
	int entnum = G_EDICTNUM(OFS_PARM0);
	char* s = PF_VarString(1);

	if (entnum < 1 || entnum > svs.maxclients)
	{
		Con_Printf((char*)"tried to sprint to a non-client\n");
		return;
	}

	client_t* client = &svs.clients[entnum - 1];

	MSG_WriteChar(&client->message, svc_centerprint);
	MSG_WriteString(&client->message, s);
}


/*
=================
PF_normalize

vector normalize(vector)
=================
*/
void PF_normalize()
{
	vec3_t new_value;

	float* value1 = G_VECTOR(OFS_PARM0);

	float new_ = value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2];
	new_ = sqrt(new_);

	if (new_ == 0)
		new_value[0] = new_value[1] = new_value[2] = 0;
	else
	{
		new_ = 1 / new_;
		new_value[0] = value1[0] * new_;
		new_value[1] = value1[1] * new_;
		new_value[2] = value1[2] * new_;
	}

	VectorCopy(new_value, G_VECTOR(OFS_RETURN));
}

/*
=================
PF_vlen

scalar vlen(vector)
=================
*/
void PF_vlen()
{
	float* value1 = G_VECTOR(OFS_PARM0);

	float new_ = value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2];
	new_ = sqrt(new_);

	G_FLOAT(OFS_RETURN) = new_;
}

/*
=================
PF_vectoyaw

float vectoyaw(vector)
=================
*/
void PF_vectoyaw()
{
	float yaw;

	float* value1 = G_VECTOR(OFS_PARM0);

	if (value1[1] == 0 && value1[0] == 0)
		yaw = 0;
	else
	{
		yaw = (int)(atan2(value1[1], value1[0]) * 180 / std::numbers::pi);
		if (yaw < 0)
			yaw += 360;
	}

	G_FLOAT(OFS_RETURN) = yaw;
}


/*
=================
PF_vectoangles

vector vectoangles(vector)
=================
*/
void PF_vectoangles()
{
	float yaw, pitch;

	float* value1 = G_VECTOR(OFS_PARM0);

	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = (int)(atan2(value1[1], value1[0]) * 180 / std::numbers::pi);
		if (yaw < 0)
			yaw += 360;

		float forward = sqrt(value1[0] * value1[0] + value1[1] * value1[1]);
		pitch = (int)(atan2(value1[2], forward) * 180 / std::numbers::pi);
		if (pitch < 0)
			pitch += 360;
	}

	G_FLOAT(OFS_RETURN + 0) = pitch;
	G_FLOAT(OFS_RETURN + 1) = yaw;
	G_FLOAT(OFS_RETURN + 2) = 0;
}

/*
=================
PF_Random

Returns a number from 0<= num < 1

random()
=================
*/
void PF_random()
{
	float num = (rand() & 0x7fff) / ((float)0x7fff);

	G_FLOAT(OFS_RETURN) = num;
}

/*
=================
PF_particle

particle(origin, color, count)
=================
*/
void PF_particle()
{
	float* org = G_VECTOR(OFS_PARM0);
	float* dir = G_VECTOR(OFS_PARM1);
	float color = G_FLOAT(OFS_PARM2);
	float count = G_FLOAT(OFS_PARM3);
	SV_StartParticle(org, dir, color, count);
}


/*
=================
PF_ambientsound

=================
*/
void PF_ambientsound()
{
	char** check;
	int soundnum;

	float* pos = G_VECTOR(OFS_PARM0);
	char* samp = G_STRING(OFS_PARM1);
	float vol = G_FLOAT(OFS_PARM2);
	float attenuation = G_FLOAT(OFS_PARM3);

	// check to see if samp was properly precached
	for (soundnum = 0, check = sv.sound_precache; *check; check++, soundnum++)
		if (!strcmp(*check, samp))
			break;

	if (!*check)
	{
		Con_Printf((char*)"no precache: %s\n", samp);
		return;
	}

	// add an svc_spawnambient command to the level signon packet

	MSG_WriteByte(&sv.signon, svc_spawnstaticsound);
	for (int i = 0; i < 3; i++)
		MSG_WriteCoord(&sv.signon, pos[i]);

	MSG_WriteByte(&sv.signon, soundnum);

	MSG_WriteByte(&sv.signon, vol * 255);
	MSG_WriteByte(&sv.signon, attenuation * 64);
}

/*
=================
PF_sound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.

=================
*/
void PF_sound()
{
	using namespace std::string_view_literals;
	edict_t* entity = G_EDICT(OFS_PARM0);
	int channel = G_FLOAT(OFS_PARM1);
	char* sample = G_STRING(OFS_PARM2);
	int volume = G_FLOAT(OFS_PARM3) * 255;
	float attenuation = G_FLOAT(OFS_PARM4);

	if (volume < 0 || volume > 255)
		Sys_Error(std::format("SV_StartSound: volume = {}"sv, volume));

	if (attenuation < 0 || attenuation > 4)
		Sys_Error(std::format("SV_StartSound: attenuation = {}"sv, attenuation));

	if (channel < 0 || channel > 7)
		Sys_Error(std::format("SV_StartSound: channel = {}"sv, channel));

	SV_StartSound(entity, channel, sample, volume, attenuation);
}

/*
=================
PF_break

break()
=================
*/
void PF_break()
{
	Con_Printf((char*)"break statement\n");
	*(int*)-4 = 0; // dump to debugger
	// PR_RunError ((char*)"break statement");
}

/*
=================
PF_traceline

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

traceline (vector1, vector2, tryents)
=================
*/
void PF_traceline()
{
	float* v1 = G_VECTOR(OFS_PARM0);
	float* v2 = G_VECTOR(OFS_PARM1);
	int nomonsters = G_FLOAT(OFS_PARM2);
	edict_t* ent = G_EDICT(OFS_PARM3);

	trace_t trace = SV_Move(v1, vec3_origin, vec3_origin, v2, nomonsters, ent);

	pr_global_struct->trace_allsolid = trace.allsolid;
	pr_global_struct->trace_startsolid = trace.startsolid;
	pr_global_struct->trace_fraction = trace.fraction;
	pr_global_struct->trace_inwater = trace.inwater;
	pr_global_struct->trace_inopen = trace.inopen;
	VectorCopy(trace.endpos, pr_global_struct->trace_endpos);
	VectorCopy(trace.plane.normal, pr_global_struct->trace_plane_normal);
	pr_global_struct->trace_plane_dist = trace.plane.dist;
	if (trace.ent)
		pr_global_struct->trace_ent = EDICT_TO_PROG(trace.ent);
	else
		pr_global_struct->trace_ent = EDICT_TO_PROG(sv.edicts);
}



/*
=================
PF_checkpos

Returns true if the given entity can move to the given position from it's
current position by walking or rolling.
FIXME: make work...
scalar checkpos (entity, vector)
=================
*/
void PF_checkpos()
{
}

//============================================================================

byte checkpvs[MAX_MAP_LEAFS / 8];

int PF_new_checkclient(int check)
{
	int i;
	edict_t* ent;
	vec3_t org;

	// cycle to the next one

	if (check < 1)
		check = 1;
	if (check > svs.maxclients)
		check = svs.maxclients;

	if (check == svs.maxclients)
		i = 1;
	else
		i = check + 1;

	for (; ; i++)
	{
		if (i == svs.maxclients + 1)
			i = 1;

		ent = EDICT_NUM(i);

		if (i == check)
			break; // didn't find anything else

		if (ent->free)
			continue;
		if (ent->v.health <= 0)
			continue;
		if ((int)ent->v.flags & FL_NOTARGET)
			continue;

		// anything that is a client, or has a client as an enemy
		break;
	}

	// get the PVS for the entity
	VectorAdd(ent->v.origin, ent->v.view_ofs, org);
	mleaf_t* leaf = Mod_PointInLeaf(org, sv.worldmodel);
	byte* pvs = Mod_LeafPVS(leaf, sv.worldmodel);
	memcpy(checkpvs, pvs, (sv.worldmodel->numleafs + 7) >> 3);

	return i;
}

/*
=================
PF_checkclient

Returns a client (or object that has a client enemy) that would be a
valid target.

If there are more than one valid options, they are cycled each frame

If (self.origin + self.viewofs) is not in the PVS of the current target,
it is not returned at all.

name checkclient ()
=================
*/
#define MAX_CHECK 16
int c_invis, c_notvis;

void PF_checkclient()
{
	vec3_t view;

	// find a new_ check if on a new_ frame
	if (sv.time - sv.lastchecktime >= 0.1)
	{
		sv.lastcheck = PF_new_checkclient(sv.lastcheck);
		sv.lastchecktime = sv.time;
	}

	// return check if it might be visible 
	edict_t* ent = EDICT_NUM(sv.lastcheck);
	if (ent->free || ent->v.health <= 0)
	{
		RETURN_EDICT(sv.edicts);
		return;
	}

	// if current entity can't possibly see the check entity, return 0
	edict_t* self = PROG_TO_EDICT(pr_global_struct->self);
	VectorAdd(self->v.origin, self->v.view_ofs, view);
	mleaf_t* leaf = Mod_PointInLeaf(view, sv.worldmodel);
	int l = (leaf - sv.worldmodel->leafs) - 1;
	if ((l < 0) || !(checkpvs[l >> 3] & (1 << (l & 7))))
	{
		c_notvis++;
		RETURN_EDICT(sv.edicts);
		return;
	}

	// might be able to see it
	c_invis++;
	RETURN_EDICT(ent);
}

//============================================================================


/*
=================
PF_stuffcmd

Sends text over to the client's execution buffer

stuffcmd (clientent, value)
=================
*/
void PF_stuffcmd()
{
	int entnum = G_EDICTNUM(OFS_PARM0);
	if (entnum < 1 || entnum > svs.maxclients)
		PR_RunError((char*)"Parm 0 not a client");
	char* str = G_STRING(OFS_PARM1);

	client_t* old = host_client;
	host_client = &svs.clients[entnum - 1];
	Host_ClientCommands((char*)"%s", str);
	host_client = old;
}

/*
=================
PF_localcmd

Sends text over to the client's execution buffer

localcmd (string)
=================
*/
void PF_localcmd()
{
	char* str = G_STRING(OFS_PARM0);
	Cbuf_AddText(str);
}

/*
=================
PF_cvar

float cvar (string)
=================
*/
void PF_cvar()
{
	char* str = G_STRING(OFS_PARM0);

	G_FLOAT(OFS_RETURN) = Cvar_VariableValue(str);
}

/*
=================
PF_cvar_set

float cvar (string)
=================
*/
void PF_cvar_set()
{
	char* var = G_STRING(OFS_PARM0);
	char* val = G_STRING(OFS_PARM1);

	Cvar_Set(var, val);
}

/*
=================
PF_findradius

Returns a chain of entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
void PF_findradius()
{
	vec3_t eorg;

	edict_t* chain = sv.edicts;

	float* org = G_VECTOR(OFS_PARM0);
	float rad = G_FLOAT(OFS_PARM1);

	edict_t* ent = NEXT_EDICT(sv.edicts);
	for (int i = 1; i < sv.num_edicts; i++, ent = NEXT_EDICT(ent))
	{
		if (ent->free)
			continue;
		if (ent->v.solid == SOLID_NOT)
			continue;
		for (int j = 0; j < 3; j++)
			eorg[j] = org[j] - (ent->v.origin[j] + (ent->v.mins[j] + ent->v.maxs[j]) * 0.5);
		if (Length(eorg) > rad)
			continue;

		ent->v.chain = EDICT_TO_PROG(chain);
		chain = ent;
	}

	RETURN_EDICT(chain);
}


/*
=========
PF_dprint
=========
*/
void PF_dprint()
{
	Con_DPrintf((char*)"%s", PF_VarString(0));
}

char pr_string_temp[128];

void PF_ftos()
{
	float v = G_FLOAT(OFS_PARM0);

	if (v == (int)v)
		sprintf(pr_string_temp, (char*)"%d", (int)v);
	else
		sprintf(pr_string_temp, (char*)"%5.1f", v);
	G_INT(OFS_RETURN) = pr_string_temp - pr_strings;
}

void PF_fabs()
{
	float v = G_FLOAT(OFS_PARM0);
	G_FLOAT(OFS_RETURN) = fabs(v);
}

void PF_vtos()
{
	sprintf(pr_string_temp, (char*)"'%5.1f %5.1f %5.1f'", G_VECTOR(OFS_PARM0)[0], G_VECTOR(OFS_PARM0)[1],
	        G_VECTOR(OFS_PARM0)[2]);
	G_INT(OFS_RETURN) = pr_string_temp - pr_strings;
}


void PF_Spawn()
{
	edict_t* ed = ED_Alloc();
	RETURN_EDICT(ed);
}

void PF_Remove()
{
	edict_t* ed = G_EDICT(OFS_PARM0);
	ED_Free(ed);
}


// entity (entity start, .string field, string match) find = #5;
void PF_Find()
{
	int e = G_EDICTNUM(OFS_PARM0);
	int f = G_INT(OFS_PARM1);
	char* s = G_STRING(OFS_PARM2);
	if (!s)
		PR_RunError((char*)"PF_Find: bad search string");

	for (e++; e < sv.num_edicts; e++)
	{
		edict_t* ed = EDICT_NUM(e);
		if (ed->free)
			continue;
		char* t = E_STRING(ed, f);
		if (!t)
			continue;
		if (!strcmp(t, s))
		{
			RETURN_EDICT(ed);
			return;
		}
	}

	RETURN_EDICT(sv.edicts);
}


void PR_CheckEmptyString(char* s)
{
	if (s[0] <= ' ')
		PR_RunError((char*)"Bad string");
}

void PF_precache_file()
{
	// precache_file is only used to copy files with qcc, it does nothing
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
}

void PF_precache_sound()
{
	if (sv.state != ss_loading)
		PR_RunError((char*)"PF_Precache_*: Precache can only be done in spawn functions");

	char* s = G_STRING(OFS_PARM0);
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
	PR_CheckEmptyString(s);

	for (int i = 0; i < MAX_SOUNDS; i++)
	{
		if (!sv.sound_precache[i])
		{
			sv.sound_precache[i] = s;
			return;
		}
		if (!strcmp(sv.sound_precache[i], s))
			return;
	}
	PR_RunError((char*)"PF_precache_sound: overflow");
}

void PF_precache_model()
{
	if (sv.state != ss_loading)
		PR_RunError((char*)"PF_Precache_*: Precache can only be done in spawn functions");

	char* s = G_STRING(OFS_PARM0);
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
	PR_CheckEmptyString(s);

	for (int i = 0; i < MAX_MODELS; i++)
	{
		if (!sv.model_precache[i])
		{
			sv.model_precache[i] = s;
			sv.models[i] = Mod_ForName(s, true);
			return;
		}
		if (!strcmp(sv.model_precache[i], s))
			return;
	}
	PR_RunError((char*)"PF_precache_model: overflow");
}


void PF_coredump()
{
	ED_PrintEdicts();
}

void PF_traceon()
{
	pr_trace = true;
}

void PF_traceoff()
{
	pr_trace = false;
}

void PF_eprint()
{
	ED_PrintNum(G_EDICTNUM(OFS_PARM0));
}

/*
===============
PF_walkmove

float(float yaw, float dist) walkmove
===============
*/
void PF_walkmove()
{
	vec3_t move;

	edict_t* ent = PROG_TO_EDICT(pr_global_struct->self);
	float yaw = G_FLOAT(OFS_PARM0);
	float dist = G_FLOAT(OFS_PARM1);

	if (!((int)ent->v.flags & (FL_ONGROUND | FL_FLY | FL_SWIM)))
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	yaw = DEG2RAD(yaw);

	move[0] = cos(yaw) * dist;
	move[1] = sin(yaw) * dist;
	move[2] = 0;

	// save program state, because SV_movestep may call other progs
	dfunction_t* oldf = pr_xfunction;
	int oldself = pr_global_struct->self;

	G_FLOAT(OFS_RETURN) = SV_movestep(ent, move, true);


	// restore program state
	pr_xfunction = oldf;
	pr_global_struct->self = oldself;
}

/*
===============
PF_droptofloor

void() droptofloor
===============
*/
void PF_droptofloor()
{
	vec3_t end;

	edict_t* ent = PROG_TO_EDICT(pr_global_struct->self);

	VectorCopy(ent->v.origin, end);
	end[2] -= 256;

	trace_t trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, false, ent);

	if (trace.fraction == 1 || trace.allsolid)
		G_FLOAT(OFS_RETURN) = 0;
	else
	{
		VectorCopy(trace.endpos, ent->v.origin);
		SV_LinkEdict(ent, false);
		ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
		ent->v.groundentity = EDICT_TO_PROG(trace.ent);
		G_FLOAT(OFS_RETURN) = 1;
	}
}

/*
===============
PF_lightstyle

void(float style, string value) lightstyle
===============
*/
void PF_lightstyle()
{
	client_t* client;
	int j;

	int style = G_FLOAT(OFS_PARM0);
	char* val = G_STRING(OFS_PARM1);

	// change the string in sv
	sv.lightstyles[style] = val;

	// send message to all clients on this_ server
	if (sv.state != ss_active)
		return;

	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
		if (client->active || client->spawned)
		{
			MSG_WriteChar(&client->message, svc_lightstyle);
			MSG_WriteChar(&client->message, style);
			MSG_WriteString(&client->message, val);
		}
}

void PF_rint()
{
	float f = G_FLOAT(OFS_PARM0);
	if (f > 0)
		G_FLOAT(OFS_RETURN) = (int)(f + 0.5);
	else
		G_FLOAT(OFS_RETURN) = (int)(f - 0.5);
}

void PF_floor()
{
	G_FLOAT(OFS_RETURN) = floor(G_FLOAT(OFS_PARM0));
}

void PF_ceil()
{
	G_FLOAT(OFS_RETURN) = ceil(G_FLOAT(OFS_PARM0));
}


/*
=============
PF_checkbottom
=============
*/
void PF_checkbottom()
{
	edict_t* ent = G_EDICT(OFS_PARM0);

	G_FLOAT(OFS_RETURN) = SV_CheckBottom(ent);
}

/*
=============
PF_pointcontents
=============
*/
void PF_pointcontents()
{
	float* v = G_VECTOR(OFS_PARM0);

	G_FLOAT(OFS_RETURN) = SV_PointContents(v);
}

/*
=============
PF_nextent

entity nextent(entity)
=============
*/
void PF_nextent()
{
	int i = G_EDICTNUM(OFS_PARM0);
	while (true)
	{
		i++;
		if (i == sv.num_edicts)
		{
			RETURN_EDICT(sv.edicts);
			return;
		}
		edict_t* ent = EDICT_NUM(i);
		if (!ent->free)
		{
			RETURN_EDICT(ent);
			return;
		}
	}
}

/*
=============
PF_aim

Pick a vector for the player to shoot along
vector aim(entity, missilespeed)
=============
*/
cvar_t sv_aim = {(char*)"sv_aim", (char*)"0.93"};

void PF_aim()
{
	vec3_t start, dir, end, bestdir;
	float dist;

	edict_t* ent = G_EDICT(OFS_PARM0);
	float speed = G_FLOAT(OFS_PARM1);

	VectorCopy(ent->v.origin, start);
	start[2] += 20;

	// try sending a trace straight
	VectorCopy(pr_global_struct->v_forward, dir);
	VectorMA(start, 2048, dir, end);
	trace_t tr = SV_Move(start, vec3_origin, vec3_origin, end, false, ent);
	if (tr.ent && tr.ent->v.takedamage == DAMAGE_AIM
		&& (!teamplay.value || ent->v.team <= 0 || ent->v.team != tr.ent->v.team))
	{
		VectorCopy(pr_global_struct->v_forward, G_VECTOR(OFS_RETURN));
		return;
	}


	// try all possible entities
	VectorCopy(dir, bestdir);
	float bestdist = sv_aim.value;
	edict_t* bestent = NULL;

	edict_t* check = NEXT_EDICT(sv.edicts);
	for (int i = 1; i < sv.num_edicts; i++, check = NEXT_EDICT(check))
	{
		if (check->v.takedamage != DAMAGE_AIM)
			continue;
		if (check == ent)
			continue;
		if (teamplay.value && ent->v.team > 0 && ent->v.team == check->v.team)
			continue; // don't aim at teammate
		for (int j = 0; j < 3; j++)
			end[j] = check->v.origin[j]
				+ 0.5 * (check->v.mins[j] + check->v.maxs[j]);
		VectorSubtract(end, start, dir);
		VectorNormalize(dir);
		dist = DotProduct(dir, pr_global_struct->v_forward);
		if (dist < bestdist)
			continue; // to far to turn
		tr = SV_Move(start, vec3_origin, vec3_origin, end, false, ent);
		if (tr.ent == check)
		{
			// can shoot at this_ one
			bestdist = dist;
			bestent = check;
		}
	}

	if (bestent)
	{
		VectorSubtract(bestent->v.origin, ent->v.origin, dir);
		dist = DotProduct(dir, pr_global_struct->v_forward);
		VectorScale(pr_global_struct->v_forward, dist, end);
		end[2] = dir[2];
		VectorNormalize(end);
		VectorCopy(end, G_VECTOR(OFS_RETURN));
	}
	else
	{
		VectorCopy(bestdir, G_VECTOR(OFS_RETURN));
	}
}

/*
==============
PF_changeyaw

This was a major timewaster in progs, so it was converted to C
==============
*/
void PF_changeyaw()
{
	edict_t* ent = PROG_TO_EDICT(pr_global_struct->self);
	float current = anglemod(ent->v.angles[1]);
	float ideal = ent->v.ideal_yaw;
	float speed = ent->v.yaw_speed;

	if (current == ideal)
		return;
	float move = ideal - current;
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}

	ent->v.angles[1] = anglemod(current + move);
}


/*
===============================================================================

MESSAGE WRITING

===============================================================================
*/

#define MSG_BROADCAST 0 // unreliable to all
#define MSG_ONE 1 // reliable to one (msg_entity)
#define MSG_ALL 2 // reliable to all
#define MSG_INIT 3 // write to the init string

sizebuf_t* WriteDest()
{
	int entnum;
	edict_t* ent;

	int dest = G_FLOAT(OFS_PARM0);
	switch (dest)
	{
	case MSG_BROADCAST:
		return &sv.datagram;

	case MSG_ONE:
		ent = PROG_TO_EDICT(pr_global_struct->msg_entity);
		entnum = NUM_FOR_EDICT(ent);
		if (entnum < 1 || entnum > svs.maxclients)
			PR_RunError((char*)"WriteDest: not a client");
		return &svs.clients[entnum - 1].message;

	case MSG_ALL:
		return &sv.reliable_datagram;

	case MSG_INIT:
		return &sv.signon;

	default:
		PR_RunError((char*)"WriteDest: bad destination");
		break;
	}

	return NULL;
}

void PF_WriteByte()
{
	MSG_WriteByte(WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteChar()
{
	MSG_WriteChar(WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteShort()
{
	MSG_WriteShort(WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteLong()
{
	MSG_WriteLong(WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteAngle()
{
	MSG_WriteAngle(WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteCoord()
{
	MSG_WriteCoord(WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteString()
{
	MSG_WriteString(WriteDest(), G_STRING(OFS_PARM1));
}


void PF_WriteEntity()
{
	MSG_WriteShort(WriteDest(), G_EDICTNUM(OFS_PARM1));
}

//=============================================================================

int SV_ModelIndex(char* name);

void PF_makestatic()
{
	edict_t* ent = G_EDICT(OFS_PARM0);

	MSG_WriteByte(&sv.signon, svc_spawnstatic);

	MSG_WriteByte(&sv.signon, SV_ModelIndex(pr_strings + ent->v.model));

	MSG_WriteByte(&sv.signon, ent->v.frame);
	MSG_WriteByte(&sv.signon, ent->v.colormap);
	MSG_WriteByte(&sv.signon, ent->v.skin);
	for (int i = 0; i < 3; i++)
	{
		MSG_WriteCoord(&sv.signon, ent->v.origin[i]);
		MSG_WriteAngle(&sv.signon, ent->v.angles[i]);
	}

	// throw the entity away now
	ED_Free(ent);
}

//=============================================================================

/*
==============
PF_setspawnparms
==============
*/
void PF_setspawnparms()
{
	edict_t* ent = G_EDICT(OFS_PARM0);
	int i = NUM_FOR_EDICT(ent);
	if (i < 1 || i > svs.maxclients)
		PR_RunError((char*)"Entity is not a client");

	// copy spawn parms out of the client_t
	client_t* client = svs.clients + (i - 1);

	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		(&pr_global_struct->parm1)[i] = client->spawn_parms[i];
}

/*
==============
PF_changelevel
==============
*/
void PF_changelevel()
{
	// make sure we don't issue two changelevels
	if (svs.changelevel_issued)
		return;
	svs.changelevel_issued = true;

	char* s = G_STRING(OFS_PARM0);
	Cbuf_AddText(va((char*)"changelevel %s\n", s));
}


void PF_Fixme()
{
	PR_RunError((char*)"unimplemented bulitin");
}


builtin_t pr_builtin[] =
{
	PF_Fixme,
	PF_makevectors, // void(entity e) makevectors  = #1;
	PF_setorigin, // void(entity e, vector o) setorigin = #2;
	PF_setmodel, // void(entity e, string m) setmodel = #3;
	PF_setsize, // void(entity e, vector min, vector max) setsize = #4;
	PF_Fixme, // void(entity e, vector min, vector max) setabssize = #5;
	PF_break, // void() break = #6;
	PF_random, // float() random = #7;
	PF_sound, // void(entity e, float chan, string samp) sound = #8;
	PF_normalize, // vector(vector v) normalize = #9;
	PF_error, // void(string e) error = #10;
	PF_objerror, // void(string e) objerror = #11;
	PF_vlen, // float(vector v) vlen = #12;
	PF_vectoyaw, // float(vector v) vectoyaw = #13;
	PF_Spawn, // entity() spawn = #14;
	PF_Remove, // void(entity e) remove = #15;
	PF_traceline, // float(vector v1, vector v2, float tryents) traceline = #16;
	PF_checkclient, // entity() clientlist = #17;
	PF_Find, // entity(entity start, .string fld, string match) find = #18;
	PF_precache_sound, // void(string s) precache_sound = #19;
	PF_precache_model, // void(string s) precache_model = #20;
	PF_stuffcmd, // void(entity client, string s)stuffcmd = #21;
	PF_findradius, // entity(vector org, float rad) findradius = #22;
	PF_bprint, // void(string s) bprint = #23;
	PF_sprint, // void(entity client, string s) sprint = #24;
	PF_dprint, // void(string s) dprint = #25;
	PF_ftos, // void(string s) ftos = #26;
	PF_vtos, // void(string s) vtos = #27;
	PF_coredump,
	PF_traceon,
	PF_traceoff,
	PF_eprint, // void(entity e) debug print an entire entity
	PF_walkmove, // float(float yaw, float dist) walkmove
	PF_Fixme, // float(float yaw, float dist) walkmove
	PF_droptofloor,
	PF_lightstyle,
	PF_rint,
	PF_floor,
	PF_ceil,
	PF_Fixme,
	PF_checkbottom,
	PF_pointcontents,
	PF_Fixme,
	PF_fabs,
	PF_aim,
	PF_cvar,
	PF_localcmd,
	PF_nextent,
	PF_particle,
	PF_changeyaw,
	PF_Fixme,
	PF_vectoangles,

	PF_WriteByte,
	PF_WriteChar,
	PF_WriteShort,
	PF_WriteLong,
	PF_WriteCoord,
	PF_WriteAngle,
	PF_WriteString,
	PF_WriteEntity,
	
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,

	SV_MoveToGoal,
	PF_precache_file,
	PF_makestatic,

	PF_changelevel,
	PF_Fixme,

	PF_cvar_set,
	PF_centerprint,

	PF_ambientsound,

	PF_precache_model,
	PF_precache_sound, // precache_sound2 is different only for qcc
	PF_precache_file,

	PF_setspawnparms
};

builtin_t* pr_builtins = pr_builtin;
int pr_numbuiltins = sizeof(pr_builtin) / sizeof(pr_builtin[0]);
