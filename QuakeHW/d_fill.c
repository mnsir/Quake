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
// d_clear: clears a specified rectangle to the specified color

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


/*
================
D_FillRect
================
*/
void D_FillRect(vrect_t* rect, int color)
{
	int rx, ry, rwidth, rheight;
	unsigned char* dest;
	unsigned* ldest;

	rx = rect->x;
	ry = rect->y;
	rwidth = rect->width;
	rheight = rect->height;

	if (rx < 0)
	{
		rwidth += rx;
		rx = 0;
	}
	if (ry < 0)
	{
		rheight += ry;
		ry = 0;
	}
	if (rx + rwidth > vid.width)
		rwidth = vid.width - rx;
	if (ry + rheight > vid.height)
		rheight = vid.height - rx;

	if (rwidth < 1 || rheight < 1)
		return;

	dest = (vid.buffer + ry * vid.rowbytes + rx);

	if (((rwidth & 0x03) == 0) && (((long)dest & 0x03) == 0))
	{
		// faster aligned dword clear
		ldest = (unsigned*)dest;
		color += color << 16;

		rwidth >>= 2;
		color += color << 8;

		for (ry = 0; ry < rheight; ry++)
		{
			for (rx = 0; rx < rwidth; rx++)
				ldest[rx] = color;
			ldest = (unsigned*)((byte*)ldest + vid.rowbytes);
		}
	}
	else
	{
		// slower byte-by-byte clear for unaligned cases
		for (ry = 0; ry < rheight; ry++)
		{
			for (rx = 0; rx < rwidth; rx++)
				dest[rx] = color;
			dest += vid.rowbytes;
		}
	}
}
