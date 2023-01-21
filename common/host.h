#pragma once

#include <setjmp.h>

#include "cvar.h"
#include "server.h"


//=============================================================================
// host.h

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct
{
	char* basedir;
	char* cachedir; // for development over ISDN lines
	int argc;
	char** argv;
	void* membase;
	int memsize;
} quakeparms_t;


//=============================================================================

//
// host
//
extern quakeparms_t host_parms;

extern cvar_t sys_ticrate;
extern cvar_t developer;

extern qboolean host_initialized; // true if into command execution
extern double host_frametime;
extern byte* host_basepal;
extern byte* host_colormap;
extern int host_framecount; // incremented every frame, never reset
extern double realtime; // not bounded in any way, changed at
// start of every frame, never reset

void Host_ClearMemory(void);
void Host_ServerFrame(void);
void Host_Init(quakeparms_t* parms);
void Host_Shutdown(void);
void Host_Error(char* error, ...);
void Host_EndGame(char* message, ...);
void Host_Frame(float time);
void Host_ClientCommands(char* fmt, ...);
void Host_ShutdownServer(qboolean crash);

extern int minimum_memory;

extern cvar_t teamplay;
extern cvar_t skill;
extern cvar_t deathmatch;
extern cvar_t coop;
extern cvar_t fraglimit;
extern cvar_t timelimit;

extern client_t* host_client;
extern jmp_buf host_abortserver;
extern double host_time;

void SV_DropClient(qboolean crash);
void SV_ClientPrintf(char* fmt, ...);
void SV_BroadcastPrintf(char* fmt, ...);
