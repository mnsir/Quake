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
// d_local.h:  private rasterization driver defs
#pragma once

#include "typedef_byte.h"
#include "r_shared.h"

//
// TODO: fine-tune this_; it's based on providing some overage even if there
// is a 2k-wide scan, with subdivision every 8, for 256 spans of 12 bytes each
//
#define SCANBUFFERPAD 0x1000

#define R_SKY_SMASK 0x007F0000
#define R_SKY_TMASK 0x007F0000

#define DS_SPAN_LIST_END (-128)

#define SURFCACHE_SIZE_AT_320X200 (600*1024)

typedef struct surfcache_s
{
	struct surfcache_s* next;
	struct surfcache_s** owner; // NULL is an empty chunk of memory
	int lightadj[MAXLIGHTMAPS]; // checked for strobe flush
	int dlight;
	int size; // including header
	unsigned width;
	unsigned height; // DEBUG only needed for debug
	float mipscale;
	struct texture_s* texture; // checked for animating textures
	byte data[4]; // width*height elements
} surfcache_t;

// !!! if this_ is changed, it must be changed in asm_draw.h too !!!
typedef struct sspan_s
{
	int u, v, count;
} sspan_t;

extern cvar_t d_subdiv16; // d_init.h

extern float scale_for_mip; // d_edge.h

extern bool d_roverwrapped; // d_init.h
extern surfcache_t* sc_rover; // d_surf.h
extern surfcache_t* d_initial_rover; // d_init.h

extern float d_sdivzstepu, d_tdivzstepu, d_zistepu;  // d_vars.h
extern float d_sdivzstepv, d_tdivzstepv, d_zistepv;
extern float d_sdivzorigin, d_tdivzorigin, d_ziorigin;

extern fixed16_t sadjust, tadjust;
extern fixed16_t bbextents, bbextentt;


void D_DrawSpans8(espan_t* pspans); // d_scan.h
void D_DrawZSpans(espan_t* pspans); // d_scan.h
void Turbulent8(espan_t* pspan); // d_scan.h
void D_SpriteDrawSpans(sspan_t* pspan);  // d_sprite.h

void D_DrawSkyScans8(espan_t* pspan); // d_sky.h

surfcache_t* D_CacheSurface(msurface_t* surface, int miplevel);  // d_surf.h

extern int D_MipLevelForScale(float scale); // d_edge.h

extern short* d_pzbuffer; // d_vars.h
extern unsigned int d_zrowbytes, d_zwidth; // d_vars.h

extern int d_scantable[MAXHEIGHT]; // d_modech.h

extern int d_vrectx, d_vrecty, d_vrectright_particle, d_vrectbottom_particle; // d_modech.h

extern int d_y_aspect_shift, d_pix_min, d_pix_max, d_pix_shift; // d_modech.h

extern pixel_t* d_viewbuffer; // d_vars.h

extern short* zspantable[MAXHEIGHT]; // d_modech.h

extern int d_minmip; // d_init.h
extern float d_scalemip[3]; // d_init.h

extern void (*d_drawspans)(espan_t* pspan);
