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
// gl_warp.c -- sky and water polygons

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
#include <format>
#include <numbers>

#include "host.h"
#include "chase.h"

extern model_t* loadmodel;

int solidskytexture;
int alphaskytexture;
float speedscale; // for top sky and bottom sky

msurface_t* warpface;

extern cvar_t gl_subdivide_size;

void BoundPoly(int numverts, float* verts, vec3_t& mins, vec3_t& maxs)
{
	mins[0] = mins[1] = mins[2] = 9999;
	maxs[0] = maxs[1] = maxs[2] = -9999;
	float* v = verts;
	for (int i = 0; i < numverts; i++)
		for (int j = 0; j < 3; j++, v++)
		{
			if (*v < mins[j])
				mins[j] = *v;
			if (*v > maxs[j])
				maxs[j] = *v;
		}
}

void SubdividePolygon(int numverts, float* verts)
{
	using namespace std::string_view_literals;
	int i, j;
	vec3_t mins, maxs;
	vec3_t front[64], back[64];
	int b;
	float dist[64];

	if (numverts > 60)
		Sys_Error(std::format("numverts = {}"sv, numverts));

	BoundPoly(numverts, verts, mins, maxs);

	for (i = 0; i < 3; i++)
	{
		float m = (mins[i] + maxs[i]) * 0.5;
		m = gl_subdivide_size.value * floor(m / gl_subdivide_size.value + 0.5);
		if (maxs[i] - m < 8)
			continue;
		if (m - mins[i] < 8)
			continue;

		// cut it
		float* v = verts + i;
		for (j = 0; j < numverts; j++, v += 3)
			dist[j] = *v - m;

		// wrap cases
		dist[j] = dist[0];
		v -= i;
		VectorCopy(ToVec3(verts), ToVec3(v));

		int f = b = 0;
		v = verts;
		for (j = 0; j < numverts; j++, v += 3)
		{
			if (dist[j] >= 0)
			{
				VectorCopy(ToVec3(v), front[f]);
				f++;
			}
			if (dist[j] <= 0)
			{
				VectorCopy(ToVec3(v), back[b]);
				b++;
			}
			if (dist[j] == 0 || dist[j + 1] == 0)
				continue;
			if ((dist[j] > 0) != (dist[j + 1] > 0))
			{
				// clip point
				float frac = dist[j] / (dist[j] - dist[j + 1]);
				for (int k = 0; k < 3; k++)
					front[f][k] = back[b][k] = v[k] + frac * (v[3 + k] - v[k]);
				f++;
				b++;
			}
		}

		SubdividePolygon(f, front[0].data());
		SubdividePolygon(b, back[0].data());
		return;
	}

	glpoly_t* poly = static_cast<glpoly_t*>(Hunk_Alloc(sizeof(glpoly_t) + (numverts - 4) * VERTEXSIZE * sizeof(float)));
	poly->next = warpface->polys;
	warpface->polys = poly;
	poly->numverts = numverts;
	for (i = 0; i < numverts; i++, verts += 3)
	{
		VectorCopy(ToVec3(verts), ToVec3(poly->verts[i]));
		float s = DotProduct(ToVec3(verts), ToVec3(warpface->texinfo->vecs[0]));
		float t = DotProduct(ToVec3(verts), ToVec3(warpface->texinfo->vecs[1]));
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;
	}
}

/*
================
GL_SubdivideSurface

Breaks a polygon up along axial 64 unit
boundaries so that turbulent and sky warps
can be done reasonably.
================
*/
void GL_SubdivideSurface(msurface_t* fa)
{
	vec3_t verts[64];
	vec3_t vec;
	texture_t* t;

	warpface = fa;

	//
	// convert edges back to a normal polygon
	//
	int numverts = 0;
	for (int i = 0; i < fa->numedges; i++)
	{
		int lindex = loadmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
			vec = loadmodel->vertexes[loadmodel->edges[lindex].v[0]].position;
		else
			vec = loadmodel->vertexes[loadmodel->edges[-lindex].v[1]].position;
		VectorCopy(vec, verts[numverts]);
		numverts++;
	}

	SubdividePolygon(numverts, verts[0].data());
}

//=========================================================


// speed up sin calculations - Ed
float turbsin[] =
{
	0, 0.19633, 0.392541, 0.588517, 0.784137, 0.979285, 1.17384, 1.3677,
	1.56072, 1.75281, 1.94384, 2.1337, 2.32228, 2.50945, 2.69512, 2.87916,
	3.06147, 3.24193, 3.42044, 3.59689, 3.77117, 3.94319, 4.11282, 4.27998,
	4.44456, 4.60647, 4.76559, 4.92185, 5.07515, 5.22538, 5.37247, 5.51632,
	5.65685, 5.79398, 5.92761, 6.05767, 6.18408, 6.30677, 6.42566, 6.54068,
	6.65176, 6.75883, 6.86183, 6.9607, 7.05537, 7.14579, 7.23191, 7.31368,
	7.39104, 7.46394, 7.53235, 7.59623, 7.65552, 7.71021, 7.76025, 7.80562,
	7.84628, 7.88222, 7.91341, 7.93984, 7.96148, 7.97832, 7.99036, 7.99759,
	8, 7.99759, 7.99036, 7.97832, 7.96148, 7.93984, 7.91341, 7.88222,
	7.84628, 7.80562, 7.76025, 7.71021, 7.65552, 7.59623, 7.53235, 7.46394,
	7.39104, 7.31368, 7.23191, 7.14579, 7.05537, 6.9607, 6.86183, 6.75883,
	6.65176, 6.54068, 6.42566, 6.30677, 6.18408, 6.05767, 5.92761, 5.79398,
	5.65685, 5.51632, 5.37247, 5.22538, 5.07515, 4.92185, 4.76559, 4.60647,
	4.44456, 4.27998, 4.11282, 3.94319, 3.77117, 3.59689, 3.42044, 3.24193,
	3.06147, 2.87916, 2.69512, 2.50945, 2.32228, 2.1337, 1.94384, 1.75281,
	1.56072, 1.3677, 1.17384, 0.979285, 0.784137, 0.588517, 0.392541, 0.19633,
	9.79717e-16, -0.19633, -0.392541, -0.588517, -0.784137, -0.979285, -1.17384, -1.3677,
	-1.56072, -1.75281, -1.94384, -2.1337, -2.32228, -2.50945, -2.69512, -2.87916,
	-3.06147, -3.24193, -3.42044, -3.59689, -3.77117, -3.94319, -4.11282, -4.27998,
	-4.44456, -4.60647, -4.76559, -4.92185, -5.07515, -5.22538, -5.37247, -5.51632,
	-5.65685, -5.79398, -5.92761, -6.05767, -6.18408, -6.30677, -6.42566, -6.54068,
	-6.65176, -6.75883, -6.86183, -6.9607, -7.05537, -7.14579, -7.23191, -7.31368,
	-7.39104, -7.46394, -7.53235, -7.59623, -7.65552, -7.71021, -7.76025, -7.80562,
	-7.84628, -7.88222, -7.91341, -7.93984, -7.96148, -7.97832, -7.99036, -7.99759,
	-8, -7.99759, -7.99036, -7.97832, -7.96148, -7.93984, -7.91341, -7.88222,
	-7.84628, -7.80562, -7.76025, -7.71021, -7.65552, -7.59623, -7.53235, -7.46394,
	-7.39104, -7.31368, -7.23191, -7.14579, -7.05537, -6.9607, -6.86183, -6.75883,
	-6.65176, -6.54068, -6.42566, -6.30677, -6.18408, -6.05767, -5.92761, -5.79398,
	-5.65685, -5.51632, -5.37247, -5.22538, -5.07515, -4.92185, -4.76559, -4.60647,
	-4.44456, -4.27998, -4.11282, -3.94319, -3.77117, -3.59689, -3.42044, -3.24193,
	-3.06147, -2.87916, -2.69512, -2.50945, -2.32228, -2.1337, -1.94384, -1.75281,
	-1.56072, -1.3677, -1.17384, -0.979285, -0.784137, -0.588517, -0.392541, -0.19633,
};
constexpr auto TURBSCALE = (256.0 / (2 * std::numbers::pi));

/*
=============
EmitWaterPolys

Does a water warp on the pre-fragmented glpoly_t chain
=============
*/
void EmitWaterPolys(msurface_t* fa)
{
	float* v;
	int i;


	for (glpoly_t* p = fa->polys; p; p = p->next)
	{
		glBegin(GL_POLYGON);
		for (i = 0, v = p->verts[0]; i < p->numverts; i++, v += VERTEXSIZE)
		{
			float os = v[3];
			float ot = v[4];

			float s = os + turbsin[(int)((ot * 0.125 + realtime) * TURBSCALE) & 255];
			s *= (1.0 / 64);

			float t = ot + turbsin[(int)((os * 0.125 + realtime) * TURBSCALE) & 255];
			t *= (1.0 / 64);

			glTexCoord2f(s, t);
			glVertex3fv(v);
		}
		glEnd();
	}
}


/*
=============
EmitSkyPolys
=============
*/
void EmitSkyPolys(msurface_t* fa)
{
	float* v;
	int i;
	vec3_t dir;

	for (glpoly_t* p = fa->polys; p; p = p->next)
	{
		glBegin(GL_POLYGON);
		for (i = 0, v = p->verts[0]; i < p->numverts; i++, v += VERTEXSIZE)
		{
			VectorSubtract(ToVec3(v), r_origin, dir);
			dir[2] *= 3; // flatten the sphere

			float length = dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];
			length = sqrt(length);
			length = 6 * 63 / length;

			dir[0] *= length;
			dir[1] *= length;

			float s = (speedscale + dir[0]) * (1.0 / 128);
			float t = (speedscale + dir[1]) * (1.0 / 128);

			glTexCoord2f(s, t);
			glVertex3fv(v);
		}
		glEnd();
	}
}

/*
===============
EmitBothSkyLayers

Does a sky warp on the pre-fragmented glpoly_t chain
This will be called for brushmodels, the world
will have them chained together.
===============
*/
void EmitBothSkyLayers(msurface_t* fa)
{
	int i;
	int lindex;
	float* vec;

	GL_DisableMultitexture();

	GL_Bind(solidskytexture);
	speedscale = realtime * 8;
	speedscale -= (int)speedscale & ~127;

	EmitSkyPolys(fa);

	glEnable(GL_BLEND);
	GL_Bind(alphaskytexture);
	speedscale = realtime * 16;
	speedscale -= (int)speedscale & ~127;

	EmitSkyPolys(fa);

	glDisable(GL_BLEND);
}


/*
=================
R_DrawSkyChain
=================
*/
void R_DrawSkyChain(msurface_t* s)
{
	msurface_t* fa;

	GL_DisableMultitexture();

	// used when gl_texsort is on
	GL_Bind(solidskytexture);
	speedscale = realtime * 8;
	speedscale -= (int)speedscale & ~127;

	for (fa = s; fa; fa = fa->texturechain)
		EmitSkyPolys(fa);

	glEnable(GL_BLEND);
	GL_Bind(alphaskytexture);
	speedscale = realtime * 16;
	speedscale -= (int)speedscale & ~127;

	for (fa = s; fa; fa = fa->texturechain)
		EmitSkyPolys(fa);

	glDisable(GL_BLEND);
}


/*
=================================================================

  Quake 2 environment sky

=================================================================
*/


//===============================================================

/*
=============
R_InitSky

A sky texture is 256*128, with the right side being a masked overlay
==============
*/
void R_InitSky(texture_t* mt)
{
	int i, j, p;
	unsigned trans[128 * 128];
	unsigned transpix;
	int g, b;
	extern int skytexturenum;

	byte* src = (uint8_t*)mt + mt->offsets[0];

	// make an average value for the back to avoid
	// a fringe on the top level

	int r = g = b = 0;
	for (i = 0; i < 128; i++)
		for (j = 0; j < 128; j++)
		{
			p = src[i * 256 + j + 128];
			unsigned* rgba = &d_8to24table[p];
			trans[(i * 128) + j] = *rgba;
			r += ((uint8_t*)rgba)[0];
			g += ((uint8_t*)rgba)[1];
			b += ((uint8_t*)rgba)[2];
		}

	((uint8_t*)&transpix)[0] = r / (128 * 128);
	((uint8_t*)&transpix)[1] = g / (128 * 128);
	((uint8_t*)&transpix)[2] = b / (128 * 128);
	((uint8_t*)&transpix)[3] = 0;


	if (!solidskytexture)
		solidskytexture = texture_extension_number++;
	GL_Bind(solidskytexture);
	glTexImage2D(GL_TEXTURE_2D, 0, gl_solid_format, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	for (i = 0; i < 128; i++)
		for (j = 0; j < 128; j++)
		{
			p = src[i * 256 + j];
			if (p == 0)
				trans[(i * 128) + j] = transpix;
			else
				trans[(i * 128) + j] = d_8to24table[p];
		}

	if (!alphaskytexture)
		alphaskytexture = texture_extension_number++;
	GL_Bind(alphaskytexture);
	glTexImage2D(GL_TEXTURE_2D, 0, gl_alpha_format, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
