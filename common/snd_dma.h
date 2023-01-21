#pragma once

#include "common.h"
#include "cvar.h"
#include "mathlib.h"
#include "sound.h"


// !!! if this is changed, it much be changed in asm_i386.h too !!!
typedef struct
{
	sfx_t* sfx; // sfx number
	int leftvol; // 0-255 volume
	int rightvol; // 0-255 volume
	int end; // end time in global paintsamples
	int pos; // sample position in sfx
	int looping; // where to loop, -1 = no looping
	int entnum; // to allow overriding a specific sound
	int entchannel; //
	vec3_t origin; // origin of sound effect
	vec_t dist_mult; // distance multiplier (attenuation/clipK)
	int master_vol; // 0-255 master volume
} channel_t;

typedef struct
{
	qboolean gamealive;
	qboolean soundalive;
	qboolean splitbuffer;
	int channels;
	int samples; // mono samples in buffer
	int submission_chunk; // don't mix less than this #
	int samplepos; // in mono samples
	int samplebits;
	int speed;
	unsigned char* buffer;
} dma_t;

#define MAX_CHANNELS 128
extern channel_t channels[MAX_CHANNELS];
// 0 to MAX_DYNAMIC_CHANNELS-1 = normal entity sounds
// MAX_DYNAMIC_CHANNELS to MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS -1 = water, etc
// MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS to total_channels = static sounds

extern int total_channels;

//
// Fake dma is a synchronous faking of the DMA progress used for
// isolating performance in the renderer.  The fakedma_updates is
// number of times S_Update() is called per second.
//

extern qboolean fakedma;
extern int fakedma_updates;
extern int paintedtime;
extern vec3_t listener_origin;
extern vec3_t listener_forward;
extern vec3_t listener_right;
extern vec3_t listener_up;
extern volatile dma_t* shm;
extern volatile dma_t sn;
extern vec_t sound_nominal_clip_dist;

extern cvar_t loadas8bit;
extern cvar_t bgmvolume;
extern cvar_t volume;

extern qboolean snd_initialized;

extern int snd_blocked;


void S_Init(void);
void S_Startup(void);
void S_Shutdown(void);
void S_StartSound(int entnum, int entchannel, sfx_t* sfx, vec3_t origin, float fvol, float attenuation);
void S_StaticSound(sfx_t* sfx, vec3_t origin, float vol, float attenuation);
void S_StopSound(int entnum, int entchannel);
void S_StopAllSounds(qboolean clear);
void S_ClearBuffer(void);
void S_Update(vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);
void S_ExtraUpdate(void);

sfx_t* S_PrecacheSound(char* sample);
void S_TouchSound(char* sample);
void S_ClearPrecache(void);
void S_BeginPrecaching(void);
void S_EndPrecaching(void);

// picks a channel based on priorities, empty slots, number of channels
channel_t* SND_PickChannel(int entnum, int entchannel);

// spatializes a channel
void SND_Spatialize(channel_t* ch);

void S_LocalSound(char* s);

void S_AmbientOff(void);
void S_AmbientOn(void);