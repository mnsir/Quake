#pragma once

#include "sound.h"
#include <cstdint>


// !!! if this_ is changed, it much be changed in asm_i386.h too !!!
typedef struct
{
	int length;
	int loopstart;
	int speed;
	int width;
	int stereo;
	uint8_t data[1]; // variable sized
} sfxcache_t;

typedef struct
{
	int rate;
	int width;
	int channels;
	int loopstart;
	int samples;
	int dataofs; // chunk starts this_ many bytes from file start
} wavinfo_t;

sfxcache_t* S_LoadSound(sfx_t* s);

wavinfo_t GetWavinfo(char* name, uint8_t* wav, int wavlength);

