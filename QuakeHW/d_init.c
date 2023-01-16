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
// d_init.c: rasterization driver initialization


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
#include "d_local.h"

#define NUM_MIPS 4

cvar_t d_subdiv16 = {"d_subdiv16", "1"};
cvar_t d_mipcap = {"d_mipcap", "0"};
cvar_t d_mipscale = {"d_mipscale", "1"};

surfcache_t* d_initial_rover;
qboolean d_roverwrapped;
int d_minmip;
float d_scalemip[NUM_MIPS - 1];

static float basemip[NUM_MIPS - 1] = {1.0, 0.5 * 0.8, 0.25 * 0.8};

extern int d_aflatcolor;

void (*d_drawspans)(espan_t* pspan);


/*
===============
D_Init
===============
*/
void D_Init(void)
{
	r_skydirect = 1;

	Cvar_RegisterVariable(&d_subdiv16);
	Cvar_RegisterVariable(&d_mipcap);
	Cvar_RegisterVariable(&d_mipscale);

	r_drawpolys = false;
	r_worldpolysbacktofront = false;
	r_recursiveaffinetriangles = true;
	r_pixbytes = 1;
	r_aliasuvscale = 1.0;
}


/*
===============
D_CopyRects
===============
*/
void D_CopyRects(vrect_t* prects, int transparent)
{
	// this function is only required if the CPU doesn't have direct access to the
	// back buffer, and there's some driver interface function that the driver
	// doesn't support and requires Quake to do in software (such as drawing the
	// console); Quake will then draw into wherever the driver points vid.buffer
	// and will call this function before swapping buffers

	UNUSED(prects);
	UNUSED(transparent);
}


/*
===============
D_EnableBackBufferAccess
===============
*/
void D_EnableBackBufferAccess(void)
{
	VID_LockBuffer();
}


/*
===============
D_TurnZOn
===============
*/
void D_TurnZOn(void)
{
	// not needed for software version
}


/*
===============
D_DisableBackBufferAccess
===============
*/
void D_DisableBackBufferAccess(void)
{
	VID_UnlockBuffer();
}


/*
===============
D_SetupFrame
===============
*/
void D_SetupFrame(void)
{
	int i;

	if (r_dowarp)
		d_viewbuffer = r_warpbuffer;
	else
		d_viewbuffer = (void*)vid.buffer;

	if (r_dowarp)
		screenwidth = WARP_WIDTH;
	else
		screenwidth = vid.rowbytes;

	d_roverwrapped = false;
	d_initial_rover = sc_rover;

	d_minmip = d_mipcap.value;
	if (d_minmip > 3)
		d_minmip = 3;
	else if (d_minmip < 0)
		d_minmip = 0;

	for (i = 0; i < (NUM_MIPS - 1); i++)
		d_scalemip[i] = basemip[i] * d_mipscale.value;
	
	d_drawspans = D_DrawSpans8;

	d_aflatcolor = 0;
}


/*
===============
D_UpdateRects
===============
*/
void D_UpdateRects(vrect_t* prect)
{
	// the software driver draws these directly to the vid buffer

	UNUSED(prect);
}
