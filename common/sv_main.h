#pragma once

#include "common.h"

#include "common_model.h"

#include "progs.h"
#include "server.h"


typedef enum { ss_loading, ss_active } server_state_t;

typedef struct
{
	bool active; // false if only a net client

	bool paused;
	bool loadgame; // handle connections specially

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
	bool changelevel_issued; // cleared when at SV_SpawnServer
} server_static_t;

extern server_static_t svs; // persistant server info
extern server_t sv; // local server

void SV_Init();
void SV_StartParticle(const vec3_t& org, const vec3_t& dir, int color, int count);
void SV_StartSound(edict_t* entity, int channel, char* sample, int volume, float attenuation);
void SV_SendClientMessages();
void SV_ClearDatagram();
int SV_ModelIndex(char* name);
void SV_WriteClientdataToMessage(edict_t* ent, sizebuf_t* msg);
void SV_CheckForNewClients();
void SV_SaveSpawnparms();
void SV_SpawnServer(char* server);