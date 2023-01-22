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
// host.c -- coordinates spawning and killing of local servers

#include "host.h"

#include <cstdarg>

#include "cdaudio.h"
#include "chase.h"
#include "client.h"
#include "cl_main.h"
#include "cmd.h"
#include "console.h"
#include "draw.h"
#include "host_cmd.h"
#include "input.h"
#include "keys.h"
#include "menu.h"
#include "protocol.h"
#include "pr_edict.h"
#include "pr_exec.h"
#include "sbar.h"
#include "screen.h"
#include "snd_dma.h"
#include "sv_main.h"
#include "sv_phys.h"
#include "sv_user.h"
#include "sys.h"
#include "view.h"
#include "wad.h"


/*

A server can allways be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

quakeparms_t host_parms;

qboolean host_initialized; // true if into command execution

double host_frametime;
double host_time;
double realtime; // without any filtering or bounding
double oldrealtime; // last frame run
int host_framecount;

int host_hunklevel;

int minimum_memory;

client_t* host_client; // current client

jmp_buf host_abortserver;

byte* host_basepal;
byte* host_colormap;

cvar_t host_framerate = {(char*)"host_framerate", (char*)"0"}; // set for slow motion
cvar_t host_speeds = {(char*)"host_speeds", (char*)"0"}; // set for running times

cvar_t sys_ticrate = {(char*)"sys_ticrate", (char*)"0.05"};
cvar_t serverprofile = {(char*)"serverprofile", (char*)"0"};

cvar_t fraglimit = {(char*)"fraglimit", (char*)"0", false, true};
cvar_t timelimit = {(char*)"timelimit", (char*)"0", false, true};
cvar_t teamplay = {(char*)"teamplay", (char*)"0", false, true};

cvar_t samelevel = {(char*)"samelevel", (char*)"0"};
cvar_t noexit = {(char*)"noexit", (char*)"0", false, true};

cvar_t developer = {(char*)"developer", (char*)"0"};

cvar_t skill = {(char*)"skill", (char*)"1"}; // 0 - 3
cvar_t deathmatch = {(char*)"deathmatch", (char*)"0"}; // 0, 1, or 2
cvar_t coop = {(char*)"coop", (char*)"0"}; // 0 or 1

cvar_t pausable = {(char*)"pausable", (char*)"1"};

cvar_t temp1 = {(char*)"temp1", (char*)"0"};


/*
================
Host_EndGame
================
*/
void Host_EndGame(char* message, ...)
{
	va_list argptr;
	char string[1024];

	va_start(argptr, message);
	vsprintf(string, message, argptr);
	va_end(argptr);
	Con_DPrintf((char*)"Host_EndGame: %s\n", string);

	if (sv.active)
		Host_ShutdownServer(false);

	if (cls.state == ca_dedicated)
		Sys_Error((char*)"Host_EndGame: %s\n", string); // dedicated servers exit

	if (cls.demonum != -1)
		CL_NextDemo();
	else
		CL_Disconnect();

	longjmp(host_abortserver, 1);
}

/*
================
Host_Error

This shuts down both the client and server
================
*/
void Host_Error(char* error, ...)
{
	va_list argptr;
	char string[1024];
	static qboolean inerror = false;

	if (inerror)
		Sys_Error((char*)"Host_Error: recursively entered");
	inerror = true;

	SCR_EndLoadingPlaque(); // reenable screen updates

	va_start(argptr, error);
	vsprintf(string, error, argptr);
	va_end(argptr);
	Con_Printf((char*)"Host_Error: %s\n", string);

	if (sv.active)
		Host_ShutdownServer(false);

	if (cls.state == ca_dedicated)
		Sys_Error((char*)"Host_Error: %s\n", string); // dedicated servers exit

	CL_Disconnect();
	cls.demonum = -1;

	inerror = false;

	longjmp(host_abortserver, 1);
}

/*
================
Host_FindMaxClients
================
*/
void Host_FindMaxClients(void)
{
	svs.maxclients = 1;

	int i = COM_CheckParm((char*)"-dedicated");
	if (i)
	{
		cls.state = ca_dedicated;
		if (i != (com_argc - 1))
		{
			svs.maxclients = Q_atoi(com_argv[i + 1]);
		}
		else
			svs.maxclients = 8;
	}
	else
		cls.state = ca_disconnected;

	i = COM_CheckParm((char*)"-listen");
	if (i)
	{
		if (cls.state == ca_dedicated)
			Sys_Error((char*)"Only one of -dedicated or -listen can be specified");
		if (i != (com_argc - 1))
			svs.maxclients = Q_atoi(com_argv[i + 1]);
		else
			svs.maxclients = 8;
	}
	if (svs.maxclients < 1)
		svs.maxclients = 8;
	else if (svs.maxclients > MAX_SCOREBOARD)
		svs.maxclients = MAX_SCOREBOARD;

	svs.maxclientslimit = svs.maxclients;
	if (svs.maxclientslimit < 4)
		svs.maxclientslimit = 4;
	svs.clients = static_cast<client_s*>(Hunk_AllocName(svs.maxclientslimit * sizeof(client_t), (char*)"clients"));

	if (svs.maxclients > 1)
		Cvar_SetValue((char*)"deathmatch", 1.0);
	else
		Cvar_SetValue((char*)"deathmatch", 0.0);
}


/*
=======================
Host_InitLocal
======================
*/
void Host_InitLocal(void)
{
	Host_InitCommands();

	Cvar_RegisterVariable(&host_framerate);
	Cvar_RegisterVariable(&host_speeds);

	Cvar_RegisterVariable(&sys_ticrate);
	Cvar_RegisterVariable(&serverprofile);

	Cvar_RegisterVariable(&fraglimit);
	Cvar_RegisterVariable(&timelimit);
	Cvar_RegisterVariable(&teamplay);
	Cvar_RegisterVariable(&samelevel);
	Cvar_RegisterVariable(&noexit);
	Cvar_RegisterVariable(&skill);
	Cvar_RegisterVariable(&developer);
	Cvar_RegisterVariable(&deathmatch);
	Cvar_RegisterVariable(&coop);

	Cvar_RegisterVariable(&pausable);

	Cvar_RegisterVariable(&temp1);

	Host_FindMaxClients();

	host_time = 1.0; // so a think at time 0 won't get called
}


/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfiguration(void)
{
	// dedicated servers initialize the host but don't parse and set the
	// config.cfg cvars
	if (host_initialized & !isDedicated)
	{
		FILE* f = fopen(va((char*)"%s/config.cfg", com_gamedir), (char*)"w");
		if (!f)
		{
			Con_Printf((char*)"Couldn't write config.cfg.\n");
			return;
		}

		Key_WriteBindings(f);
		Cvar_WriteVariables(f);

		fclose(f);
	}
}


/*
=================
SV_ClientPrintf

Sends text across to be displayed
FIXME: make this_ just a stuffed echo?
=================
*/
void SV_ClientPrintf(char* fmt, ...)
{
	va_list argptr;
	char string[1024];

	va_start(argptr, fmt);
	vsprintf(string, fmt, argptr);
	va_end(argptr);

	MSG_WriteByte(&host_client->message, svc_print);
	MSG_WriteString(&host_client->message, string);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf(char* fmt, ...)
{
	va_list argptr;
	char string[1024];

	va_start(argptr, fmt);
	vsprintf(string, fmt, argptr);
	va_end(argptr);

	for (int i = 0; i < svs.maxclients; i++)
		if (svs.clients[i].active && svs.clients[i].spawned)
		{
			MSG_WriteByte(&svs.clients[i].message, svc_print);
			MSG_WriteString(&svs.clients[i].message, string);
		}
}

/*
=================
Host_ClientCommands

Send text over to the client to be executed
=================
*/
void Host_ClientCommands(char* fmt, ...)
{
	va_list argptr;
	char string[1024];

	va_start(argptr, fmt);
	vsprintf(string, fmt, argptr);
	va_end(argptr);

	MSG_WriteByte(&host_client->message, svc_stufftext);
	MSG_WriteString(&host_client->message, string);
}

/*
=====================
SV_DropClient

Called when the player is getting totally kicked off the host
if (crash = true), don't bother sending signofs
=====================
*/
void SV_DropClient(qboolean crash)
{
	int i;
	client_t* client;

	if (!crash)
	{
		// send any final messages (don't check for errors)
		if (NET_CanSendMessage(host_client->netconnection))
		{
			MSG_WriteByte(&host_client->message, svc_disconnect);
			NET_SendMessage(host_client->netconnection, &host_client->message);
		}

		if (host_client->edict && host_client->spawned)
		{
			// call the prog function for removing a client
			// this_ will set the body to a dead frame, among other things
			int saveSelf = pr_global_struct->self;
			pr_global_struct->self = EDICT_TO_PROG(host_client->edict);
			PR_ExecuteProgram(pr_global_struct->ClientDisconnect);
			pr_global_struct->self = saveSelf;
		}

		Sys_Printf((char*)"Client %s removed\n", host_client->name);
	}

	// break the net connection
	NET_Close(host_client->netconnection);
	host_client->netconnection = NULL;

	// free the client (the body stays around)
	host_client->active = false;
	host_client->name[0] = 0;
	host_client->old_frags = -999999;
	net_activeconnections--;

	// send notification to all clients
	for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
	{
		if (!client->active)
			continue;
		MSG_WriteByte(&client->message, svc_updatename);
		MSG_WriteByte(&client->message, host_client - svs.clients);
		MSG_WriteString(&client->message, (char*)"");
		MSG_WriteByte(&client->message, svc_updatefrags);
		MSG_WriteByte(&client->message, host_client - svs.clients);
		MSG_WriteShort(&client->message, 0);
		MSG_WriteByte(&client->message, svc_updatecolors);
		MSG_WriteByte(&client->message, host_client - svs.clients);
		MSG_WriteByte(&client->message, 0);
	}
}

/*
==================
Host_ShutdownServer

This only happens at the end of a game, not between levels
==================
*/
void Host_ShutdownServer(qboolean crash)
{
	int i;
	int count;
	sizebuf_t buf;
	char message[4];

	if (!sv.active)
		return;

	sv.active = false;

	// stop all client sounds immediately
	if (cls.state == ca_connected)
		CL_Disconnect();

	// flush any pending messages - like the score!!!
	double start = Sys_FloatTime();
	do
	{
		count = 0;
		for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
		{
			if (host_client->active && host_client->message.cursize)
			{
				if (NET_CanSendMessage(host_client->netconnection))
				{
					NET_SendMessage(host_client->netconnection, &host_client->message);
					SZ_Clear(&host_client->message);
				}
				else
				{
					NET_GetMessage(host_client->netconnection);
					count++;
				}
			}
		}
		if ((Sys_FloatTime() - start) > 3.0)
			break;
	}
	while (count);

	// make sure all the clients know we're disconnecting
	buf.data = (byte*)message;
	buf.maxsize = 4;
	buf.cursize = 0;
	MSG_WriteByte(&buf, svc_disconnect);
	count = NET_SendToAll(&buf, 5);
	if (count)
		Con_Printf((char*)"Host_ShutdownServer: NET_SendToAll failed for %u clients\n", count);

	for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
		if (host_client->active)
			SV_DropClient(crash);

	//
	// clear structures
	//
	memset(&sv, 0, sizeof(sv));
	memset(svs.clients, 0, svs.maxclientslimit * sizeof(client_t));
}


/*
================
Host_ClearMemory

This clears all the memory used by both the client and server, but does
not reinitialize anything.
================
*/
void Host_ClearMemory(void)
{
	Con_DPrintf((char*)"Clearing memory\n");
	D_FlushCaches();
	Mod_ClearAll();
	if (host_hunklevel)
		Hunk_FreeToLowMark(host_hunklevel);

	cls.signon = 0;
	memset(&sv, 0, sizeof(sv));
	memset(&cl, 0, sizeof(cl));
}


//============================================================================


/*
===================
Host_FilterTime

Returns false if the time is too short to run a frame
===================
*/
qboolean Host_FilterTime(float time)
{
	realtime += time;

	if (!cls.timedemo && realtime - oldrealtime < 1.0 / 72.0)
		return false; // framerate is too high

	host_frametime = realtime - oldrealtime;
	oldrealtime = realtime;

	if (host_framerate.value > 0)
		host_frametime = host_framerate.value;
	else
	{
		// don't allow really long or short frames
		if (host_frametime > 0.1)
			host_frametime = 0.1;
		if (host_frametime < 0.001)
			host_frametime = 0.001;
	}

	return true;
}


/*
===================
Host_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void Host_GetConsoleCommands(void)
{
	while (1)
	{
		char* cmd = Sys_ConsoleInput();
		if (!cmd)
			break;
		Cbuf_AddText(cmd);
	}
}


/*
==================
Host_ServerFrame

==================
*/
void Host_ServerFrame(void)
{
	// run the world state 
	pr_global_struct->frametime = host_frametime;

	// set the time and clear the general datagram
	SV_ClearDatagram();

	// check for new_ clients
	SV_CheckForNewClients();

	// read client messages
	SV_RunClients();

	// move things around and think
	// always pause in single player if in console or menus
	if (!sv.paused && (svs.maxclients > 1 || key_dest == key_game))
		SV_Physics();

	// send all messages to the clients
	SV_SendClientMessages();
}


/*
==================
Host_Frame

Runs all active servers
==================
*/
void _Host_Frame(float time)
{
	static double time1 = 0;
	static double time2 = 0;
	static double time3 = 0;

	if (setjmp(host_abortserver))
		return; // something bad happened, or the server disconnected

	// keep the random time dependent
	rand();

	// decide the simulation time
	if (!Host_FilterTime(time))
		return; // don't run too fast, or packets will flood out

	// get new_ key events
	Sys_SendKeyEvents();

	// allow mice or other external controllers to add commands
	IN_Commands();

	// process console commands
	Cbuf_Execute();

	NET_Poll();

	// if running the server locally, make intentions now
	if (sv.active)
		CL_SendCmd();

	//-------------------
	//
	// server operations
	//
	//-------------------

	// check for commands typed to the host
	Host_GetConsoleCommands();

	if (sv.active)
		Host_ServerFrame();

	//-------------------
	//
	// client operations
	//
	//-------------------

	// if running the server remotely, send intentions now after
	// the incoming messages have been read
	if (!sv.active)
		CL_SendCmd();

	host_time += host_frametime;

	// fetch results from server
	if (cls.state == ca_connected)
	{
		CL_ReadFromServer();
	}

	// update video
	if (host_speeds.value)
		time1 = Sys_FloatTime();

	SCR_UpdateScreen();

	if (host_speeds.value)
		time2 = Sys_FloatTime();

	// update audio
	if (cls.signon == SIGNONS)
	{
		S_Update(r_origin, vpn, vright, vup);
		CL_DecayLights();
	}
	else
		S_Update(vec3_origin, vec3_origin, vec3_origin, vec3_origin);

	CDAudio_Update();

	if (host_speeds.value)
	{
		int pass1 = (time1 - time3) * 1000;
		time3 = Sys_FloatTime();
		int pass2 = (time2 - time1) * 1000;
		int pass3 = (time3 - time2) * 1000;
		Con_Printf((char*)"%3i tot %3i server %3i gfx %3i snd\n",
		           pass1 + pass2 + pass3, pass1, pass2, pass3);
	}

	host_framecount++;
}

void Host_Frame(float time)
{
	static double timetotal;
	static int timecount;

	if (!serverprofile.value)
	{
		_Host_Frame(time);
		return;
	}

	double time1 = Sys_FloatTime();
	_Host_Frame(time);
	double time2 = Sys_FloatTime();

	timetotal += time2 - time1;
	timecount++;

	if (timecount < 1000)
		return;

	int m = timetotal * 1000 / timecount;
	timecount = 0;
	timetotal = 0;
	int c = 0;
	for (int i = 0; i < svs.maxclients; i++)
	{
		if (svs.clients[i].active)
			c++;
	}

	Con_Printf((char*)"serverprofile: %2i clients %2i msec\n", c, m);
}

//============================================================================


extern int vcrFile;
#define VCR_SIGNATURE 0x56435231
// "VCR1"

void Host_InitVCR(quakeparms_t* parms)
{
	int i, len, n;

	if (COM_CheckParm((char*)"-playback"))
	{
		if (com_argc != 2)
			Sys_Error((char*)"No other parameters allowed with -playback\n");

		Sys_FileOpenRead((char*)"quake.vcr", &vcrFile);
		if (vcrFile == -1)
			Sys_Error((char*)"playback file not found\n");

		Sys_FileRead(vcrFile, &i, sizeof(int));
		if (i != VCR_SIGNATURE)
			Sys_Error((char*)"Invalid signature in vcr file\n");

		Sys_FileRead(vcrFile, &com_argc, sizeof(int));
		com_argv = static_cast<char**>(malloc(com_argc * sizeof(char*)));
		com_argv[0] = parms->argv[0];
		for (i = 0; i < com_argc; i++)
		{
			Sys_FileRead(vcrFile, &len, sizeof(int));
			char* p = static_cast<char*>(malloc(len));
			Sys_FileRead(vcrFile, p, len);
			com_argv[i + 1] = p;
		}
		com_argc++; /* add one for arg[0] */
		parms->argc = com_argc;
		parms->argv = com_argv;
	}

	if ((n = COM_CheckParm((char*)"-record")) != 0)
	{
		vcrFile = Sys_FileOpenWrite((char*)"quake.vcr");

		i = VCR_SIGNATURE;
		Sys_FileWrite(vcrFile, &i, sizeof(int));
		i = com_argc - 1;
		Sys_FileWrite(vcrFile, &i, sizeof(int));
		for (i = 1; i < com_argc; i++)
		{
			if (i == n)
			{
				len = 10;
				Sys_FileWrite(vcrFile, &len, sizeof(int));
				Sys_FileWrite(vcrFile, (char*)"-playback", len);
				continue;
			}
			len = Q_strlen(com_argv[i]) + 1;
			Sys_FileWrite(vcrFile, &len, sizeof(int));
			Sys_FileWrite(vcrFile, com_argv[i], len);
		}
	}
}

/*
====================
Host_Init
====================
*/
void Host_Init(quakeparms_t* parms)
{
	if (standard_quake)
		minimum_memory = MINIMUM_MEMORY;
	else
		minimum_memory = MINIMUM_MEMORY_LEVELPAK;

	if (COM_CheckParm((char*)"-minmemory"))
		parms->memsize = minimum_memory;

	host_parms = *parms;

	if (parms->memsize < minimum_memory)
		Sys_Error((char*)"Only %4.1f megs of memory available, can't execute game", parms->memsize / (float)0x100000);

	com_argc = parms->argc;
	com_argv = parms->argv;

	Memory_Init(parms->membase, parms->memsize);
	Cbuf_Init();
	Cmd_Init();
	V_Init();
	Chase_Init();
	Host_InitVCR(parms);
	COM_Init(parms->basedir);
	Host_InitLocal();
	W_LoadWadFile((char*)"gfx.wad");
	Key_Init();
	Con_Init();
	M_Init();
	PR_Init();
	Mod_Init();
	NET_Init();
	SV_Init();

	Con_Printf((char*)"Exe: " __TIME__ " " __DATE__ "\n");
	Con_Printf((char*)"%4.1f megabyte heap\n", parms->memsize / (1024 * 1024.0));

	R_InitTextures(); // needed even for dedicated servers

	if (cls.state != ca_dedicated)
	{
		host_basepal = COM_LoadHunkFile((char*)"gfx/palette.lmp");
		if (!host_basepal)
			Sys_Error((char*)"Couldn't load gfx/palette.lmp");
		host_colormap = COM_LoadHunkFile((char*)"gfx/colormap.lmp");
		if (!host_colormap)
			Sys_Error((char*)"Couldn't load gfx/colormap.lmp");
		
		VID_Init(host_basepal);

		Draw_Init();
		SCR_Init();
		R_Init();

#ifdef GLQUAKE
		// FIXME: doesn't use the new_ one-window approach yet
		S_Init();
#endif
		
		CDAudio_Init();
		Sbar_Init();
		CL_Init();
		IN_Init();
	}

	Cbuf_InsertText((char*)"exec quake.rc\n");

	Hunk_AllocName(0, (char*)"-HOST_HUNKLEVEL-");
	host_hunklevel = Hunk_LowMark();

	host_initialized = true;

	Sys_Printf((char*)"========Quake Initialized=========\n");
}


/*
===============
Host_Shutdown

FIXME: this_ is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void Host_Shutdown(void)
{
	static qboolean isdown = false;

	if (isdown)
	{
		printf((char*)"recursive shutdown\n");
		return;
	}
	isdown = true;

	// keep Con_Printf from trying to update the screen
	scr_disabled_for_loading = true;

	Host_WriteConfiguration();

	CDAudio_Shutdown();
	NET_Shutdown();
	S_Shutdown();
	IN_Shutdown();

	if (cls.state != ca_dedicated)
	{
		VID_Shutdown();
	}
}
