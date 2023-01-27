#pragma once

#include <cstdio>
#include "cvar.h"
#include "cl_input.h"
#include "quakedef.h"
#include "cl_main.h"
#include "vid.h"
#include "mathlib.h"
#include "net_main.h"

#include "common_model.h"

#include "sound.h"
#include "render.h"

typedef struct
{
	int destcolor[3];
	int percent; // 0-256
} cshift_t;


typedef struct
{
	char name[MAX_SCOREBOARDNAME];
	float entertime;
	int frags;
	int colors; // two 4 bit fields
	uint8_t translations[VID_GRADES * 256];
} scoreboard_t;

#define NUM_CSHIFTS 4

//
// the client_state_t structure is wiped completely at every
// server signon
//
typedef struct
{
	int movemessages; // since connecting to this_ server
	// throw out the first couple, so the player
	// doesn't accidentally do something the 
	// first frame
	usercmd_t cmd; // last command sent to the server

	// information for local display
	int stats[MAX_CL_STATS]; // health, etc
	int items; // inventory bit flags
	float item_gettime[32]; // cl.time of aquiring item, for blinking
	float faceanimtime; // use anim frame if cl.time < this_

	cshift_t cshifts[NUM_CSHIFTS]; // color shifts for damage, powerups
	cshift_t prev_cshifts[NUM_CSHIFTS]; // and content types

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  The server sets punchangle when
	// the view is temporarliy offset, and an angle reset commands at the start
	// of each level and after teleporting.
	vec3_t mviewangles[2]; // during demo playback viewangles is lerped
	// between these
	vec3_t viewangles;

	vec3_t mvelocity[2]; // update by server, used for lean+bob
	// (0 is new_est)
	vec3_t velocity; // lerped between mvelocity[0] and [1]

	vec3_t punchangle; // temporary offset

	// pitch drifting vars
	float idealpitch;
	float pitchvel;
	bool nodrift;
	float driftmove;
	double laststop;

	float viewheight;
	float crouch; // local amount for smoothing stepups

	bool paused; // send over by server
	bool onground;
	bool inwater;

	int intermission; // don't change view angle, full screen, etc
	int completed_time; // latched at intermission start

	double mtime[2]; // the timestamp of last two messages 
	double time; // clients view of time, should be between
	// servertime and oldservertime to generate
	// a lerp point for other data
	double oldtime; // previous cl.time, time-oldtime is used
	// to decay light values and smooth step ups


	float last_received_message; // (realtime) for net trouble icon

	//
	// information that is static for the entire time connected to a server
	//
	struct model_s* model_precache[MAX_MODELS];
	struct sfx_s* sound_precache[MAX_SOUNDS];

	char levelname[40]; // for display on solo scoreboard
	int viewentity; // cl_entitites[cl.viewentity] = player
	int maxclients;
	int gametype;

	// refresh related state
	struct model_s* worldmodel; // cl_entitites[0].model
	struct efrag_s* free_efrags;
	int num_entities; // held in cl_entities array
	int num_statics; // held in cl_staticentities array
	entity_t viewent; // the gun model

	int cdtrack, looptrack; // cd audio

	// frag scoreboard
	scoreboard_t* scores; // [cl.maxclients]
} client_state_t;



#define MAX_MAPSTRING 2048
#define MAX_DEMOS 8
#define MAX_DEMONAME 16

typedef enum
{
	ca_dedicated,
	// a dedicated server with no ability to start a client
	ca_disconnected,
	// full screen console with no connection
	ca_connected // valid netcon, talking to a server
} cactive_t;

//
// the client_static_t structure is persistant through an arbitrary number
// of server connections
//
typedef struct
{
	cactive_t state;

	// personalization data sent to server 
	char mapstring[MAX_QPATH];
	char spawnparms[MAX_MAPSTRING]; // to restart a level

	// demo loop control
	int demonum; // -1 = don't play demos
	char demos[MAX_DEMOS][MAX_DEMONAME]; // when not playing

	// demo recording info must be here, because record is started before
	// entering a map (and clearing client_state_t)
	bool demorecording;
	bool demoplayback;
	bool timedemo;
	int forcetrack; // -1 = use normal cd track
	FILE* demofile;
	int td_lastframe; // to meter out one message a frame
	int td_startframe; // host_framecount at start
	float td_starttime; // realtime at second frame of timedemo


	// connection information
	int signon; // 0 to SIGNONS
	struct qsocket_s* netcon;
	sizebuf_t message; // writing buffer to send to server
} client_static_t;

typedef struct
{
	int length;
	char map[MAX_STYLESTRING];
} lightstyle_t;

typedef struct
{
	vec3_t origin;
	float radius;
	float die; // stop lighting after this_ time
	float decay; // drop this_ each second
	float minlight; // don't add when contributing less
	int key;
} dlight_t;


extern cvar_t cl_name;
extern cvar_t cl_color;
extern cvar_t cl_shownet;
extern cvar_t cl_nolerp;
extern cvar_t lookspring;
extern cvar_t lookstrafe;
extern cvar_t sensitivity;
extern cvar_t m_pitch;
extern cvar_t m_yaw;
extern cvar_t m_forward;
extern cvar_t m_side;

extern client_state_t cl;
extern client_static_t cls;

// FIXME, allocate dynamically
#define MAX_EFRAGS 640
extern efrag_t cl_efrags[MAX_EFRAGS];
extern entity_t cl_entities[MAX_EDICTS];
#define MAX_STATIC_ENTITIES 128 // torches, etc
extern entity_t cl_static_entities[MAX_STATIC_ENTITIES];
extern lightstyle_t cl_lightstyle[MAX_LIGHTSTYLES];
#define MAX_DLIGHTS 32
extern dlight_t cl_dlights[MAX_DLIGHTS];

extern int cl_numvisedicts;
#define MAX_VISEDICTS 256
extern entity_t* cl_visedicts[MAX_VISEDICTS];

dlight_t* CL_AllocDlight(int key);
void CL_DecayLights();
void CL_Init();
void CL_EstablishConnection(char* host);
void CL_Disconnect();
void CL_Disconnect_f();
void CL_NextDemo();
void CL_SendCmd();
void CL_ClearState();
int CL_ReadFromServer();
void CL_SignonReply();

