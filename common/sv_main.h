#pragma once

#include "typedef_byte.h"
#include "common.h"
#include "mathlib.h"
#include "progs.h"

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "common.h"
#include "bspfile.h"
#include "vid.h"
#include "sys.h"
#include "zone.h"
#include "mathlib.h"
#include "entity_state.h";
#include "wad.h"
#include "draw.h"
#include "cvar.h"
#include "screen.h"
#include "net_main.h"
#include "net_win.h"
#include "net.h"
#include "protocol.h"
#include "cmd.h"
#include "sbar.h"
#include "sound.h"
#include "render.h"
#include "client.h"
#include "progs.h"
#include "cl_demo.h"
#include "cl_input.h"
#include "cl_main.h"
#include "cl_parse.h"
#include "cl_tent.h"
#include "server.h"
#include "sv_main.h"
#include "sv_user.h"
#include "sv_phys.h"
#include "sv_move.h"
#ifdef GLQUAKE
#include "gl_model.h"
#else
#include "model.h"
#include "d_iface.h"
#endif
#include "input.h"
#include "world.h"
#include "keys.h"
#include "console.h"
#include "view.h"
#include "menu.h"
#include "crc.h"
#include "cdaudio.h"
#ifdef GLQUAKE
#include "glquake.h"
#endif
#include "host.h"
#include "chase.h"
#include "quakedef.h"

typedef enum { ss_loading, ss_active } server_state_t;

typedef struct
{
	qboolean active; // false if only a net client

	qboolean paused;
	qboolean loadgame; // handle connections specially

	double time;

	int lastcheck; // used by PF_checkclient
	double lastchecktime;

	char name[64]; // map name

	char modelname[64]; // maps/<name>.bsp, for model_precache[0]
	struct model_s* worldmodel;
	char* model_precache[MAX_MODELS]; // NULL terminated
	struct model_s* models[MAX_MODELS];
	char* sound_precache[MAX_SOUNDS]; // NULL terminated
	char* lightstyles[MAX_LIGHTSTYLES];
	int num_edicts;
	int max_edicts;
	edict_t* edicts; // can NOT be array indexed, because
	// edict_t is variable sized, but can
	// be used to reference the world ent
	server_state_t state; // some actions are only valid during load

	sizebuf_t datagram;
	byte datagram_buf[MAX_DATAGRAM];

	sizebuf_t reliable_datagram; // copied to all clients at end of frame
	byte reliable_datagram_buf[MAX_DATAGRAM];

	sizebuf_t signon;
	byte signon_buf[8192];
} server_t;

typedef struct
{
	int maxclients;
	int maxclientslimit;
	struct client_s* clients; // [maxclients]
	int serverflags; // episode completion information
	qboolean changelevel_issued; // cleared when at SV_SpawnServer
} server_static_t;

extern server_static_t svs; // persistant server info
extern server_t sv; // local server

void SV_Init(void);
void SV_StartParticle(vec3_t org, vec3_t dir, int color, int count);
void SV_StartSound(edict_t* entity, int channel, char* sample, int volume, float attenuation);
void SV_SendClientMessages(void);
void SV_ClearDatagram(void);
int SV_ModelIndex(char* name);
void SV_WriteClientdataToMessage(edict_t* ent, sizebuf_t* msg);
void SV_CheckForNewClients(void);
void SV_SaveSpawnparms();
void SV_SpawnServer(char* server);