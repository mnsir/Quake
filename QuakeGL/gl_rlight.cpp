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
// r_light.c

#include <cmath>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include "bspfile.h"
#include "vid.h"
#include "sys.h"
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

#include "common_model.h"

#ifdef GLQUAKE
#else
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
#include <numbers>

#include "host.h"
#include "chase.h"

int r_dlightframecount;


/*
==================
R_AnimateLight
==================
*/
void R_AnimateLight()
{
	//
	// light animations
	// 'm' is normal light, 'a' is no light, 'z' is double bright
	int i = (int)(cl.time * 10);
	for (int j = 0; j < MAX_LIGHTSTYLES; j++)
	{
		if (!cl_lightstyle[j].length)
		{
			d_lightstylevalue[j] = 256;
			continue;
		}
		int k = i % cl_lightstyle[j].length;
		k = cl_lightstyle[j].map[k] - 'a';
		k = k * 22;
		d_lightstylevalue[j] = k;
	}
}

/*
=============================================================================

DYNAMIC LIGHTS BLEND RENDERING

=============================================================================
*/

void AddLightBlend(float r, float g, float b, float a2)
{
	float a;

	v_blend[3] = a = v_blend[3] + a2 * (1 - v_blend[3]);

	a2 = a2 / a;

	v_blend[0] = v_blend[1] * (1 - a2) + r * a2;
	v_blend[1] = v_blend[1] * (1 - a2) + g * a2;
	v_blend[2] = v_blend[2] * (1 - a2) + b * a2;
}

void R_RenderDlight(dlight_t* light)
{
	int i;
	vec3_t v;

	float rad = light->radius * 0.35;

	VectorSubtract(light->origin, r_origin, v);
	if (Length(v) < rad)
	{
		// view is inside the dlight
		AddLightBlend(1, 0.5, 0, light->radius * 0.0003);
		return;
	}

	glBegin(GL_TRIANGLE_FAN);
	glColor3f(0.2, 0.1, 0.0);
	for (i = 0; i < 3; i++)
		v[i] = light->origin[i] - vpn[i] * rad;
	glVertex3fv(v.data());
	glColor3f(0, 0, 0);
	for (i = 16; i >= 0; i--)
	{
		float a = i / 16.0 * std::numbers::pi * 2;
		for (int j = 0; j < 3; j++)
			v[j] = light->origin[j] + vright[j] * cos(a) * rad
				+ vup[j] * sin(a) * rad;
		glVertex3fv(v.data());
	}
	glEnd();
}

/*
=============
R_RenderDlights
=============
*/
void R_RenderDlights()
{
	if (!gl_flashblend.value)
		return;

	r_dlightframecount = r_framecount + 1; // because the count hasn't
	//  advanced yet for this_ frame
	glDepthMask(0);
	glDisable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	dlight_t* l = cl_dlights;
	for (int i = 0; i < MAX_DLIGHTS; i++, l++)
	{
		if (l->die < cl.time || !l->radius)
			continue;
		R_RenderDlight(l);
	}

	glColor3f(1, 1, 1);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(1);
}


/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_MarkLights
=============
*/
void R_MarkLights(dlight_t* light, int bit, mnode_t* node)
{
	if (node->contents < 0)
		return;

	mplane_t* splitplane = node->plane;
	float dist = DotProduct(light->origin, splitplane->normal) - splitplane->dist;

	if (dist > light->radius)
	{
		R_MarkLights(light, bit, node->children[0]);
		return;
	}
	if (dist < -light->radius)
	{
		R_MarkLights(light, bit, node->children[1]);
		return;
	}

	// mark the polygons
	msurface_t* surf = cl.worldmodel->surfaces + node->firstsurface;
	for (int i = 0; i < node->numsurfaces; i++, surf++)
	{
		if (surf->dlightframe != r_dlightframecount)
		{
			surf->dlightbits = 0;
			surf->dlightframe = r_dlightframecount;
		}
		surf->dlightbits |= bit;
	}

	R_MarkLights(light, bit, node->children[0]);
	R_MarkLights(light, bit, node->children[1]);
}


/*
=============
R_PushDlights
=============
*/
void R_PushDlights()
{
	if (gl_flashblend.value)
		return;

	r_dlightframecount = r_framecount + 1; // because the count hasn't
	//  advanced yet for this_ frame
	dlight_t* l = cl_dlights;

	for (int i = 0; i < MAX_DLIGHTS; i++, l++)
	{
		if (l->die < cl.time || !l->radius)
			continue;
		R_MarkLights(l, 1 << i, cl.worldmodel->nodes);
	}
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

mplane_t* lightplane;
vec3_t lightspot;

int RecursiveLightPoint(mnode_t* node, const vec3_t& start, const vec3_t& end)
{
	vec3_t mid;

	if (node->contents < 0)
		return -1; // didn't hit anything

	// calculate mid point

	// FIXME: optimize for axial
	mplane_t* plane = node->plane;
	float front = DotProduct(start, plane->normal) - plane->dist;
	float back = DotProduct(end, plane->normal) - plane->dist;
	int side = front < 0;

	if ((back < 0) == side)
		return RecursiveLightPoint(node->children[side], start, end);

	float frac = front / (front - back);
	mid[0] = start[0] + (end[0] - start[0]) * frac;
	mid[1] = start[1] + (end[1] - start[1]) * frac;
	mid[2] = start[2] + (end[2] - start[2]) * frac;

	// go down front side 
	int r = RecursiveLightPoint(node->children[side], start, mid);
	if (r >= 0)
		return r; // hit something

	if ((back < 0) == side)
		return -1; // didn't hit anuthing

	// check for impact on this_ node
	VectorCopy(mid, lightspot);
	lightplane = plane;

	msurface_t* surf = cl.worldmodel->surfaces + node->firstsurface;
	for (int i = 0; i < node->numsurfaces; i++, surf++)
	{
		if (surf->flags & SURF_DRAWTILED)
			continue; // no lightmaps

		mtexinfo_t* tex = surf->texinfo;

		int s = DotProduct(mid, ToVec3(tex->vecs[0])) + tex->vecs[0][3];
		int t = DotProduct(mid, ToVec3(tex->vecs[1])) + tex->vecs[1][3];

		if (s < surf->texturemins[0] ||
			t < surf->texturemins[1])
			continue;

		int ds = s - surf->texturemins[0];
		int dt = t - surf->texturemins[1];

		if (ds > surf->extents[0] || dt > surf->extents[1])
			continue;

		if (!surf->samples)
			return 0;

		ds >>= 4;
		dt >>= 4;

		byte* lightmap = surf->samples;
		r = 0;
		if (lightmap)
		{
			lightmap += dt * ((surf->extents[0] >> 4) + 1) + ds;

			for (int maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255;
			     maps++)
			{
				unsigned scale = d_lightstylevalue[surf->styles[maps]];
				r += *lightmap * scale;
				lightmap += ((surf->extents[0] >> 4) + 1) *
					((surf->extents[1] >> 4) + 1);
			}

			r >>= 8;
		}

		return r;
	}

	// go down back side
	return RecursiveLightPoint(node->children[!side], mid, end);
}

int R_LightPoint(const vec3_t& p)
{
	vec3_t end;

	if (!cl.worldmodel->lightdata)
		return 255;

	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;

	int r = RecursiveLightPoint(cl.worldmodel->nodes, p, end);

	if (r == -1)
		r = 0;

	return r;
}
