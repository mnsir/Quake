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
// snd_mem.c: sound caching

#include "snd_mem.h"

#include <cstring>
#include <format>

#include "console.h"
#include "snd_dma.h"
#include "sys.h"


int cache_full_cycle;

byte* S_Alloc(int size);

/*
================
ResampleSfx
================
*/
void ResampleSfx(sfx_t* sfx, int inrate, int inwidth, byte* data)
{
	int i;
	int sample;

	sfxcache_t* sc = static_cast<sfxcache_t*>(Cache_Check(&sfx->cache));
	if (!sc)
		return;

	float stepscale = (float)inrate / shm->speed; // this_ is usually 0.5, 1, or 2

	int outcount = sc->length / stepscale;
	sc->length = outcount;
	if (sc->loopstart != -1)
		sc->loopstart = sc->loopstart / stepscale;

	sc->speed = shm->speed;
	if (loadas8bit.value)
		sc->width = 1;
	else
		sc->width = inwidth;
	sc->stereo = 0;

	// resample / decimate to the current source rate

	if (stepscale == 1 && inwidth == 1 && sc->width == 1)
	{
		// fast special case
		for (i = 0; i < outcount; i++)
			((signed char*)sc->data)[i]
				= data[i] - 128;
	}
	else
	{
		// general case
		int samplefrac = 0;
		int fracstep = stepscale * 256;
		for (i = 0; i < outcount; i++)
		{
			int srcsample = samplefrac >> 8;
			samplefrac += fracstep;
			if (inwidth == 2)
				sample = LittleShort(((short*)data)[srcsample]);
			else
				sample = data[srcsample] - 128 << 8;
			if (sc->width == 2)
				((short*)sc->data)[i] = sample;
			else
				((signed char*)sc->data)[i] = sample >> 8;
		}
	}
}

//=============================================================================

/*
==============
S_LoadSound
==============
*/
sfxcache_t* S_LoadSound(sfx_t* s)
{
	char namebuffer[256];
	byte stackbuf[1 * 1024]; // avoid dirtying the cache heap

	// see if still in memory
	sfxcache_t* sc = static_cast<sfxcache_t*>(Cache_Check(&s->cache));
	if (sc)
		return sc;

	//Con_Printf ((char*)"S_LoadSound: %x\n", (int)stackbuf);
	// load it in
	Q_strcpy(namebuffer, (char*)"sound/");
	Q_strcat(namebuffer, s->name);

	// Con_Printf ((char*)"loading %s\n",namebuffer);

	byte* data = COM_LoadStackFile(namebuffer, stackbuf, sizeof(stackbuf));

	if (!data)
	{
		Con_Printf((char*)"Couldn't load %s\n", namebuffer);
		return NULL;
	}

	wavinfo_t info = GetWavinfo(s->name, data, com_filesize);
	if (info.channels != 1)
	{
		Con_Printf((char*)"%s is a stereo sample\n", s->name);
		return NULL;
	}

	float stepscale = (float)info.rate / shm->speed;
	int len = info.samples / stepscale;

	len = len * info.width * info.channels;

	sc = static_cast<sfxcache_t*>(Cache_Alloc(&s->cache, len + sizeof(sfxcache_t), s->name));
	if (!sc)
		return NULL;

	sc->length = info.samples;
	sc->loopstart = info.loopstart;
	sc->speed = info.rate;
	sc->width = info.width;
	sc->stereo = info.channels;

	ResampleSfx(s, sc->speed, sc->width, data + info.dataofs);

	return sc;
}


/*
===============================================================================

WAV loading

===============================================================================
*/


byte* data_p;
byte* iff_end;
byte* last_chunk;
byte* iff_data;
int iff_chunk_len;


short GetLittleShort(void)
{
	short val = *data_p;
	val = val + (*(data_p + 1) << 8);
	data_p += 2;
	return val;
}

int GetLittleLong(void)
{
	int val = *data_p;
	val = val + (*(data_p + 1) << 8);
	val = val + (*(data_p + 2) << 16);
	val = val + (*(data_p + 3) << 24);
	data_p += 4;
	return val;
}

void FindNextChunk(char* name)
{
	while (true)
	{
		data_p = last_chunk;

		if (data_p >= iff_end)
		{
			// didn't find the chunk
			data_p = NULL;
			return;
		}

		data_p += 4;
		iff_chunk_len = GetLittleLong();
		if (iff_chunk_len < 0)
		{
			data_p = NULL;
			return;
		}
		// if (iff_chunk_len > 1024*1024)
		// Sys_Error ((char*)"FindNextChunk: %i length is past the 1 meg sanity limit", iff_chunk_len);
		data_p -= 8;
		last_chunk = data_p + 8 + ((iff_chunk_len + 1) & ~1);
		if (!Q_strncmp((char*)data_p, name, 4))
			return;
	}
}

void FindChunk(char* name)
{
	last_chunk = iff_data;
	FindNextChunk(name);
}


void DumpChunks(void)
{
	char str[5];

	str[4] = 0;
	data_p = iff_data;
	do
	{
		memcpy(str, data_p, 4);
		data_p += 4;
		iff_chunk_len = GetLittleLong();
		Con_Printf((char*)"0x%x : %s (%d)\n", (int)(data_p - 4), str, iff_chunk_len);
		data_p += (iff_chunk_len + 1) & ~1;
	}
	while (data_p < iff_end);
}

/*
============
GetWavinfo
============
*/
wavinfo_t GetWavinfo(char* name, byte* wav, int wavlength)
{
	using namespace std::string_view_literals;
	wavinfo_t info;

	memset(&info, 0, sizeof(info));

	if (!wav)
		return info;

	iff_data = wav;
	iff_end = wav + wavlength;

	// find "RIFF" chunk
	FindChunk((char*)"RIFF");
	if (!(data_p && !Q_strncmp((char*)data_p + 8, (char*)"WAVE", 4)))
	{
		Con_Printf((char*)"Missing RIFF/WAVE chunks\n");
		return info;
	}

	// get "fmt " chunk
	iff_data = data_p + 12;
	// DumpChunks ();

	FindChunk((char*)"fmt ");
	if (!data_p)
	{
		Con_Printf((char*)"Missing fmt chunk\n");
		return info;
	}
	data_p += 8;
	int format = GetLittleShort();
	if (format != 1)
	{
		Con_Printf((char*)"Microsoft PCM format only\n");
		return info;
	}

	info.channels = GetLittleShort();
	info.rate = GetLittleLong();
	data_p += 4 + 2;
	info.width = GetLittleShort() / 8;

	// get cue chunk
	FindChunk((char*)"cue ");
	if (data_p)
	{
		data_p += 32;
		info.loopstart = GetLittleLong();
		// Con_Printf((char*)"loopstart=%d\n", sfx->loopstart);

		// if the next chunk is a LIST chunk, look for a cue length marker
		FindNextChunk((char*)"LIST");
		if (data_p)
		{
			if (!strncmp((char*)data_p + 28, "mark", 4))
			{
				// this_ is not a proper parse, but it works with cooledit...
				data_p += 24;
				int i = GetLittleLong(); // samples in loop
				info.samples = info.loopstart + i;
				// Con_Printf((char*)"looped length: %i\n", i);
			}
		}
	}
	else
		info.loopstart = -1;

	// find data chunk
	FindChunk((char*)"data");
	if (!data_p)
	{
		Con_Printf((char*)"Missing data chunk\n");
		return info;
	}

	data_p += 4;
	int samples = GetLittleLong() / info.width;

	if (info.samples)
	{
		if (samples < info.samples)
			Sys_Error(std::format("Sound {} has a bad loop length"sv, name));
	}
	else
		info.samples = samples;

	info.dataofs = data_p - wav;

	return info;
}
