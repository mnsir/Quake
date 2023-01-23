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
// d_surf.c: rasterization driver surface heap manager

#include <format>

#include "common.h"
#include "console.h"
#include "d_iface.h"
#include "d_local.h"
#include "model.h"
#include "r_local.h"
#include "sys.h"


float surfscale;
bool r_cache_thrash; // set if surface cache is thrashing

int sc_size;
surfcache_t *sc_rover, *sc_base;

#define GUARDSIZE       4


int D_SurfaceCacheForRes(int width, int height)
{
	int size;

	if (COM_CheckParm((char*)"-surfcachesize"))
	{
		size = Q_atoi(com_argv[COM_CheckParm((char*)"-surfcachesize") + 1]) * 1024;
		return size;
	}

	size = SURFCACHE_SIZE_AT_320X200;

	int pix = width * height;
	if (pix > 64000)
		size += (pix - 64000) * 3;


	return size;
}

void D_CheckCacheGuard(void)
{
	using namespace std::string_view_literals;
	byte* s = (byte*)sc_base + sc_size;
	for (int i = 0; i < GUARDSIZE; i++)
		if (s[i] != (byte)i)
			Sys_Error("D_CheckCacheGuard: failed"sv);
}

void D_ClearCacheGuard(void)
{
	byte* s = (byte*)sc_base + sc_size;
	for (int i = 0; i < GUARDSIZE; i++)
		s[i] = (byte)i;
}


/*
================
D_InitCaches

================
*/
void D_InitCaches(void* buffer, int size)
{
	if (!msg_suppress_1)
		Con_Printf((char*)"%ik surface cache\n", size / 1024);

	sc_size = size - GUARDSIZE;
	sc_base = (surfcache_t*)buffer;
	sc_rover = sc_base;

	sc_base->next = NULL;
	sc_base->owner = NULL;
	sc_base->size = sc_size;

	D_ClearCacheGuard();
}


/*
==================
D_FlushCaches
==================
*/
void D_FlushCaches(void)
{
	if (!sc_base)
		return;

	for (surfcache_t* c = sc_base; c; c = c->next)
	{
		if (c->owner)
			*c->owner = NULL;
	}

	sc_rover = sc_base;
	sc_base->next = NULL;
	sc_base->owner = NULL;
	sc_base->size = sc_size;
}

/*
=================
D_SCAlloc
=================
*/
surfcache_t* D_SCAlloc(int width, int size)
{
	using namespace std::string_view_literals;
	if ((width < 0) || (width > 256))
		Sys_Error(std::format("D_SCAlloc: bad cache width {}\n"sv, width));

	if ((size <= 0) || (size > 0x10000))
		Sys_Error(std::format("D_SCAlloc: bad cache size {}\n"sv, size));

	size = (int)&((surfcache_t*)0)->data[size];
	size = (size + 3) & ~3;
	if (size > sc_size)
		Sys_Error(std::format("D_SCAlloc: {} > cache size"sv, size));

	// if there is not size bytes after the rover, reset to the start
	bool wrapped_this__time = false;

	if (!sc_rover || (byte*)sc_rover - (byte*)sc_base > sc_size - size)
	{
		if (sc_rover)
		{
			wrapped_this__time = true;
		}
		sc_rover = sc_base;
	}

	// colect and free surfcache_t blocks until the rover block is large enough
	surfcache_t* new_ = sc_rover;
	if (sc_rover->owner)
		*sc_rover->owner = NULL;

	while (new_->size < size)
	{
		// free another
		sc_rover = sc_rover->next;
		if (!sc_rover)
			Sys_Error("D_SCAlloc: hit the end of memory"sv);
		if (sc_rover->owner)
			*sc_rover->owner = NULL;

		new_->size += sc_rover->size;
		new_->next = sc_rover->next;
	}

	// create a fragment out of any leftovers
	if (new_->size - size > 256)
	{
		sc_rover = (surfcache_t*)((byte*)new_ + size);
		sc_rover->size = new_->size - size;
		sc_rover->next = new_->next;
		sc_rover->width = 0;
		sc_rover->owner = NULL;
		new_->next = sc_rover;
		new_->size = size;
	}
	else
		sc_rover = new_->next;

	new_->width = width;
	// DEBUG
	if (width > 0)
		new_->height = (size - sizeof(*new_) + sizeof(new_->data)) / width;

	new_->owner = NULL; // should be set properly after return

	if (d_roverwrapped)
	{
		if (wrapped_this__time || (sc_rover >= d_initial_rover))
			r_cache_thrash = true;
	}
	else if (wrapped_this__time)
	{
		d_roverwrapped = true;
	}

	D_CheckCacheGuard(); // DEBUG
	return new_;
}


/*
=================
D_SCDump
=================
*/
void D_SCDump(void)
{
	for (surfcache_t* test = sc_base; test; test = test->next)
	{
		if (test == sc_rover)
			Sys_Printf((char*)"ROVER:\n");
		printf((char*)"%p : %i bytes     %i width\n", test, test->size, test->width);
	}
}

//=============================================================================

// if the num is not a power of 2, assume it will not repeat

int MaskForNum(int num)
{
	if (num == 128)
		return 127;
	if (num == 64)
		return 63;
	if (num == 32)
		return 31;
	if (num == 16)
		return 15;
	return 255;
}

int D_log2(int num)
{
	int c = 0;

	while (num >>= 1)
		c++;
	return c;
}

//=============================================================================

/*
================
D_CacheSurface
================
*/
surfcache_t* D_CacheSurface(msurface_t* surface, int miplevel)
{
	//
	// if the surface is animating or flashing, flush the cache
	//
	r_drawsurf.texture = R_TextureAnimation(surface->texinfo->texture);
	r_drawsurf.lightadj[0] = d_lightstylevalue[surface->styles[0]];
	r_drawsurf.lightadj[1] = d_lightstylevalue[surface->styles[1]];
	r_drawsurf.lightadj[2] = d_lightstylevalue[surface->styles[2]];
	r_drawsurf.lightadj[3] = d_lightstylevalue[surface->styles[3]];

	//
	// see if the cache holds apropriate data
	//
	surfcache_t* cache = surface->cachespots[miplevel];

	if (cache && !cache->dlight && surface->dlightframe != r_framecount
		&& cache->texture == r_drawsurf.texture
		&& cache->lightadj[0] == r_drawsurf.lightadj[0]
		&& cache->lightadj[1] == r_drawsurf.lightadj[1]
		&& cache->lightadj[2] == r_drawsurf.lightadj[2]
		&& cache->lightadj[3] == r_drawsurf.lightadj[3])
		return cache;

	//
	// determine shape of surface
	//
	surfscale = 1.0 / (1 << miplevel);
	r_drawsurf.surfmip = miplevel;
	r_drawsurf.surfwidth = surface->extents[0] >> miplevel;
	r_drawsurf.rowbytes = r_drawsurf.surfwidth;
	r_drawsurf.surfheight = surface->extents[1] >> miplevel;

	//
	// allocate memory if needed
	//
	if (!cache) // if a texture just animated, don't reallocate it
	{
		cache = D_SCAlloc(r_drawsurf.surfwidth,
		                  r_drawsurf.surfwidth * r_drawsurf.surfheight);
		surface->cachespots[miplevel] = cache;
		cache->owner = &surface->cachespots[miplevel];
		cache->mipscale = surfscale;
	}

	if (surface->dlightframe == r_framecount)
		cache->dlight = 1;
	else
		cache->dlight = 0;

	r_drawsurf.surfdat = (pixel_t*)cache->data;

	cache->texture = r_drawsurf.texture;
	cache->lightadj[0] = r_drawsurf.lightadj[0];
	cache->lightadj[1] = r_drawsurf.lightadj[1];
	cache->lightadj[2] = r_drawsurf.lightadj[2];
	cache->lightadj[3] = r_drawsurf.lightadj[3];

	//
	// draw and light the surface texture
	//
	r_drawsurf.surf = surface;

	c_surf++;
	R_DrawSurface();

	return surface->cachespots[miplevel];
}
