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
// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "model.h"

#include <cmath>
#include <format>

#include "console.h"
#include "d_iface.h"
#include "r_local.h"
#include "sys.h"


model_t* loadmodel;
char loadname[32]; // for hunk tags

void Mod_LoadSpriteModel(model_t* mod, void* buffer);
void Mod_LoadBrushModel(model_t* mod, void* buffer);
void Mod_LoadAliasModel(model_t* mod, void* buffer);
model_t* Mod_LoadModel(model_t* mod, bool crash);

byte mod_novis[MAX_MAP_LEAFS / 8];

#define MAX_MOD_KNOWN 256
model_t mod_known[MAX_MOD_KNOWN];
int mod_numknown;

// values for model_t's needload
#define NL_PRESENT 0
#define NL_NEEDS_LOADED 1
#define NL_UNREFERENCED 2

/*
===============
Mod_Init
===============
*/
void Mod_Init(void)
{
	memset(mod_novis, 0xff, sizeof(mod_novis));
}

/*
===============
Mod_Extradata

Caches the data if needed
===============
*/
void* Mod_Extradata(model_t* mod)
{
	using namespace std::string_view_literals;
	void* r = Cache_Check(&mod->cache);
	if (r)
		return r;

	Mod_LoadModel(mod, true);

	if (!mod->cache.data)
		Sys_Error("Mod_Extradata: caching failed"sv);
	return mod->cache.data;
}

/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t* Mod_PointInLeaf(vec3_t p, model_t* model)
{
	using namespace std::string_view_literals;
	if (!model || !model->nodes)
		Sys_Error("Mod_PointInLeaf: bad model"sv);

	mnode_t* node = model->nodes;
	while (true)
	{
		if (node->contents < 0)
			return (mleaf_t*)node;
		mplane_t* plane = node->plane;
		float d = DotProduct(p, plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return NULL; // never reached
}


/*
===================
Mod_DecompressVis
===================
*/
byte* Mod_DecompressVis(byte* in, model_t* model)
{
	static byte decompressed[MAX_MAP_LEAFS / 8];

	int row = (model->numleafs + 7) >> 3;
	byte* out = decompressed;

	if (!in)
	{
		// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		int c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);

	return decompressed;
}

byte* Mod_LeafPVS(mleaf_t* leaf, model_t* model)
{
	if (leaf == model->leafs)
		return mod_novis;
	return Mod_DecompressVis(leaf->compressed_vis, model);
}

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll(void)
{
	int i;
	model_t* mod;


	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		mod->needload = NL_UNREFERENCED;
		//FIX FOR CACHE_ALLOC ERRORS:
		if (mod->type == mod_sprite) mod->cache.data = NULL;
	}
}

/*
==================
Mod_FindName

==================
*/
model_t* Mod_FindName(char* name)
{
	using namespace std::string_view_literals;
	int i;
	model_t* mod;
	model_t* avail = NULL;

	if (!name[0])
		Sys_Error("Mod_ForName: NULL name"sv);

	//
	// search the currently loaded models
	//
	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		if (!strcmp(mod->name, name))
			break;
		if (mod->needload == NL_UNREFERENCED)
			if (!avail || mod->type != mod_alias)
				avail = mod;
	}

	if (i == mod_numknown)
	{
		if (mod_numknown == MAX_MOD_KNOWN)
		{
			if (avail)
			{
				mod = avail;
				if (mod->type == mod_alias)
					if (Cache_Check(&mod->cache))
						Cache_Free(&mod->cache);
			}
			else
				Sys_Error("mod_numknown == MAX_MOD_KNOWN"sv);
		}
		else
			mod_numknown++;
		strcpy(mod->name, name);
		mod->needload = NL_NEEDS_LOADED;
	}

	return mod;
}

/*
==================
Mod_TouchModel

==================
*/
void Mod_TouchModel(char* name)
{
	model_t* mod = Mod_FindName(name);

	if (mod->needload == NL_PRESENT)
	{
		if (mod->type == mod_alias)
			Cache_Check(&mod->cache);
	}
}

/*
==================
Mod_LoadModel

Loads a model into the cache
==================
*/
model_t* Mod_LoadModel(model_t* mod, bool crash)
{
	using namespace std::string_view_literals;
	byte stackbuf[1024]; // avoid dirtying the cache heap

	if (mod->type == mod_alias)
	{
		if (Cache_Check(&mod->cache))
		{
			mod->needload = NL_PRESENT;
			return mod;
		}
	}
	else
	{
		if (mod->needload == NL_PRESENT)
			return mod;
	}

	//
	// because the world is so huge, load it one piece at a time
	//

	//
	// load the file
	//
	unsigned* buf = (unsigned*)COM_LoadStackFile(mod->name, stackbuf, sizeof(stackbuf));
	if (!buf)
	{
		if (crash)
			Sys_Error(std::format("Mod_NumForName: {} not found"sv, mod->name));
		return NULL;
	}

	//
	// allocate a new_ model
	//
	COM_FileBase(mod->name, loadname);

	loadmodel = mod;

	//
	// fill it in
	//

	// call the apropriate loader
	mod->needload = NL_PRESENT;

	switch (LittleLong(*buf))
	{
	case IDPOLYHEADER:
		Mod_LoadAliasModel(mod, buf);
		break;

	case IDSPRITEHEADER:
		Mod_LoadSpriteModel(mod, buf);
		break;

	default:
		Mod_LoadBrushModel(mod, buf);
		break;
	}

	return mod;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t* Mod_ForName(char* name, bool crash)
{
	model_t* mod = Mod_FindName(name);

	return Mod_LoadModel(mod, crash);
}


/*
===============================================================================

 BRUSHMODEL LOADING

===============================================================================
*/

byte* mod_base;


/*
=================
Mod_LoadTextures
=================
*/
void Mod_LoadTextures(lump_t* l)
{
	using namespace std::string_view_literals;
	int i, j;
	texture_t* tx, * tx2;
	texture_t* anims[10];
	texture_t* altanims[10];

	if (!l->filelen)
	{
		loadmodel->textures = NULL;
		return;
	}
	dmiptexlump_t* m = (dmiptexlump_t*)(mod_base + l->fileofs);

	m->nummiptex = LittleLong(m->nummiptex);

	loadmodel->numtextures = m->nummiptex;
	loadmodel->textures = static_cast<texture_t**>(Hunk_AllocName(m->nummiptex * sizeof*loadmodel->textures, loadname));

	for (i = 0; i < m->nummiptex; i++)
	{
		m->dataofs[i] = LittleLong(m->dataofs[i]);
		if (m->dataofs[i] == -1)
			continue;
		miptex_t* mt = (miptex_t*)((byte*)m + m->dataofs[i]);
		mt->width = LittleLong(mt->width);
		mt->height = LittleLong(mt->height);
		for (j = 0; j < MIPLEVELS; j++)
			mt->offsets[j] = LittleLong(mt->offsets[j]);

		if ((mt->width & 15) || (mt->height & 15))
			Sys_Error(std::format("Texture {} is not 16 aligned"sv, mt->name));
		int pixels = mt->width * mt->height / 64 * 85;
		tx = static_cast<texture_t*>(Hunk_AllocName(sizeof(texture_t) + pixels, loadname));
		loadmodel->textures[i] = tx;

		memcpy(tx->name, mt->name, sizeof(tx->name));
		tx->width = mt->width;
		tx->height = mt->height;
		for (j = 0; j < MIPLEVELS; j++)
			tx->offsets[j] = mt->offsets[j] + sizeof(texture_t) - sizeof(miptex_t);
		// the pixels immediately follow the structures
		memcpy(tx + 1, mt + 1, pixels);

		if (!Q_strncmp(mt->name, (char*)"sky", 3))
			R_InitSky(tx);
	}

	//
	// sequence the animations
	//
	for (i = 0; i < m->nummiptex; i++)
	{
		tx = loadmodel->textures[i];
		if (!tx || tx->name[0] != '+')
			continue;
		if (tx->anim_next)
			continue; // allready sequenced

		// find the number of frames in the animation
		memset(anims, 0, sizeof(anims));
		memset(altanims, 0, sizeof(altanims));

		int max = tx->name[1];
		int altmax = 0;
		if (max >= 'a' && max <= 'z')
			max -= 'a' - 'A';
		if (max >= '0' && max <= '9')
		{
			max -= '0';
			altmax = 0;
			anims[max] = tx;
			max++;
		}
		else if (max >= 'A' && max <= 'J')
		{
			altmax = max - 'A';
			max = 0;
			altanims[altmax] = tx;
			altmax++;
		}
		else
			Sys_Error(std::format("Bad animating texture {}"sv, tx->name));

		for (j = i + 1; j < m->nummiptex; j++)
		{
			tx2 = loadmodel->textures[j];
			if (!tx2 || tx2->name[0] != '+')
				continue;
			if (strcmp(tx2->name + 2, tx->name + 2))
				continue;

			int num = tx2->name[1];
			if (num >= 'a' && num <= 'z')
				num -= 'a' - 'A';
			if (num >= '0' && num <= '9')
			{
				num -= '0';
				anims[num] = tx2;
				if (num + 1 > max)
					max = num + 1;
			}
			else if (num >= 'A' && num <= 'J')
			{
				num = num - 'A';
				altanims[num] = tx2;
				if (num + 1 > altmax)
					altmax = num + 1;
			}
			else
				Sys_Error(std::format("Bad animating texture {}"sv, tx->name));
		}

#define ANIM_CYCLE 2
		// link them all together
		for (j = 0; j < max; j++)
		{
			tx2 = anims[j];
			if (!tx2)
				Sys_Error(std::format("Missing frame {} of {}"sv, j, tx->name));
			tx2->anim_total = max * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j + 1) * ANIM_CYCLE;
			tx2->anim_next = anims[(j + 1) % max];
			if (altmax)
				tx2->alternate_anims = altanims[0];
		}
		for (j = 0; j < altmax; j++)
		{
			tx2 = altanims[j];
			if (!tx2)
				Sys_Error(std::format("Missing frame {} of {}"sv, j, tx->name));
			tx2->anim_total = altmax * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j + 1) * ANIM_CYCLE;
			tx2->anim_next = altanims[(j + 1) % altmax];
			if (max)
				tx2->alternate_anims = anims[0];
		}
	}
}

/*
=================
Mod_LoadLighting
=================
*/
void Mod_LoadLighting(lump_t* l)
{
	if (!l->filelen)
	{
		loadmodel->lightdata = NULL;
		return;
	}
	loadmodel->lightdata = static_cast<byte*>(Hunk_AllocName(l->filelen, loadname));
	memcpy(loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadVisibility
=================
*/
void Mod_LoadVisibility(lump_t* l)
{
	if (!l->filelen)
	{
		loadmodel->visdata = NULL;
		return;
	}
	loadmodel->visdata = static_cast<byte*>(Hunk_AllocName(l->filelen, loadname));
	memcpy(loadmodel->visdata, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadEntities
=================
*/
void Mod_LoadEntities(lump_t* l)
{
	if (!l->filelen)
	{
		loadmodel->entities = NULL;
		return;
	}
	loadmodel->entities = static_cast<char*>(Hunk_AllocName(l->filelen, loadname));
	memcpy(loadmodel->entities, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadVertexes
=================
*/
void Mod_LoadVertexes(lump_t* l)
{
	using namespace std::string_view_literals;
	dvertex_t* in = static_cast<dvertex_t*>((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		Sys_Error(std::format("MOD_LoadBmodel: funny lump size in {}"sv, loadmodel->name));
	int count = l->filelen / sizeof(*in);
	mvertex_t* out = static_cast<mvertex_t*>(Hunk_AllocName(count * sizeof*out, loadname));

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		out->position[0] = LittleFloat(in->point[0]);
		out->position[1] = LittleFloat(in->point[1]);
		out->position[2] = LittleFloat(in->point[2]);
	}
}

/*
=================
Mod_LoadSubmodels
=================
*/
void Mod_LoadSubmodels(lump_t* l)
{
	using namespace std::string_view_literals;
	int j;

	dmodel_t* in = static_cast<dmodel_t*>((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		Sys_Error(std::format("MOD_LoadBmodel: funny lump size in {}"sv, loadmodel->name));
	int count = l->filelen / sizeof(*in);
	dmodel_t* out = static_cast<dmodel_t*>(Hunk_AllocName(count * sizeof*out, loadname));

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
			out->origin[j] = LittleFloat(in->origin[j]);
		}
		for (j = 0; j < MAX_MAP_HULLS; j++)
			out->headnode[j] = LittleLong(in->headnode[j]);
		out->visleafs = LittleLong(in->visleafs);
		out->firstface = LittleLong(in->firstface);
		out->numfaces = LittleLong(in->numfaces);
	}
}

/*
=================
Mod_LoadEdges
=================
*/
void Mod_LoadEdges(lump_t* l)
{
	using namespace std::string_view_literals;
	dedge_t* in = static_cast<dedge_t*>((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		Sys_Error(std::format("MOD_LoadBmodel: funny lump size in {}"sv, loadmodel->name));
	int count = l->filelen / sizeof(*in);
	medge_t* out = static_cast<medge_t*>(Hunk_AllocName((count + 1) * sizeof*out, loadname));

	loadmodel->edges = out;
	loadmodel->numedges = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
	}
}

/*
=================
Mod_LoadTexinfo
=================
*/
void Mod_LoadTexinfo(lump_t* l)
{
	using namespace std::string_view_literals;
	texinfo_t* in = static_cast<texinfo_t*>((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		Sys_Error(std::format("MOD_LoadBmodel: funny lump size in {}"sv, loadmodel->name));
	int count = l->filelen / sizeof(*in);
	mtexinfo_t* out = static_cast<mtexinfo_t*>(Hunk_AllocName(count * sizeof*out, loadname));

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		for (int j = 0; j < 8; j++)
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
		float len1 = Length(out->vecs[0]);
		float len2 = Length(out->vecs[1]);
		len1 = (len1 + len2) / 2;
		if (len1 < 0.32)
			out->mipadjust = 4;
		else if (len1 < 0.49)
			out->mipadjust = 3;
		else if (len1 < 0.99)
			out->mipadjust = 2;
		else
			out->mipadjust = 1;

		int miptex = LittleLong(in->miptex);
		out->flags = LittleLong(in->flags);

		if (!loadmodel->textures)
		{
			out->texture = r_notexture_mip; // checkerboard texture
			out->flags = 0;
		}
		else
		{
			if (miptex >= loadmodel->numtextures)
				Sys_Error("miptex >= loadmodel->numtextures"sv);
			out->texture = loadmodel->textures[miptex];
			if (!out->texture)
			{
				out->texture = r_notexture_mip; // texture not found
				out->flags = 0;
			}
		}
	}
}

/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
void CalcSurfaceExtents(msurface_t* s)
{
	using namespace std::string_view_literals;
	float mins[2], maxs[2];
	int i;
	mvertex_t* v;
	int bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	mtexinfo_t* tex = s->texinfo;

	for (i = 0; i < s->numedges; i++)
	{
		int e = loadmodel->surfedges[s->firstedge + i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

		for (int j = 0; j < 2; j++)
		{
			float val = v->position[0] * tex->vecs[j][0] +
				v->position[1] * tex->vecs[j][1] +
				v->position[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i = 0; i < 2; i++)
	{
		bmins[i] = floor(mins[i] / 16);
		bmaxs[i] = ceil(maxs[i] / 16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;
		if (!(tex->flags & TEX_SPECIAL) && s->extents[i] > 256)
			Sys_Error("Bad surface extents"sv);
	}
}


/*
=================
Mod_LoadFaces
=================
*/
void Mod_LoadFaces(lump_t* l)
{
	using namespace std::string_view_literals;
	int i;

	dface_t* in = static_cast<dface_t*>((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		Sys_Error(std::format("MOD_LoadBmodel: funny lump size in {}"sv, loadmodel->name));
	int count = l->filelen / sizeof(*in);
	msurface_t* out = static_cast<msurface_t*>(Hunk_AllocName(count * sizeof*out, loadname));

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	for (int surfnum = 0; surfnum < count; surfnum++, in++, out++)
	{
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);
		out->flags = 0;

		int planenum = LittleShort(in->planenum);
		int side = LittleShort(in->side);
		if (side)
			out->flags |= SURF_PLANEBACK;

		out->plane = loadmodel->planes + planenum;

		out->texinfo = loadmodel->texinfo + LittleShort(in->texinfo);

		CalcSurfaceExtents(out);

		// lighting info

		for (i = 0; i < MAXLIGHTMAPS; i++)
			out->styles[i] = in->styles[i];
		i = LittleLong(in->lightofs);
		if (i == -1)
			out->samples = NULL;
		else
			out->samples = loadmodel->lightdata + i;

		// set the drawing flags flag

		if (!Q_strncmp(out->texinfo->texture->name, (char*)"sky", 3)) // sky
		{
			out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);
			continue;
		}

		if (!Q_strncmp(out->texinfo->texture->name, (char*)"*", 1)) // turbulent
		{
			out->flags |= (SURF_DRAWTURB | SURF_DRAWTILED);
			for (i = 0; i < 2; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
		}
	}
}


/*
=================
Mod_SetParent
=================
*/
void Mod_SetParent(mnode_t* node, mnode_t* parent)
{
	node->parent = parent;
	if (node->contents < 0)
		return;
	Mod_SetParent(node->children[0], node);
	Mod_SetParent(node->children[1], node);
}

/*
=================
Mod_LoadNodes
=================
*/
void Mod_LoadNodes(lump_t* l)
{
	using namespace std::string_view_literals;
	int j;

	dnode_t* in = static_cast<dnode_t*>((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		Sys_Error(std::format("MOD_LoadBmodel: funny lump size in {}"sv, loadmodel->name));
	int count = l->filelen / sizeof(*in);
	mnode_t* out = static_cast<mnode_t*>(Hunk_AllocName(count * sizeof*out, loadname));

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
		}

		int p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;

		out->firstsurface = LittleShort(in->firstface);
		out->numsurfaces = LittleShort(in->numfaces);

		for (j = 0; j < 2; j++)
		{
			p = LittleShort(in->children[j]);
			if (p >= 0)
				out->children[j] = loadmodel->nodes + p;
			else
				out->children[j] = (mnode_t*)(loadmodel->leafs + (-1 - p));
		}
	}

	Mod_SetParent(loadmodel->nodes, NULL); // sets nodes and leafs
}

/*
=================
Mod_LoadLeafs
=================
*/
void Mod_LoadLeafs(lump_t* l)
{
	using namespace std::string_view_literals;
	int j;

	dleaf_t* in = static_cast<dleaf_t*>((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		Sys_Error(std::format("MOD_LoadBmodel: funny lump size in {}"sv, loadmodel->name));
	int count = l->filelen / sizeof(*in);
	mleaf_t* out = static_cast<mleaf_t*>(Hunk_AllocName(count * sizeof*out, loadname));

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
		}

		int p = LittleLong(in->contents);
		out->contents = p;

		out->firstmarksurface = loadmodel->marksurfaces +
			LittleShort(in->firstmarksurface);
		out->nummarksurfaces = LittleShort(in->nummarksurfaces);

		p = LittleLong(in->visofs);
		if (p == -1)
			out->compressed_vis = NULL;
		else
			out->compressed_vis = loadmodel->visdata + p;
		out->efrags = NULL;

		for (j = 0; j < 4; j++)
			out->ambient_sound_level[j] = in->ambient_level[j];
	}
}

/*
=================
Mod_LoadClipnodes
=================
*/
void Mod_LoadClipnodes(lump_t* l)
{
	using namespace std::string_view_literals;
	dclipnode_t* in = static_cast<dclipnode_t*>((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		Sys_Error(std::format("MOD_LoadBmodel: funny lump size in {}"sv, loadmodel->name));
	int count = l->filelen / sizeof(*in);
	dclipnode_t* out = static_cast<dclipnode_t*>(Hunk_AllocName(count * sizeof*out, loadname));

	loadmodel->clipnodes = out;
	loadmodel->numclipnodes = count;

	hull_t* hull = &loadmodel->hulls[1];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 32;

	hull = &loadmodel->hulls[2];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -32;
	hull->clip_mins[1] = -32;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 32;
	hull->clip_maxs[1] = 32;
	hull->clip_maxs[2] = 64;

	for (int i = 0; i < count; i++, out++, in++)
	{
		out->planenum = LittleLong(in->planenum);
		out->children[0] = LittleShort(in->children[0]);
		out->children[1] = LittleShort(in->children[1]);
	}
}

/*
=================
Mod_MakeHull0

Deplicate the drawing hull structure as a clipping hull
=================
*/
void Mod_MakeHull0(void)
{
	hull_t* hull = &loadmodel->hulls[0];

	mnode_t* in = loadmodel->nodes;
	int count = loadmodel->numnodes;
	dclipnode_t* out = static_cast<dclipnode_t*>(Hunk_AllocName(count * sizeof*out, loadname));

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;

	for (int i = 0; i < count; i++, out++, in++)
	{
		out->planenum = in->plane - loadmodel->planes;
		for (int j = 0; j < 2; j++)
		{
			mnode_t* child = in->children[j];
			if (child->contents < 0)
				out->children[j] = child->contents;
			else
				out->children[j] = child - loadmodel->nodes;
		}
	}
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
void Mod_LoadMarksurfaces(lump_t* l)
{
	using namespace std::string_view_literals;
	short* in = static_cast<short*>((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		Sys_Error(std::format("MOD_LoadBmodel: funny lump size in {}"sv, loadmodel->name));
	int count = l->filelen / sizeof(*in);
	msurface_t** out = static_cast<msurface_t**>(Hunk_AllocName(count * sizeof*out, loadname));

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for (int i = 0; i < count; i++)
	{
		int j = LittleShort(in[i]);
		if (j >= loadmodel->numsurfaces)
			Sys_Error("Mod_ParseMarksurfaces: bad surface number"sv);
		out[i] = loadmodel->surfaces + j;
	}
}

/*
=================
Mod_LoadSurfedges
=================
*/
void Mod_LoadSurfedges(lump_t* l)
{
	using namespace std::string_view_literals;
	int* in = static_cast<int*>((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		Sys_Error(std::format("MOD_LoadBmodel: funny lump size in {}"sv, loadmodel->name));
	int count = l->filelen / sizeof(*in);
	int* out = static_cast<int*>(Hunk_AllocName(count * sizeof*out, loadname));

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

	for (int i = 0; i < count; i++)
		out[i] = LittleLong(in[i]);
}

/*
=================
Mod_LoadPlanes
=================
*/
void Mod_LoadPlanes(lump_t* l)
{
	using namespace std::string_view_literals;
	dplane_t* in = static_cast<dplane_t*>((void*)(mod_base + l->fileofs));
	if (l->filelen % sizeof(*in))
		Sys_Error(std::format("MOD_LoadBmodel: funny lump size in {}"sv, loadmodel->name));
	int count = l->filelen / sizeof(*in);
	mplane_t* out = static_cast<mplane_t*>(Hunk_AllocName(count * 2 * sizeof*out, loadname));

	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		int bits = 0;
		for (int j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1 << j;
		}

		out->dist = LittleFloat(in->dist);
		out->type = LittleLong(in->type);
		out->signbits = bits;
	}
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds(vec3_t mins, vec3_t maxs)
{
	vec3_t corner;

	for (int i = 0; i < 3; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return Length(corner);
}

/*
=================
Mod_LoadBrushModel
=================
*/
void Mod_LoadBrushModel(model_t* mod, void* buffer)
{
	using namespace std::string_view_literals;
	loadmodel->type = mod_brush;

	dheader_t* header = (dheader_t*)buffer;

	int i = LittleLong(header->version);
	if (i != BSPVERSION)
		Sys_Error(std::format("Mod_LoadBrushModel: {} has wrong version number ({} should be {})"sv, mod->name, i, BSPVERSION));

	// swap all the lumps
	mod_base = (byte*)header;

	for (i = 0; i < sizeof(dheader_t) / 4; i++)
		((int*)header)[i] = LittleLong(((int*)header)[i]);

	// load into heap

	Mod_LoadVertexes(&header->lumps[LUMP_VERTEXES]);
	Mod_LoadEdges(&header->lumps[LUMP_EDGES]);
	Mod_LoadSurfedges(&header->lumps[LUMP_SURFEDGES]);
	Mod_LoadTextures(&header->lumps[LUMP_TEXTURES]);
	Mod_LoadLighting(&header->lumps[LUMP_LIGHTING]);
	Mod_LoadPlanes(&header->lumps[LUMP_PLANES]);
	Mod_LoadTexinfo(&header->lumps[LUMP_TEXINFO]);
	Mod_LoadFaces(&header->lumps[LUMP_FACES]);
	Mod_LoadMarksurfaces(&header->lumps[LUMP_MARKSURFACES]);
	Mod_LoadVisibility(&header->lumps[LUMP_VISIBILITY]);
	Mod_LoadLeafs(&header->lumps[LUMP_LEAFS]);
	Mod_LoadNodes(&header->lumps[LUMP_NODES]);
	Mod_LoadClipnodes(&header->lumps[LUMP_CLIPNODES]);
	Mod_LoadEntities(&header->lumps[LUMP_ENTITIES]);
	Mod_LoadSubmodels(&header->lumps[LUMP_MODELS]);

	Mod_MakeHull0();

	mod->numframes = 2; // regular and alternate animation
	mod->flags = 0;

	//
	// set up the submodels (FIXME: this_ is confusing)
	//
	for (i = 0; i < mod->numsubmodels; i++)
	{
		dmodel_t* bm = &mod->submodels[i];

		mod->hulls[0].firstclipnode = bm->headnode[0];
		for (int j = 1; j < MAX_MAP_HULLS; j++)
		{
			mod->hulls[j].firstclipnode = bm->headnode[j];
			mod->hulls[j].lastclipnode = mod->numclipnodes - 1;
		}

		mod->firstmodelsurface = bm->firstface;
		mod->nummodelsurfaces = bm->numfaces;

		VectorCopy(bm->maxs, mod->maxs);
		VectorCopy(bm->mins, mod->mins);
		mod->radius = RadiusFromBounds(mod->mins, mod->maxs);

		mod->numleafs = bm->visleafs;

		if (i < mod->numsubmodels - 1)
		{
			// duplicate the basic information
			char name[10];

			sprintf(name, (char*)"*%i", i + 1);
			loadmodel = Mod_FindName(name);
			*loadmodel = *mod;
			strcpy(loadmodel->name, name);
			mod = loadmodel;
		}
	}
}

/*
==============================================================================

ALIAS MODELS

==============================================================================
*/

/*
=================
Mod_LoadAliasFrame
=================
*/
void* Mod_LoadAliasFrame(void* pin, int* pframeindex, int numv,
	trivertx_t* pbboxmin, trivertx_t* pbboxmax, aliashdr_t* pheader, char* name)
{
	daliasframe_t* pdaliasframe = (daliasframe_t*)pin;

	strcpy(name, pdaliasframe->name);

	for (int i = 0; i < 3; i++)
	{
		// these are byte values, so we don't have to worry about
		// endianness
		pbboxmin->v[i] = pdaliasframe->bboxmin.v[i];
		pbboxmax->v[i] = pdaliasframe->bboxmax.v[i];
	}

	trivertx_t* pinframe = (trivertx_t*)(pdaliasframe + 1);
	trivertx_t* pframe = static_cast<trivertx_t*>(Hunk_AllocName(numv * sizeof*pframe, loadname));

	*pframeindex = (byte*)pframe - (byte*)pheader;

	for (int j = 0; j < numv; j++)
	{
		// these are all byte values, so no need to deal with endianness
		pframe[j].lightnormalindex = pinframe[j].lightnormalindex;

		for (int k = 0; k < 3; k++)
		{
			pframe[j].v[k] = pinframe[j].v[k];
		}
	}

	pinframe += numv;

	return pinframe;
}


/*
=================
Mod_LoadAliasGroup
=================
*/
void* Mod_LoadAliasGroup(void* pin, int* pframeindex, int numv,
	trivertx_t* pbboxmin, trivertx_t* pbboxmax, aliashdr_t* pheader, char* name)
{
	using namespace std::string_view_literals;
	int i;

	daliasgroup_t* pingroup = (daliasgroup_t*)pin;

	int numframes = LittleLong(pingroup->numframes);

	maliasgroup_t* paliasgroup = static_cast<maliasgroup_t*>(Hunk_AllocName(sizeof(maliasgroup_t) +
	                                                                        (numframes - 1) * sizeof paliasgroup->frames
	                                                                        [0], loadname));

	paliasgroup->numframes = numframes;

	for (i = 0; i < 3; i++)
	{
		// these are byte values, so we don't have to worry about endianness
		pbboxmin->v[i] = pingroup->bboxmin.v[i];
		pbboxmax->v[i] = pingroup->bboxmax.v[i];
	}

	*pframeindex = (byte*)paliasgroup - (byte*)pheader;

	daliasinterval_t* pin_intervals = (daliasinterval_t*)(pingroup + 1);

	float* poutintervals = static_cast<float*>(Hunk_AllocName(numframes * sizeof(float), loadname));

	paliasgroup->intervals = (byte*)poutintervals - (byte*)pheader;

	for (i = 0; i < numframes; i++)
	{
		*poutintervals = LittleFloat(pin_intervals->interval);
		if (*poutintervals <= 0.0)
			Sys_Error("Mod_LoadAliasGroup: interval<=0"sv);

		poutintervals++;
		pin_intervals++;
	}

	void* ptemp = pin_intervals;

	for (i = 0; i < numframes; i++)
	{
		ptemp = Mod_LoadAliasFrame(ptemp,
			&paliasgroup->frames[i].frame,
			numv,
			&paliasgroup->frames[i].bboxmin,
			&paliasgroup->frames[i].bboxmax,
			pheader, name);
	}

	return ptemp;
}


/*
=================
Mod_LoadAliasSkin
=================
*/
void* Mod_LoadAliasSkin(void* pin, int* pskinindex, int skinsize,
	aliashdr_t* pheader)
{
	using namespace std::string_view_literals;
	byte* pskin = static_cast<byte*>(Hunk_AllocName(skinsize * r_pixbytes, loadname));
	byte* pinskin = (byte*)pin;
	*pskinindex = pskin - (byte*)pheader;

	if (r_pixbytes == 1)
	{
		Q_memcpy(pskin, pinskin, skinsize);
	}
	else if (r_pixbytes == 2)
	{
		unsigned short* pusskin = (unsigned short*)pskin;

		for (int i = 0; i < skinsize; i++)
			pusskin[i] = d_8to16table[pinskin[i]];
	}
	else
	{
		Sys_Error(std::format("Mod_LoadAliasSkin: driver set invalid r_pixbytes: {}\n"sv, r_pixbytes));
	}

	pinskin += skinsize;

	return pinskin;
}


/*
=================
Mod_LoadAliasSkinGroup
=================
*/
void* Mod_LoadAliasSkinGroup(void* pin, int* pskinindex, int skinsize,
	aliashdr_t* pheader)
{
	using namespace std::string_view_literals;
	int i;

	daliasskingroup_t* pinskingroup = (daliasskingroup_t*)pin;

	int numskins = LittleLong(pinskingroup->numskins);

	maliasskingroup_t* paliasskingroup = static_cast<maliasskingroup_t*>(Hunk_AllocName(sizeof(maliasskingroup_t) +
		(numskins - 1) * sizeof paliasskingroup->skindescs[0],
		loadname));

	paliasskingroup->numskins = numskins;

	*pskinindex = (byte*)paliasskingroup - (byte*)pheader;

	daliasskininterval_t* pinskinintervals = (daliasskininterval_t*)(pinskingroup + 1);

	float* poutskinintervals = static_cast<float*>(Hunk_AllocName(numskins * sizeof(float), loadname));

	paliasskingroup->intervals = (byte*)poutskinintervals - (byte*)pheader;

	for (i = 0; i < numskins; i++)
	{
		*poutskinintervals = LittleFloat(pinskinintervals->interval);
		if (*poutskinintervals <= 0)
			Sys_Error("Mod_LoadAliasSkinGroup: interval<=0"sv);

		poutskinintervals++;
		pinskinintervals++;
	}

	void* ptemp = pinskinintervals;

	for (i = 0; i < numskins; i++)
	{
		ptemp = Mod_LoadAliasSkin(ptemp,
			&paliasskingroup->skindescs[i].skin, skinsize, pheader);
	}

	return ptemp;
}


/*
=================
Mod_LoadAliasModel
=================
*/
void Mod_LoadAliasModel(model_t* mod, void* buffer)
{
	using namespace std::string_view_literals;
	int i;
	mdl_t* pmodel, * pinmodel;
	stvert_t* pstverts, * pinstverts;
	aliashdr_t* pheader;
	dtriangle_t* pintriangles;

	int start = Hunk_LowMark();

	pinmodel = (mdl_t*)buffer;

	int version = LittleLong(pinmodel->version);
	if (version != ALIAS_VERSION)
		Sys_Error(std::format("{} has wrong version number ({} should be {})"sv, mod->name, version, ALIAS_VERSION));

	//
	// allocate space for a working header, plus all the data except the frames,
	// skin and group info
	//
	int size = sizeof(aliashdr_t) + (LittleLong(pinmodel->numframes) - 1) *
		sizeof(pheader->frames[0]) +
		sizeof(mdl_t) +
		LittleLong(pinmodel->numverts) * sizeof(stvert_t) +
		LittleLong(pinmodel->numtris) * sizeof(mtriangle_t);

	pheader = static_cast<aliashdr_t*>(Hunk_AllocName(size, loadname));
	pmodel = (mdl_t*)((byte*)&pheader[1] +
		(LittleLong(pinmodel->numframes) - 1) *
		sizeof(pheader->frames[0]));

	// mod->cache.data = pheader;
	mod->flags = LittleLong(pinmodel->flags);

	//
	// endian-adjust and copy the data, starting with the alias model header
	//
	pmodel->boundingradius = LittleFloat(pinmodel->boundingradius);
	pmodel->numskins = LittleLong(pinmodel->numskins);
	pmodel->skinwidth = LittleLong(pinmodel->skinwidth);
	pmodel->skinheight = LittleLong(pinmodel->skinheight);

	if (pmodel->skinheight > MAX_LBM_HEIGHT)
		Sys_Error(std::format("model {} has a skin taller than {}"sv, mod->name, MAX_LBM_HEIGHT));

	pmodel->numverts = LittleLong(pinmodel->numverts);

	if (pmodel->numverts <= 0)
		Sys_Error(std::format("model {} has no vertices"sv, mod->name));

	if (pmodel->numverts > MAXALIASVERTS)
		Sys_Error(std::format("model {} has too many vertices"sv, mod->name));

	pmodel->numtris = LittleLong(pinmodel->numtris);

	if (pmodel->numtris <= 0)
		Sys_Error(std::format("model {} has no triangles"sv, mod->name));

	pmodel->numframes = LittleLong(pinmodel->numframes);
	pmodel->size = LittleFloat(pinmodel->size) * ALIAS_BASE_SIZE_RATIO;
	mod->synctype = (synctype_t)LittleLong(pinmodel->synctype);
	mod->numframes = pmodel->numframes;

	for (i = 0; i < 3; i++)
	{
		pmodel->scale[i] = LittleFloat(pinmodel->scale[i]);
		pmodel->scale_origin[i] = LittleFloat(pinmodel->scale_origin[i]);
		pmodel->eyeposition[i] = LittleFloat(pinmodel->eyeposition[i]);
	}

	int numskins = pmodel->numskins;
	int numframes = pmodel->numframes;

	if (pmodel->skinwidth & 0x03)
		Sys_Error("Mod_LoadAliasModel: skinwidth not multiple of 4"sv);

	pheader->model = (byte*)pmodel - (byte*)pheader;

	//
	// load the skins
	//
	int skinsize = pmodel->skinheight * pmodel->skinwidth;

	if (numskins < 1)
		Sys_Error(std::format("Mod_LoadAliasModel: Invalid # of skins: {}\n"sv, numskins));

	daliasskintype_t* pskintype = (daliasskintype_t*)&pinmodel[1];

	maliasskindesc_t* pskindesc = static_cast<maliasskindesc_t*>(Hunk_AllocName(numskins * sizeof(maliasskindesc_t),
		loadname));

	pheader->skindesc = (byte*)pskindesc - (byte*)pheader;

	for (i = 0; i < numskins; i++)
	{
		aliasskintype_t skintype = (aliasskintype_t)LittleLong(pskintype->type);
		pskindesc[i].type = skintype;

		if (skintype == ALIAS_SKIN_SINGLE)
		{
			pskintype = (daliasskintype_t*)
				Mod_LoadAliasSkin(pskintype + 1,
					&pskindesc[i].skin,
					skinsize, pheader);
		}
		else
		{
			pskintype = (daliasskintype_t*)
				Mod_LoadAliasSkinGroup(pskintype + 1,
					&pskindesc[i].skin,
					skinsize, pheader);
		}
	}

	//
	// set base s and t vertices
	//
	pstverts = (stvert_t*)&pmodel[1];
	pinstverts = (stvert_t*)pskintype;

	pheader->stverts = (byte*)pstverts - (byte*)pheader;

	for (i = 0; i < pmodel->numverts; i++)
	{
		pstverts[i].onseam = LittleLong(pinstverts[i].onseam);
		// put s and t in 16.16 format
		pstverts[i].s = LittleLong(pinstverts[i].s) << 16;
		pstverts[i].t = LittleLong(pinstverts[i].t) << 16;
	}

	//
	// set up the triangles
	//
	mtriangle_t* ptri = (mtriangle_t*)&pstverts[pmodel->numverts];
	pintriangles = (dtriangle_t*)&pinstverts[pmodel->numverts];

	pheader->triangles = (byte*)ptri - (byte*)pheader;

	for (i = 0; i < pmodel->numtris; i++)
	{
		ptri[i].facesfront = LittleLong(pintriangles[i].facesfront);

		for (int j = 0; j < 3; j++)
		{
			ptri[i].vertindex[j] =
				LittleLong(pintriangles[i].vertindex[j]);
		}
	}

	//
	// load the frames
	//
	if (numframes < 1)
		Sys_Error(std::format("Mod_LoadAliasModel: Invalid # of frames: {}\n"sv, numframes));

	daliasframetype_t* pframetype = (daliasframetype_t*)&pintriangles[pmodel->numtris];

	for (i = 0; i < numframes; i++)
	{
		aliasframetype_t frametype = (aliasframetype_t)LittleLong(pframetype->type);
		pheader->frames[i].type = frametype;

		if (frametype == ALIAS_SINGLE)
		{
			pframetype = (daliasframetype_t*)
				Mod_LoadAliasFrame(pframetype + 1,
					&pheader->frames[i].frame,
					pmodel->numverts,
					&pheader->frames[i].bboxmin,
					&pheader->frames[i].bboxmax,
					pheader, pheader->frames[i].name);
		}
		else
		{
			pframetype = (daliasframetype_t*)
				Mod_LoadAliasGroup(pframetype + 1,
					&pheader->frames[i].frame,
					pmodel->numverts,
					&pheader->frames[i].bboxmin,
					&pheader->frames[i].bboxmax,
					pheader, pheader->frames[i].name);
		}
	}

	mod->type = mod_alias;

	// FIXME: do this_ right
	mod->mins[0] = mod->mins[1] = mod->mins[2] = -16;
	mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16;

	//
	// move the complete, relocatable alias model to the cache
	// 
	int end = Hunk_LowMark();
	int total = end - start;

	Cache_Alloc(&mod->cache, total, loadname);
	if (!mod->cache.data)
		return;
	memcpy(mod->cache.data, pheader, total);

	Hunk_FreeToLowMark(start);
}

//=============================================================================

/*
=================
Mod_LoadSpriteFrame
=================
*/
void* Mod_LoadSpriteFrame(void* pin, mspriteframe_t** ppframe)
{
	using namespace std::string_view_literals;
	int origin[2];

	dspriteframe_t* pinframe = (dspriteframe_t*)pin;

	int width = LittleLong(pinframe->width);
	int height = LittleLong(pinframe->height);
	int size = width * height;

	mspriteframe_t* pspriteframe = static_cast<mspriteframe_t*>(Hunk_AllocName(
		sizeof(mspriteframe_t) + size * r_pixbytes,
		loadname));

	Q_memset(pspriteframe, 0, sizeof(mspriteframe_t) + size);
	*ppframe = pspriteframe;

	pspriteframe->width = width;
	pspriteframe->height = height;
	origin[0] = LittleLong(pinframe->origin[0]);
	origin[1] = LittleLong(pinframe->origin[1]);

	pspriteframe->up = origin[1];
	pspriteframe->down = origin[1] - height;
	pspriteframe->left = origin[0];
	pspriteframe->right = width + origin[0];

	if (r_pixbytes == 1)
	{
		Q_memcpy(&pspriteframe->pixels[0], pinframe + 1, size);
	}
	else if (r_pixbytes == 2)
	{
		byte* ppixin = (byte*)(pinframe + 1);
		unsigned short* ppixout = (unsigned short*)&pspriteframe->pixels[0];

		for (int i = 0; i < size; i++)
			ppixout[i] = d_8to16table[ppixin[i]];
	}
	else
	{
		Sys_Error(std::format("Mod_LoadSpriteFrame: driver set invalid r_pixbytes: {}\n"sv, r_pixbytes));
	}

	return (byte*)pinframe + sizeof(dspriteframe_t) + size;
}


/*
=================
Mod_LoadSpriteGroup
=================
*/
void* Mod_LoadSpriteGroup(void* pin, mspriteframe_t** ppframe)
{
	using namespace std::string_view_literals;
	int i;

	dspritegroup_t* pingroup = (dspritegroup_t*)pin;

	int numframes = LittleLong(pingroup->numframes);

	mspritegroup_t* pspritegroup = static_cast<mspritegroup_t*>(Hunk_AllocName(sizeof(mspritegroup_t) +
	                                                                           (numframes - 1) * sizeof pspritegroup->
	                                                                           frames[0], loadname));

	pspritegroup->numframes = numframes;

	*ppframe = (mspriteframe_t*)pspritegroup;

	dspriteinterval_t* pin_intervals = (dspriteinterval_t*)(pingroup + 1);

	float* poutintervals = static_cast<float*>(Hunk_AllocName(numframes * sizeof(float), loadname));

	pspritegroup->intervals = poutintervals;

	for (i = 0; i < numframes; i++)
	{
		*poutintervals = LittleFloat(pin_intervals->interval);
		if (*poutintervals <= 0.0)
			Sys_Error("Mod_LoadSpriteGroup: interval<=0"sv);

		poutintervals++;
		pin_intervals++;
	}

	void* ptemp = pin_intervals;

	for (i = 0; i < numframes; i++)
	{
		ptemp = Mod_LoadSpriteFrame(ptemp, &pspritegroup->frames[i]);
	}

	return ptemp;
}


/*
=================
Mod_LoadSpriteModel
=================
*/
void Mod_LoadSpriteModel(model_t* mod, void* buffer)
{
	using namespace std::string_view_literals;
	msprite_t* psprite;

	dsprite_t* pin = (dsprite_t*)buffer;

	int version = LittleLong(pin->version);
	if (version != SPRITE_VERSION)
		Sys_Error(std::format("{} has wrong version number ({} should be {})"sv, mod->name, version, SPRITE_VERSION));

	int numframes = LittleLong(pin->numframes);

	int size = sizeof(msprite_t) + (numframes - 1) * sizeof(psprite->frames);

	psprite = static_cast<msprite_t*>(Hunk_AllocName(size, loadname));

	mod->cache.data = psprite;

	psprite->type = LittleLong(pin->type);
	psprite->maxwidth = LittleLong(pin->width);
	psprite->maxheight = LittleLong(pin->height);
	psprite->beamlength = LittleFloat(pin->beamlength);
	mod->synctype = (synctype_t)LittleLong(pin->synctype);
	psprite->numframes = numframes;

	mod->mins[0] = mod->mins[1] = -psprite->maxwidth / 2;
	mod->maxs[0] = mod->maxs[1] = psprite->maxwidth / 2;
	mod->mins[2] = -psprite->maxheight / 2;
	mod->maxs[2] = psprite->maxheight / 2;

	//
	// load the frames
	//
	if (numframes < 1)
		Sys_Error(std::format("Mod_LoadSpriteModel: Invalid # of frames: {}\n"sv, numframes));

	mod->numframes = numframes;
	mod->flags = 0;

	dspriteframetype_t* pframetype = (dspriteframetype_t*)(pin + 1);

	for (int i = 0; i < numframes; i++)
	{
		spriteframetype_t frametype = (spriteframetype_t)LittleLong(pframetype->type);
		psprite->frames[i].type = frametype;

		if (frametype == SPR_SINGLE)
		{
			pframetype = (dspriteframetype_t*)
				Mod_LoadSpriteFrame(pframetype + 1,
					&psprite->frames[i].frameptr);
		}
		else
		{
			pframetype = (dspriteframetype_t*)
				Mod_LoadSpriteGroup(pframetype + 1,
					&psprite->frames[i].frameptr);
		}
	}

	mod->type = mod_sprite;
}

//=============================================================================

/*
================
Mod_Print
================
*/
void Mod_Print(void)
{
	int i;
	model_t* mod;

	Con_Printf((char*)"Cached models:\n");
	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		Con_Printf((char*)"%8p : %s", mod->cache.data, mod->name);
		if (mod->needload & NL_UNREFERENCED)
			Con_Printf((char*)" (!R)");
		if (mod->needload & NL_NEEDS_LOADED)
			Con_Printf((char*)" (!P)");
		Con_Printf((char*)"\n");
	}
}
