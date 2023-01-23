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
// r_sky.c

#include "cl_main.h"
#include "d_iface.h"
#include "mathlib.h"

int iskyspeed = 8;
int iskyspeed2 = 2;
float skyspeed, skyspeed2;

float skytime;

uint8_t* r_skysource;

int r_skymade;
int r_skydirect; // not used?


// TODO: clean up these routines

byte bottomsky[128 * 131];
byte bottommask[128 * 131];
byte new_sky[128 * 256]; // new_sky and topsky both pack in here, 128 bytes
//  of new_sky on the left of each scan, 128 bytes
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

	byte* src = (uint8_t*)mt + mt->offsets[0];

	for (i = 0; i < 128; i++)
	{
		for (j = 0; j < 128; j++)
		{
			new_sky[(i * 256) + j + 128] = src[i * 256 + j + 128];
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

	r_skysource = new_sky;
}


/*
=================
R_MakeSky
=================
*/
void R_MakeSky()
{
	static int xlast = -1, ylast = -1;

	int xshift = skytime * skyspeed;
	int yshift = skytime * skyspeed;

	if ((xshift == xlast) && (yshift == ylast))
		return;

	xlast = xshift;
	ylast = yshift;

	unsigned* pnew_sky = (unsigned*)&new_sky[0];

	for (int y = 0; y < SKYSIZE; y++)
	{
		int baseofs = ((y + yshift) & SKYMASK) * 131;
		
		for (int x = 0; x < SKYSIZE; x++)
		{
			int ofs = baseofs + ((x + xshift) & SKYMASK);

			*(uint8_t*)pnew_sky = (*((uint8_t*)pnew_sky + 128) &
					*&bottommask[ofs]) |
				*&bottomsky[ofs];
			pnew_sky = (unsigned*)((uint8_t*)pnew_sky + 1);
		}
		
		pnew_sky += 128 / sizeof(unsigned);
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
	int xshift = skytime * skyspeed;
	int yshift = skytime * skyspeed;

	unsigned* pnew_sky = (unsigned*)&new_sky[0];
	unsigned* pd = (unsigned*)pdest;

	for (int y = 0; y < SKYSIZE; y++)
	{
		int baseofs = ((y + yshift) & SKYMASK) * 131;
		
		for (int x = 0; x < SKYSIZE; x++)
		{
			int ofs = baseofs + ((x + xshift) & SKYMASK);

			*(uint8_t*)pd = (*((uint8_t*)pnew_sky + 128) &
					*&bottommask[ofs]) |
				*&bottomsky[ofs];
			pnew_sky = (unsigned*)((uint8_t*)pnew_sky + 1);
			pd = (unsigned*)((uint8_t*)pd + 1);
		}
		
		pnew_sky += 128 / sizeof(unsigned);
	}
}


/*
=================
R_GenSkyTile16
=================
*/
void R_GenSkyTile16(void* pdest)
{
	int xshift = skytime * skyspeed;
	int yshift = skytime * skyspeed;

	byte* pnew_sky = &new_sky[0];
	unsigned short* pd = (unsigned short*)pdest;

	for (int y = 0; y < SKYSIZE; y++)
	{
		int baseofs = ((y + yshift) & SKYMASK) * 131;

		// FIXME: clean this_ up
		// FIXME: do faster unaligned version?
		for (int x = 0; x < SKYSIZE; x++)
		{
			int ofs = baseofs + ((x + xshift) & SKYMASK);

			*pd = d_8to16table[(*(pnew_sky + 128) &
					*&bottommask[ofs]) |
				*&bottomsky[ofs]];
			pnew_sky++;
			pd++;
		}

		pnew_sky += TILE_SIZE;
	}
}


/*
=============
R_SetSkyFrame
==============
*/
void R_SetSkyFrame()
{
	skyspeed = iskyspeed;
	skyspeed2 = iskyspeed2;

	int g = GreatestCommonDivisor(iskyspeed, iskyspeed2);
	int s1 = iskyspeed / g;
	int s2 = iskyspeed2 / g;
	float temp = SKYSIZE * s1 * s2;

	skytime = cl.time - ((int)(cl.time / temp) * temp);


	r_skymade = 0;
}
