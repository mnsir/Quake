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
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_sky.c

#include "typedef_byte.h"

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
#include "r_local.h"
#include "d_local.h"


int iskyspeed = 8;
int iskyspeed2 = 2;
float skyspeed, skyspeed2;

float skytime;

byte* r_skysource;

int r_skymade;
int r_skydirect; // not used?


// TODO: clean up these routines

byte bottomsky[128 * 131];
byte bottommask[128 * 131];
byte newsky[128 * 256]; // newsky and topsky both pack in here, 128 bytes
//  of newsky on the left of each scan, 128 bytes
//  of topsky on the right, because the low-level
//  drawers need 256-byte scan widths


/*
=============
R_InitSky

A sky texture is 256*128, with the right side being a masked overlay
==============
*/
void R_InitSky(texture_t* mt)
{
	int i, j;
	byte* src;

	src = (byte*)mt + mt->offsets[0];

	for (i = 0; i < 128; i++)
	{
		for (j = 0; j < 128; j++)
		{
			newsky[(i * 256) + j + 128] = src[i * 256 + j + 128];
		}
	}

	for (i = 0; i < 128; i++)
	{
		for (j = 0; j < 131; j++)
		{
			if (src[i * 256 + (j & 0x7F)])
			{
				bottomsky[(i * 131) + j] = src[i * 256 + (j & 0x7F)];
				bottommask[(i * 131) + j] = 0;
			}
			else
			{
				bottomsky[(i * 131) + j] = 0;
				bottommask[(i * 131) + j] = 0xff;
			}
		}
	}

	r_skysource = newsky;
}


/*
=================
R_MakeSky
=================
*/
void R_MakeSky(void)
{
	int x, y;
	int ofs, baseofs;
	int xshift, yshift;
	unsigned* pnewsky;
	static int xlast = -1, ylast = -1;

	xshift = skytime * skyspeed;
	yshift = skytime * skyspeed;

	if ((xshift == xlast) && (yshift == ylast))
		return;

	xlast = xshift;
	ylast = yshift;

	pnewsky = (unsigned*)&newsky[0];

	for (y = 0; y < SKYSIZE; y++)
	{
		baseofs = ((y + yshift) & SKYMASK) * 131;
		
		for (x = 0; x < SKYSIZE; x++)
		{
			ofs = baseofs + ((x + xshift) & SKYMASK);

			*(byte*)pnewsky = (*((byte*)pnewsky + 128) &
					*&bottommask[ofs]) |
				*&bottomsky[ofs];
			pnewsky = (unsigned*)((byte*)pnewsky + 1);
		}
		
		pnewsky += 128 / sizeof(unsigned);
	}

	r_skymade = 1;
}


/*
=================
R_GenSkyTile
=================
*/
void R_GenSkyTile(void* pdest)
{
	int x, y;
	int ofs, baseofs;
	int xshift, yshift;
	unsigned* pnewsky;
	unsigned* pd;

	xshift = skytime * skyspeed;
	yshift = skytime * skyspeed;

	pnewsky = (unsigned*)&newsky[0];
	pd = (unsigned*)pdest;

	for (y = 0; y < SKYSIZE; y++)
	{
		baseofs = ((y + yshift) & SKYMASK) * 131;
		
		for (x = 0; x < SKYSIZE; x++)
		{
			ofs = baseofs + ((x + xshift) & SKYMASK);

			*(byte*)pd = (*((byte*)pnewsky + 128) &
					*&bottommask[ofs]) |
				*&bottomsky[ofs];
			pnewsky = (unsigned*)((byte*)pnewsky + 1);
			pd = (unsigned*)((byte*)pd + 1);
		}
		
		pnewsky += 128 / sizeof(unsigned);
	}
}


/*
=================
R_GenSkyTile16
=================
*/
void R_GenSkyTile16(void* pdest)
{
	int x, y;
	int ofs, baseofs;
	int xshift, yshift;
	byte* pnewsky;
	unsigned short* pd;

	xshift = skytime * skyspeed;
	yshift = skytime * skyspeed;

	pnewsky = &newsky[0];
	pd = (unsigned short*)pdest;

	for (y = 0; y < SKYSIZE; y++)
	{
		baseofs = ((y + yshift) & SKYMASK) * 131;

		// FIXME: clean this up
		// FIXME: do faster unaligned version?
		for (x = 0; x < SKYSIZE; x++)
		{
			ofs = baseofs + ((x + xshift) & SKYMASK);

			*pd = d_8to16table[(*(pnewsky + 128) &
					*&bottommask[ofs]) |
				*&bottomsky[ofs]];
			pnewsky++;
			pd++;
		}

		pnewsky += TILE_SIZE;
	}
}


/*
=============
R_SetSkyFrame
==============
*/
void R_SetSkyFrame(void)
{
	int g, s1, s2;
	float temp;

	skyspeed = iskyspeed;
	skyspeed2 = iskyspeed2;

	g = GreatestCommonDivisor(iskyspeed, iskyspeed2);
	s1 = iskyspeed / g;
	s2 = iskyspeed2 / g;
	temp = SKYSIZE * s1 * s2;

	skytime = cl.time - ((int)(cl.time / temp) * temp);


	r_skymade = 0;
}
