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


#include "host_cmd.h"

#include "cl_demo.h"
#include "cl_main.h"
#include "cmd.h"
#include "console.h"
#include "cvar.h"
#include "host.h"
#include "keys.h"
#include "protocol.h"
#include "pr_edict.h"
#include "pr_exec.h"
#include "screen.h"
#include "sv_main.h"
#include "sv_user.h"
#include "sys.h"
#include "world.h"

extern cvar_t pausable;

int current_skill;

void Mod_Print();

/*
==================
Host_Quit_f
==================
*/

extern void M_Menu_Quit_f();

void Host_Quit_f()
{
	if (key_dest != key_console && cls.state != ca_dedicated)
	{
		M_Menu_Quit_f();
		return;
	}
	CL_Disconnect();
	Host_ShutdownServer(false);

	Sys_Quit();
}


/*
==================
Host_Status_f
==================
*/
void Host_Status_f()
{
	client_t* client;
	int hours = 0;
	int j;
	void (*print)(char* fmt, ...);

	if (cmd_source == src_command)
	{
		if (!sv.active)
		{
			Cmd_ForwardToServer();
			return;
		}
		print = Con_Printf;
	}
	else
		print = SV_ClientPrintf;

	print((char*)"host:    %s\n", Cvar_VariableString((char*)"hostname"));
	print((char*)"version: %4.2f\n", VERSION);
	if (tcpipAvailable)
		print((char*)"tcp/ip:  %s\n", my_tcpip_address);
	if (ipxAvailable)
		print((char*)"ipx:     %s\n", my_ipx_address);
	print((char*)"map:     %s\n", sv.name);
	print((char*)"players: %i active (%i max)\n\n", net_activeconnections, svs.maxclients);
	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client->active)
			continue;
		int seconds = (int)(net_time - client->netconnection->connecttime);
		int minutes = seconds / 60;
		if (minutes)
		{
			seconds -= (minutes * 60);
			hours = minutes / 60;
			if (hours)
				minutes -= (hours * 60);
		}
		else
			hours = 0;
		print((char*)"#%-2u %-16.16s  %3i  %2i:%02i:%02i\n", j + 1, client->name, (int)client->edict->v.frags, hours, minutes,
		      seconds);
		print((char*)"   %s\n", client->netconnection->address);
	}
}


/*
==================
Host_God_f

Sets client to godmode
==================
*/
void Host_God_f()
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	sv_player->v.flags = (int)sv_player->v.flags ^ FL_GODMODE;
	if (!((int)sv_player->v.flags & FL_GODMODE))
		SV_ClientPrintf((char*)"godmode OFF\n");
	else
		SV_ClientPrintf((char*)"godmode ON\n");
}

void Host_Notarget_f()
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	sv_player->v.flags = (int)sv_player->v.flags ^ FL_NOTARGET;
	if (!((int)sv_player->v.flags & FL_NOTARGET))
		SV_ClientPrintf((char*)"notarget OFF\n");
	else
		SV_ClientPrintf((char*)"notarget ON\n");
}

bool noclip_anglehack;

void Host_Noclip_f()
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	if (sv_player->v.movetype != MOVETYPE_NOCLIP)
	{
		noclip_anglehack = true;
		sv_player->v.movetype = MOVETYPE_NOCLIP;
		SV_ClientPrintf((char*)"noclip ON\n");
	}
	else
	{
		noclip_anglehack = false;
		sv_player->v.movetype = MOVETYPE_WALK;
		SV_ClientPrintf((char*)"noclip OFF\n");
	}
}

/*
==================
Host_Fly_f

Sets client to flymode
==================
*/
void Host_Fly_f()
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	if (sv_player->v.movetype != MOVETYPE_FLY)
	{
		sv_player->v.movetype = MOVETYPE_FLY;
		SV_ClientPrintf((char*)"flymode ON\n");
	}
	else
	{
		sv_player->v.movetype = MOVETYPE_WALK;
		SV_ClientPrintf((char*)"flymode OFF\n");
	}
}


/*
==================
Host_Ping_f

==================
*/
void Host_Ping_f()
{
	int i;
	client_t* client;

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	SV_ClientPrintf((char*)"Client ping times:\n");
	for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
	{
		if (!client->active)
			continue;
		float total = 0;
		for (int j = 0; j < NUM_PING_TIMES; j++)
			total += client->ping_times[j];
		total /= NUM_PING_TIMES;
		SV_ClientPrintf((char*)"%4i %s\n", (int)(total * 1000), client->name);
	}
}

/*
===============================================================================

SERVER TRANSITIONS

===============================================================================
*/


/*
======================
Host_Map_f

handle a
map <servername>
command from the console.  Active clients are kicked off.
======================
*/
void Host_Map_f()
{
	int i;
	char name[MAX_QPATH];

	if (cmd_source != src_command)
		return;

	cls.demonum = -1; // stop demo loop in case this_ fails

	CL_Disconnect();
	Host_ShutdownServer(false);

	key_dest = key_game; // remove console or menu
	SCR_BeginLoadingPlaque();

	cls.mapstring[0] = 0;
	for (i = 0; i < Cmd_Argc(); i++)
	{
		strcat(cls.mapstring, Cmd_Argv(i));
		strcat(cls.mapstring, " ");
	}
	strcat(cls.mapstring, "\n");

	svs.serverflags = 0; // haven't completed an episode yet
	strcpy(name, Cmd_Argv(1));
	SV_SpawnServer(name);
	if (!sv.active)
		return;

	if (cls.state != ca_dedicated)
	{
		strcpy(cls.spawnparms, "");

		for (i = 2; i < Cmd_Argc(); i++)
		{
			strcat(cls.spawnparms, Cmd_Argv(i));
			strcat(cls.spawnparms, " ");
		}

		Cmd_ExecuteString((char*)"connect local", src_command);
	}
}

/*
==================
Host_Changelevel_f

Goes to a new_ map, taking all clients along
==================
*/
void Host_Changelevel_f()
{
	char level[MAX_QPATH];

	if (Cmd_Argc() != 2)
	{
		Con_Printf((char*)"changelevel <levelname> : continue game on a new_ level\n");
		return;
	}
	if (!sv.active || cls.demoplayback)
	{
		Con_Printf((char*)"Only the server may changelevel\n");
		return;
	}
	SV_SaveSpawnparms();
	strcpy(level, Cmd_Argv(1));
	SV_SpawnServer(level);
}

/*
==================
Host_Restart_f

Restarts the current server for a dead player
==================
*/
void Host_Restart_f()
{
	char mapname[MAX_QPATH];

	if (cls.demoplayback || !sv.active)
		return;

	if (cmd_source != src_command)
		return;
	strcpy(mapname, sv.name); // must copy out, because it gets cleared
	// in sv_spawnserver
	SV_SpawnServer(mapname);
}

/*
==================
Host_Reconnect_f

This command causes the client to wait for the signon messages again.
This is sent just before a server changes levels
==================
*/
void Host_Reconnect_f()
{
	SCR_BeginLoadingPlaque();
	cls.signon = 0; // need new_ connection messages
}

/*
=====================
Host_Connect_f

User command to connect to server
=====================
*/
void Host_Connect_f()
{
	char name[MAX_QPATH];

	cls.demonum = -1; // stop demo loop in case this_ fails
	if (cls.demoplayback)
	{
		CL_StopPlayback();
		CL_Disconnect();
	}
	strcpy(name, Cmd_Argv(1));
	CL_EstablishConnection(name);
	Host_Reconnect_f();
}


/*
===============================================================================

LOAD / SAVE GAME

===============================================================================
*/

#define SAVEGAME_VERSION 5

/*
===============
Host_SavegameComment

Writes a SAVEGAME_COMMENT_LENGTH character comment describing the current
===============
*/
void Host_SavegameComment(char* text)
{
	int i;
	char kills[20];

	for (i = 0; i < SAVEGAME_COMMENT_LENGTH; i++)
		text[i] = ' ';
	memcpy(text, cl.levelname, strlen(cl.levelname));
	sprintf(kills, (char*)"kills:%3i/%3i", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);
	memcpy(text + 22, kills, strlen(kills));
	// convert space to _ to make stdio happy
	for (i = 0; i < SAVEGAME_COMMENT_LENGTH; i++)
		if (text[i] == ' ')
			text[i] = '_';
	text[SAVEGAME_COMMENT_LENGTH] = '\0';
}


/*
===============
Host_Savegame_f
===============
*/
void Host_Savegame_f()
{
	char name[256];
	int i;
	char comment[SAVEGAME_COMMENT_LENGTH + 1];

	if (cmd_source != src_command)
		return;

	if (!sv.active)
	{
		Con_Printf((char*)"Not playing a local game.\n");
		return;
	}

	if (cl.intermission)
	{
		Con_Printf((char*)"Can't save in intermission.\n");
		return;
	}

	if (svs.maxclients != 1)
	{
		Con_Printf((char*)"Can't save multiplayer games.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf((char*)"save <savename> : save a game\n");
		return;
	}

	if (strstr(Cmd_Argv(1), ".."))
	{
		Con_Printf((char*)"Relative pathnames are not allowed.\n");
		return;
	}

	for (i = 0; i < svs.maxclients; i++)
	{
		if (svs.clients[i].active && (svs.clients[i].edict->v.health <= 0))
		{
			Con_Printf((char*)"Can't savegame with a dead player\n");
			return;
		}
	}

	sprintf(name, (char*)"%s/%s", com_gamedir, Cmd_Argv(1));
	COM_DefaultExtension(name, (char*)".sav");

	Con_Printf((char*)"Saving game to %s...\n", name);
	FILE* f = fopen(name, "w");
	if (!f)
	{
		Con_Printf((char*)"ERROR: couldn't open.\n");
		return;
	}

	fprintf(f, (char*)"%i\n", SAVEGAME_VERSION);
	Host_SavegameComment(comment);
	fprintf(f, (char*)"%s\n", comment);
	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		fprintf(f, (char*)"%f\n", svs.clients->spawn_parms[i]);
	fprintf(f, (char*)"%d\n", current_skill);
	fprintf(f, (char*)"%s\n", sv.name);
	fprintf(f, (char*)"%f\n", sv.time);

	// write the light styles

	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		if (sv.lightstyles[i])
			fprintf(f, (char*)"%s\n", sv.lightstyles[i]);
		else
			fprintf(f, (char*)"m\n");
	}


	ED_WriteGlobals(f);
	for (i = 0; i < sv.num_edicts; i++)
	{
		ED_Write(f, EDICT_NUM(i));
		fflush(f);
	}
	fclose(f);
	Con_Printf((char*)"done.\n");
}


/*
===============
Host_Loadgame_f
===============
*/
void Host_Loadgame_f()
{
	using namespace std::string_view_literals;
	char name[MAX_OSPATH];
	char mapname[MAX_QPATH];
	float time, tfloat;
	char str[32768];
	int i;
	int version;
	float spawn_parms[NUM_SPAWN_PARMS];

	if (cmd_source != src_command)
		return;

	if (Cmd_Argc() != 2)
	{
		Con_Printf((char*)"load <savename> : load a game\n");
		return;
	}

	cls.demonum = -1; // stop demo loop in case this_ fails

	sprintf(name, (char*)"%s/%s", com_gamedir, Cmd_Argv(1));
	COM_DefaultExtension(name, (char*)".sav");

	// we can't call SCR_BeginLoadingPlaque, because too much stack space has
	// been used.  The menu calls it before stuffing loadgame command
	// SCR_BeginLoadingPlaque ();

	Con_Printf((char*)"Loading game from %s...\n", name);
	FILE* f = fopen(name, "r");
	if (!f)
	{
		Con_Printf((char*)"ERROR: couldn't open.\n");
		return;
	}

	fscanf(f, (char*)"%i\n", &version);
	if (version != SAVEGAME_VERSION)
	{
		fclose(f);
		Con_Printf((char*)"Savegame is version %i, not %i\n", version, SAVEGAME_VERSION);
		return;
	}
	fscanf(f, (char*)"%s\n", str);
	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		fscanf(f, (char*)"%f\n", &spawn_parms[i]);
	// this_ silliness is so we can load 1.06 save files, which have float skill values
	fscanf(f, (char*)"%f\n", &tfloat);
	current_skill = (int)(tfloat + 0.1);
	Cvar_SetValue((char*)"skill", (float)current_skill);
	
	fscanf(f, (char*)"%s\n", mapname);
	fscanf(f, (char*)"%f\n", &time);

	CL_Disconnect_f();
	
	SV_SpawnServer(mapname);
	if (!sv.active)
	{
		Con_Printf((char*)"Couldn't load map\n");
		return;
	}
	sv.paused = true; // pause until all clients connect
	sv.loadgame = true;

	// load the light styles

	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		fscanf(f, (char*)"%s\n", str);
		sv.lightstyles[i] = static_cast<char*>(Hunk_Alloc(strlen(str) + 1));
		strcpy(sv.lightstyles[i], str);
	}

	// load the edicts out of the savegame file
	int entnum = -1; // -1 is the globals
	while (!feof(f))
	{
		for (i = 0; i < sizeof(str) - 1; i++)
		{
			int r = fgetc(f);
			if (r == EOF || !r)
				break;
			str[i] = r;
			if (r == '}')
			{
				i++;
				break;
			}
		}
		if (i == sizeof(str) - 1)
			Sys_Error("Loadgame buffer overflow"sv);
		str[i] = 0;
		char* start = str;
		start = COM_Parse(str);
		if (!com_token[0])
			break; // end of file
		if (strcmp(com_token, "{(char*)"))
			Sys_Error("First token isn't a brace"sv);

		if (entnum == -1)
		{
			// parse the global vars
			ED_ParseGlobals(start);
		}
		else
		{
			// parse an edict

			edict_t* ent = EDICT_NUM(entnum);
			memset(&ent->v, 0, progs->entityfields * 4);
			ent->free = false;
			ED_ParseEdict(start, ent);

			// link it into the bsp tree
			if (!ent->free)
				SV_LinkEdict(ent, false);
		}

		entnum++;
	}

	sv.num_edicts = entnum;
	sv.time = time;

	fclose(f);

	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		svs.clients->spawn_parms[i] = spawn_parms[i];

	if (cls.state != ca_dedicated)
	{
		CL_EstablishConnection((char*)"local");
		Host_Reconnect_f();
	}
}

//============================================================================

/*
======================
Host_Name_f
======================
*/
void Host_Name_f()
{
	char* new_Name;

	if (Cmd_Argc() == 1)
	{
		Con_Printf((char*)"\"name\" is \"%s\"\n", cl_name.string);
		return;
	}
	if (Cmd_Argc() == 2)
		new_Name = Cmd_Argv(1);
	else
		new_Name = Cmd_Args();
	new_Name[15] = 0;

	if (cmd_source == src_command)
	{
		if (Q_strcmp(cl_name.string, new_Name) == 0)
			return;
		Cvar_Set((char*)"_cl_name", new_Name);
		if (cls.state == ca_connected)
			Cmd_ForwardToServer();
		return;
	}

	if (host_client->name[0] && strcmp(host_client->name, "unconnected"))
		if (Q_strcmp(host_client->name, new_Name) != 0)
			Con_Printf((char*)"%s renamed to %s\n", host_client->name, new_Name);
	Q_strcpy(host_client->name, new_Name);
	host_client->edict->v.netname = host_client->name - pr_strings;

	// send notification to all clients

	MSG_WriteByte(&sv.reliable_datagram, svc_updatename);
	MSG_WriteByte(&sv.reliable_datagram, host_client - svs.clients);
	MSG_WriteString(&sv.reliable_datagram, host_client->name);
}


void Host_Version_f()
{
	Con_Printf((char*)"Version %4.2f\n", VERSION);
	Con_Printf((char*)"Exe: " __TIME__ " " __DATE__ "\n");
}


void Host_Say(bool teamonly)
{
	client_t* client;
	char text[64];
	bool fromServer = false;

	if (cmd_source == src_command)
	{
		if (cls.state == ca_dedicated)
		{
			fromServer = true;
			teamonly = false;
		}
		else
		{
			Cmd_ForwardToServer();
			return;
		}
	}

	if (Cmd_Argc() < 2)
		return;

	client_t* save = host_client;

	char* p = Cmd_Args();
	// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[Q_strlen(p) - 1] = 0;
	}

	// turn on color set 1
	if (!fromServer)
		sprintf(text, (char*)"%c%s: ", 1, save->name);
	else
		sprintf(text, (char*)"%c<%s> ", 1, hostname.string);

	int j = sizeof(text) - 2 - Q_strlen(text); // -2 for /n and null terminator
	if (Q_strlen(p) > j)
		p[j] = 0;

	strcat(text, p);
	strcat(text, "\n");

	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client || !client->active || !client->spawned)
			continue;
		if (teamplay.value && teamonly && client->edict->v.team != save->edict->v.team)
			continue;
		host_client = client;
		SV_ClientPrintf((char*)"%s", text);
	}
	host_client = save;

	Sys_Printf((char*)"%s", &text[1]);
}


void Host_Say_f()
{
	Host_Say(false);
}


void Host_Say_Team_f()
{
	Host_Say(true);
}


void Host_Tell_f()
{
	client_t* client;
	char text[64];

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (Cmd_Argc() < 3)
		return;

	Q_strcpy(text, host_client->name);
	Q_strcat(text, (char*)": ");

	char* p = Cmd_Args();

	// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[Q_strlen(p) - 1] = 0;
	}

	// check length & truncate if necessary
	int j = sizeof(text) - 2 - Q_strlen(text); // -2 for /n and null terminator
	if (Q_strlen(p) > j)
		p[j] = 0;

	strcat(text, p);
	strcat(text, "\n");

	client_t* save = host_client;
	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client->active || !client->spawned)
			continue;
		if (Q_strcasecmp(client->name, Cmd_Argv(1)))
			continue;
		host_client = client;
		SV_ClientPrintf((char*)"%s", text);
		break;
	}
	host_client = save;
}


/*
==================
Host_Color_f
==================
*/
void Host_Color_f()
{
	int top, bottom;

	if (Cmd_Argc() == 1)
	{
		Con_Printf((char*)"\"color\" is \"%i %i\"\n", ((int)cl_color.value) >> 4, ((int)cl_color.value) & 0x0f);
		Con_Printf((char*)"color <0-13> [0-13]\n");
		return;
	}

	if (Cmd_Argc() == 2)
		top = bottom = atoi(Cmd_Argv(1));
	else
	{
		top = atoi(Cmd_Argv(1));
		bottom = atoi(Cmd_Argv(2));
	}

	top &= 15;
	if (top > 13)
		top = 13;
	bottom &= 15;
	if (bottom > 13)
		bottom = 13;

	int playercolor = top * 16 + bottom;

	if (cmd_source == src_command)
	{
		Cvar_SetValue((char*)"_cl_color", playercolor);
		if (cls.state == ca_connected)
			Cmd_ForwardToServer();
		return;
	}

	host_client->colors = playercolor;
	host_client->edict->v.team = bottom + 1;

	// send notification to all clients
	MSG_WriteByte(&sv.reliable_datagram, svc_updatecolors);
	MSG_WriteByte(&sv.reliable_datagram, host_client - svs.clients);
	MSG_WriteByte(&sv.reliable_datagram, host_client->colors);
}

/*
==================
Host_Kill_f
==================
*/
void Host_Kill_f()
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (sv_player->v.health <= 0)
	{
		SV_ClientPrintf((char*)"Can't suicide -- allready dead!\n");
		return;
	}

	pr_global_struct->time = sv.time;
	pr_global_struct->self = EDICT_TO_PROG(sv_player);
	PR_ExecuteProgram(pr_global_struct->ClientKill);
}


/*
==================
Host_Pause_f
==================
*/
void Host_Pause_f()
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}
	if (!pausable.value)
		SV_ClientPrintf((char*)"Pause not allowed.\n");
	else
	{
		sv.paused ^= 1;

		if (sv.paused)
		{
			SV_BroadcastPrintf((char*)"%s paused the game\n", pr_strings + sv_player->v.netname);
		}
		else
		{
			SV_BroadcastPrintf((char*)"%s unpaused the game\n", pr_strings + sv_player->v.netname);
		}

		// send notification to all clients
		MSG_WriteByte(&sv.reliable_datagram, svc_setpause);
		MSG_WriteByte(&sv.reliable_datagram, sv.paused);
	}
}

//===========================================================================


/*
==================
Host_PreSpawn_f
==================
*/
void Host_PreSpawn_f()
{
	if (cmd_source == src_command)
	{
		Con_Printf((char*)"prespawn is not valid from the console\n");
		return;
	}

	if (host_client->spawned)
	{
		Con_Printf((char*)"prespawn not valid -- allready spawned\n");
		return;
	}

	SZ_Write(&host_client->message, sv.signon.data, sv.signon.cursize);
	MSG_WriteByte(&host_client->message, svc_signonnum);
	MSG_WriteByte(&host_client->message, 2);
	host_client->sendsignon = true;
}

/*
==================
Host_Spawn_f
==================
*/
void Host_Spawn_f()
{
	int i;
	client_t* client;
	edict_t* ent;

	if (cmd_source == src_command)
	{
		Con_Printf((char*)"spawn is not valid from the console\n");
		return;
	}

	if (host_client->spawned)
	{
		Con_Printf((char*)"Spawn not valid -- allready spawned\n");
		return;
	}

	// run the entrance script
	if (sv.loadgame)
	{
		// loaded games are fully inited allready
		// if this_ is the last client to be connected, unpause
		sv.paused = false;
	}
	else
	{
		// set up the edict
		ent = host_client->edict;

		memset(&ent->v, 0, progs->entityfields * 4);
		ent->v.colormap = NUM_FOR_EDICT(ent);
		ent->v.team = (host_client->colors & 15) + 1;
		ent->v.netname = host_client->name - pr_strings;

		// copy spawn parms out of the client_t

		for (i = 0; i < NUM_SPAWN_PARMS; i++)
			(&pr_global_struct->parm1)[i] = host_client->spawn_parms[i];

		// call the spawn function

		pr_global_struct->time = sv.time;
		pr_global_struct->self = EDICT_TO_PROG(sv_player);
		PR_ExecuteProgram(pr_global_struct->ClientConnect);

		if ((Sys_FloatTime() - host_client->netconnection->connecttime) <= sv.time)
			Sys_Printf((char*)"%s entered the game\n", host_client->name);

		PR_ExecuteProgram(pr_global_struct->PutClientInServer);
	}


	// send all current names, colors, and frag counts
	SZ_Clear(&host_client->message);

	// send time of update
	MSG_WriteByte(&host_client->message, svc_time);
	MSG_WriteFloat(&host_client->message, sv.time);

	for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
	{
		MSG_WriteByte(&host_client->message, svc_updatename);
		MSG_WriteByte(&host_client->message, i);
		MSG_WriteString(&host_client->message, client->name);
		MSG_WriteByte(&host_client->message, svc_updatefrags);
		MSG_WriteByte(&host_client->message, i);
		MSG_WriteShort(&host_client->message, client->old_frags);
		MSG_WriteByte(&host_client->message, svc_updatecolors);
		MSG_WriteByte(&host_client->message, i);
		MSG_WriteByte(&host_client->message, client->colors);
	}

	// send all current light styles
	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		MSG_WriteByte(&host_client->message, svc_lightstyle);
		MSG_WriteByte(&host_client->message, i);
		MSG_WriteString(&host_client->message, sv.lightstyles[i]);
	}

	//
	// send some stats
	//
	MSG_WriteByte(&host_client->message, svc_updatestat);
	MSG_WriteByte(&host_client->message, STAT_TOTALSECRETS);
	MSG_WriteLong(&host_client->message, pr_global_struct->total_secrets);

	MSG_WriteByte(&host_client->message, svc_updatestat);
	MSG_WriteByte(&host_client->message, STAT_TOTALMONSTERS);
	MSG_WriteLong(&host_client->message, pr_global_struct->total_monsters);

	MSG_WriteByte(&host_client->message, svc_updatestat);
	MSG_WriteByte(&host_client->message, STAT_SECRETS);
	MSG_WriteLong(&host_client->message, pr_global_struct->found_secrets);

	MSG_WriteByte(&host_client->message, svc_updatestat);
	MSG_WriteByte(&host_client->message, STAT_MONSTERS);
	MSG_WriteLong(&host_client->message, pr_global_struct->killed_monsters);


	//
	// send a fixangle
	// Never send a roll angle, because savegames can catch the server
	// in a state where it is expecting the client to correct the angle
	// and it won't happen if the game was just loaded, so you wind up
	// with a permanent head tilt
	ent = EDICT_NUM(1 + (host_client - svs.clients));
	MSG_WriteByte(&host_client->message, svc_setangle);
	for (i = 0; i < 2; i++)
		MSG_WriteAngle(&host_client->message, ent->v.angles[i]);
	MSG_WriteAngle(&host_client->message, 0);

	SV_WriteClientdataToMessage(sv_player, &host_client->message);

	MSG_WriteByte(&host_client->message, svc_signonnum);
	MSG_WriteByte(&host_client->message, 3);
	host_client->sendsignon = true;
}

/*
==================
Host_Begin_f
==================
*/
void Host_Begin_f()
{
	if (cmd_source == src_command)
	{
		Con_Printf((char*)"begin is not valid from the console\n");
		return;
	}

	host_client->spawned = true;
}

//===========================================================================


/*
==================
Host_Kick_f

Kicks a user off of the server
==================
*/
void Host_Kick_f()
{
	char* who;
	char* message = NULL;
	int i;
	bool byNumber = false;

	if (cmd_source == src_command)
	{
		if (!sv.active)
		{
			Cmd_ForwardToServer();
			return;
		}
	}
	else if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	client_t* save = host_client;

	if (Cmd_Argc() > 2 && Q_strcmp(Cmd_Argv(1), (char*)"#") == 0)
	{
		i = Q_atof(Cmd_Argv(2)) - 1;
		if (i < 0 || i >= svs.maxclients)
			return;
		if (!svs.clients[i].active)
			return;
		host_client = &svs.clients[i];
		byNumber = true;
	}
	else
	{
		for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
		{
			if (!host_client->active)
				continue;
			if (Q_strcasecmp(host_client->name, Cmd_Argv(1)) == 0)
				break;
		}
	}

	if (i < svs.maxclients)
	{
		if (cmd_source == src_command)
			if (cls.state == ca_dedicated)
				who = (char*)"Console";
			else
				who = cl_name.string;
		else
			who = save->name;

		// can't kick yourself!
		if (host_client == save)
			return;

		if (Cmd_Argc() > 2)
		{
			message = COM_Parse(Cmd_Args());
			if (byNumber)
			{
				message++; // skip the #
				while (*message == ' ') // skip white space
					message++;
				message += Q_strlen(Cmd_Argv(2)); // skip the number
			}
			while (*message && *message == ' ')
				message++;
		}
		if (message)
			SV_ClientPrintf((char*)"Kicked by %s: %s\n", who, message);
		else
			SV_ClientPrintf((char*)"Kicked by %s\n", who);
		SV_DropClient(false);
	}

	host_client = save;
}

/*
===============================================================================

DEBUGGING TOOLS

===============================================================================
*/

/*
==================
Host_Give_f
==================
*/
void Host_Give_f()
{
	int w;
	eval_t* val;

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	char* t = Cmd_Argv(1);
	int v = atoi(Cmd_Argv(2));

	switch (t[0])
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		// MED 01/04/97 added hipnotic give stuff
		if (hipnotic)
		{
			if (t[0] == '6')
			{
				if (t[1] == 'a')
					sv_player->v.items = (int)sv_player->v.items | HIT_PROXIMITY_GUN;
				else
					sv_player->v.items = (int)sv_player->v.items | IT_GRENADE_LAUNCHER;
			}
			else if (t[0] == '9')
				sv_player->v.items = (int)sv_player->v.items | HIT_LASER_CANNON;
			else if (t[0] == '0')
				sv_player->v.items = (int)sv_player->v.items | HIT_MJOLNIR;
			else if (t[0] >= '2')
				sv_player->v.items = (int)sv_player->v.items | (IT_SHOTGUN << (t[0] - '2'));
		}
		else
		{
			if (t[0] >= '2')
				sv_player->v.items = (int)sv_player->v.items | (IT_SHOTGUN << (t[0] - '2'));
		}
		break;

	case 's':
		if (rogue)
		{
			val = GetEdictFieldValue(sv_player, (char*)"ammo_shells1");
			if (val)
				val->_float = v;
		}

		sv_player->v.ammo_shells = v;
		break;
	case 'n':
		if (rogue)
		{
			val = GetEdictFieldValue(sv_player, (char*)"ammo_nails1");
			if (val)
			{
				val->_float = v;
				if (sv_player->v.weapon <= IT_LIGHTNING)
					sv_player->v.ammo_nails = v;
			}
		}
		else
		{
			sv_player->v.ammo_nails = v;
		}
		break;
	case 'l':
		if (rogue)
		{
			val = GetEdictFieldValue(sv_player, (char*)"ammo_lava_nails");
			if (val)
			{
				val->_float = v;
				if (sv_player->v.weapon > IT_LIGHTNING)
					sv_player->v.ammo_nails = v;
			}
		}
		break;
	case 'r':
		if (rogue)
		{
			val = GetEdictFieldValue(sv_player, (char*)"ammo_rockets1");
			if (val)
			{
				val->_float = v;
				if (sv_player->v.weapon <= IT_LIGHTNING)
					sv_player->v.ammo_rockets = v;
			}
		}
		else
		{
			sv_player->v.ammo_rockets = v;
		}
		break;
	case 'm':
		if (rogue)
		{
			val = GetEdictFieldValue(sv_player, (char*)"ammo_multi_rockets");
			if (val)
			{
				val->_float = v;
				if (sv_player->v.weapon > IT_LIGHTNING)
					sv_player->v.ammo_rockets = v;
			}
		}
		break;
	case 'h':
		sv_player->v.health = v;
		break;
	case 'c':
		if (rogue)
		{
			val = GetEdictFieldValue(sv_player, (char*)"ammo_cells1");
			if (val)
			{
				val->_float = v;
				if (sv_player->v.weapon <= IT_LIGHTNING)
					sv_player->v.ammo_cells = v;
			}
		}
		else
		{
			sv_player->v.ammo_cells = v;
		}
		break;
	case 'p':
		if (rogue)
		{
			val = GetEdictFieldValue(sv_player, (char*)"ammo_plasma");
			if (val)
			{
				val->_float = v;
				if (sv_player->v.weapon > IT_LIGHTNING)
					sv_player->v.ammo_cells = v;
			}
		}
		break;
	}
}

edict_t* FindViewthing()
{
	for (int i = 0; i < sv.num_edicts; i++)
	{
		edict_t* e = EDICT_NUM(i);
		if (!strcmp(pr_strings + e->v.classname, "viewthing"))
			return e;
	}
	Con_Printf((char*)"No viewthing on map\n");
	return NULL;
}

/*
==================
Host_Viewmodel_f
==================
*/
void Host_Viewmodel_f()
{
	edict_t* e = FindViewthing();
	if (!e)
		return;

	model_t* m = Mod_ForName(Cmd_Argv(1), false);
	if (!m)
	{
		Con_Printf((char*)"Can't load %s\n", Cmd_Argv(1));
		return;
	}

	e->v.frame = 0;
	cl.model_precache[(int)e->v.modelindex] = m;
}

/*
==================
Host_Viewframe_f
==================
*/
void Host_Viewframe_f()
{
	edict_t* e = FindViewthing();
	if (!e)
		return;
	model_t* m = cl.model_precache[(int)e->v.modelindex];

	int f = atoi(Cmd_Argv(1));
	if (f >= m->numframes)
		f = m->numframes - 1;

	e->v.frame = f;
}


void PrintFrameName(model_t* m, int frame)
{
	aliashdr_t* hdr = (aliashdr_t*)Mod_Extradata(m);
	if (!hdr)
		return;
	maliasframedesc_t* pframedesc = &hdr->frames[frame];

	Con_Printf((char*)"frame %i: %s\n", frame, pframedesc->name);
}

/*
==================
Host_Viewnext_f
==================
*/
void Host_Viewnext_f()
{
	edict_t* e = FindViewthing();
	if (!e)
		return;
	model_t* m = cl.model_precache[(int)e->v.modelindex];

	e->v.frame = e->v.frame + 1;
	if (e->v.frame >= m->numframes)
		e->v.frame = m->numframes - 1;

	PrintFrameName(m, e->v.frame);
}

/*
==================
Host_Viewprev_f
==================
*/
void Host_Viewprev_f()
{
	edict_t* e = FindViewthing();
	if (!e)
		return;

	model_t* m = cl.model_precache[(int)e->v.modelindex];

	e->v.frame = e->v.frame - 1;
	if (e->v.frame < 0)
		e->v.frame = 0;

	PrintFrameName(m, e->v.frame);
}

/*
===============================================================================

DEMO LOOP CONTROL

===============================================================================
*/


/*
==================
Host_Startdemos_f
==================
*/
void Host_Startdemos_f()
{
	if (cls.state == ca_dedicated)
	{
		if (!sv.active)
			Cbuf_AddText((char*)"map start\n");
		return;
	}

	int c = Cmd_Argc() - 1;
	if (c > MAX_DEMOS)
	{
		Con_Printf((char*)"Max %i demos in demoloop\n", MAX_DEMOS);
		c = MAX_DEMOS;
	}
	Con_Printf((char*)"%i demo(s) in loop\n", c);

	for (int i = 1; i < c + 1; i++)
		strncpy(cls.demos[i - 1], Cmd_Argv(i), sizeof(cls.demos[0]) - 1);

	if (!sv.active && cls.demonum != -1 && !cls.demoplayback)
	{
		cls.demonum = 0;
		CL_NextDemo();
	}
	else
		cls.demonum = -1;
}


/*
==================
Host_Demos_f

Return to looping demos
==================
*/
void Host_Demos_f()
{
	if (cls.state == ca_dedicated)
		return;
	if (cls.demonum == -1)
		cls.demonum = 1;
	CL_Disconnect_f();
	CL_NextDemo();
}

/*
==================
Host_Stopdemo_f

Return to looping demos
==================
*/
void Host_Stopdemo_f()
{
	if (cls.state == ca_dedicated)
		return;
	if (!cls.demoplayback)
		return;
	CL_StopPlayback();
	CL_Disconnect();
}

//=============================================================================

/*
==================
Host_InitCommands
==================
*/
void Host_InitCommands()
{
	Cmd_AddCommand((char*)"status", Host_Status_f);
	Cmd_AddCommand((char*)"quit", Host_Quit_f);
	Cmd_AddCommand((char*)"god", Host_God_f);
	Cmd_AddCommand((char*)"notarget", Host_Notarget_f);
	Cmd_AddCommand((char*)"fly", Host_Fly_f);
	Cmd_AddCommand((char*)"map", Host_Map_f);
	Cmd_AddCommand((char*)"restart", Host_Restart_f);
	Cmd_AddCommand((char*)"changelevel", Host_Changelevel_f);
	Cmd_AddCommand((char*)"connect", Host_Connect_f);
	Cmd_AddCommand((char*)"reconnect", Host_Reconnect_f);
	Cmd_AddCommand((char*)"name", Host_Name_f);
	Cmd_AddCommand((char*)"noclip", Host_Noclip_f);
	Cmd_AddCommand((char*)"version", Host_Version_f);
	Cmd_AddCommand((char*)"say", Host_Say_f);
	Cmd_AddCommand((char*)"say_team", Host_Say_Team_f);
	Cmd_AddCommand((char*)"tell", Host_Tell_f);
	Cmd_AddCommand((char*)"color", Host_Color_f);
	Cmd_AddCommand((char*)"kill", Host_Kill_f);
	Cmd_AddCommand((char*)"pause", Host_Pause_f);
	Cmd_AddCommand((char*)"spawn", Host_Spawn_f);
	Cmd_AddCommand((char*)"begin", Host_Begin_f);
	Cmd_AddCommand((char*)"prespawn", Host_PreSpawn_f);
	Cmd_AddCommand((char*)"kick", Host_Kick_f);
	Cmd_AddCommand((char*)"ping", Host_Ping_f);
	Cmd_AddCommand((char*)"load", Host_Loadgame_f);
	Cmd_AddCommand((char*)"save", Host_Savegame_f);
	Cmd_AddCommand((char*)"give", Host_Give_f);

	Cmd_AddCommand((char*)"startdemos", Host_Startdemos_f);
	Cmd_AddCommand((char*)"demos", Host_Demos_f);
	Cmd_AddCommand((char*)"stopdemo", Host_Stopdemo_f);

	Cmd_AddCommand((char*)"viewmodel", Host_Viewmodel_f);
	Cmd_AddCommand((char*)"viewframe", Host_Viewframe_f);
	Cmd_AddCommand((char*)"viewnext", Host_Viewnext_f);
	Cmd_AddCommand((char*)"viewprev", Host_Viewprev_f);

	Cmd_AddCommand((char*)"mcache", Mod_Print);
}
