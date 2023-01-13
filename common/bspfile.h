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
#pragma once

#include "common.h"

// upper design bounds

#define BSPFILE_PUB_MAX_MAP_HULLS 4

#define BSPFILE_PUB_MAX_MAP_MODELS 256
//#define MAX_MAP_BRUSHES 4096
#define BSPFILE_PUB_MAX_MAP_ENTITIES 1024
#define BSPFILE_PUB_MAX_MAP_ENTSTRING 65536

#define BSPFILE_PUB_MAX_MAP_PLANES 32767
#define BSPFILE_PUB_MAX_MAP_NODES 32767 // because negative shorts are contents
#define BSPFILE_PUB_MAX_MAP_CLIPNODES 32767 //
#define BSPFILE_PUB_MAX_MAP_LEAFS 8192
#define BSPFILE_PUB_MAX_MAP_VERTS 65535
#define BSPFILE_PUB_MAX_MAP_FACES 65535
#define BSPFILE_PUB_MAX_MAP_MARKSURFACES 65535
#define BSPFILE_PUB_MAX_MAP_TEXINFO 4096
#define BSPFILE_PUB_MAX_MAP_EDGES 256000
#define BSPFILE_PUB_MAX_MAP_SURFEDGES 512000
//#define MAX_MAP_TEXTURES 512
#define BSPFILE_PUB_MAX_MAP_MIPTEX 0x200000
#define BSPFILE_PUB_MAX_MAP_LIGHTING 0x100000
#define BSPFILE_PUB_MAX_MAP_VISIBILITY 0x100000

//#define MAX_MAP_PORTALS 65536

// key / value pair sizes

//#define MAX_KEY 32
//#define MAX_VALUE 1024

//=============================================================================


#define BSPFILE_PUB_BSPVERSION 29
//#define TOOLVERSION 2

typedef struct
{
	int fileofs, filelen;
} BSPFILE_PUB_lump_t;

#define BSPFILE_PUB_LUMP_ENTITIES 0
#define BSPFILE_PUB_LUMP_PLANES 1
#define BSPFILE_PUB_LUMP_TEXTURES 2
#define BSPFILE_PUB_LUMP_VERTEXES 3
#define BSPFILE_PUB_LUMP_VISIBILITY 4
#define BSPFILE_PUB_LUMP_NODES 5
#define BSPFILE_PUB_LUMP_TEXINFO 6
#define BSPFILE_PUB_LUMP_FACES 7
#define BSPFILE_PUB_LUMP_LIGHTING 8
#define BSPFILE_PUB_LUMP_CLIPNODES 9
#define BSPFILE_PUB_LUMP_LEAFS 10
#define BSPFILE_PUB_LUMP_MARKSURFACES 11
#define BSPFILE_PUB_LUMP_EDGES 12
#define BSPFILE_PUB_LUMP_SURFEDGES 13
#define BSPFILE_PUB_LUMP_MODELS 14

#define BSPFILE_PUB_HEADER_LUMPS 15

typedef struct
{
	float mins[3], maxs[3];
	float origin[3];
	int headnode[BSPFILE_PUB_MAX_MAP_HULLS];
	int visleafs; // not including the solid leaf 0
	int firstface, numfaces;
} BSPFILE_PUB_dmodel_t;

typedef struct
{
	int version;
	BSPFILE_PUB_lump_t lumps[BSPFILE_PUB_HEADER_LUMPS];
} BSPFILE_PUB_dheader_t;

typedef struct
{
	int nummiptex;
	int dataofs[4]; // [nummiptex]
} BSPFILE_PUB_dmiptexlump_t;

#define BSPFILE_PUB_MIPLEVELS 4

typedef struct
{
	char name[16];
	unsigned width, height;
	unsigned offsets[BSPFILE_PUB_MIPLEVELS]; // four mip maps stored
} BSPFILE_PUB_miptex_t;


typedef struct
{
	float point[3];
} BSPFILE_PUB_dvertex_t;


// 0-2 are axial planes
#define BSPFILE_PUB_PLANE_X 0
#define BSPFILE_PUB_PLANE_Y 1
#define BSPFILE_PUB_PLANE_Z 2

// 3-5 are non-axial planes snapped to the nearest
#define BSPFILE_PUB_PLANE_ANYX 3
#define BSPFILE_PUB_PLANE_ANYY 4
#define BSPFILE_PUB_PLANE_ANYZ 5

typedef struct
{
	float normal[3];
	float dist;
	int type; // BSPFILE_PUB_PLANE_X - BSPFILE_PUB_PLANE_ANYZ ?remove? trivial to regenerate
} BSPFILE_PUB_dplane_t;


#define SPFILE_PUB_CONTENTS_EMPTY -1
#define SPFILE_PUB_CONTENTS_SOLID -2
#define SPFILE_PUB_CONTENTS_WATER -3
#define SPFILE_PUB_CONTENTS_SLIME -4
#define SPFILE_PUB_CONTENTS_LAVA -5
#define SPFILE_PUB_CONTENTS_SKY -6
#define SPFILE_PUB_CONTENTS_ORIGIN -7 // removed at csg time
#define SPFILE_PUB_CONTENTS_CLIP -8 // changed to contents_solid

#define SPFILE_PUB_CONTENTS_CURRENT_0 -9
#define SPFILE_PUB_CONTENTS_CURRENT_90 -10
#define SPFILE_PUB_CONTENTS_CURRENT_180 -11
#define SPFILE_PUB_CONTENTS_CURRENT_270 -12
#define SPFILE_PUB_CONTENTS_CURRENT_UP -13
#define SPFILE_PUB_CONTENTS_CURRENT_DOWN -14


// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct
{
	int planenum;
	short children[2]; // negative numbers are -(leafs+1), not nodes
	short mins[3]; // for sphere culling
	short maxs[3];
	unsigned short firstface;
	unsigned short numfaces; // counting both sides
} BSPFILE_PUB_dnode_t;

typedef struct
{
	int planenum;
	short children[2]; // negative numbers are contents
} BSPFILE_PUB_dclipnode_t;


typedef struct
{
	float vecs[2][4]; // [s/t][xyz offset]
	int miptex;
	int flags;
} BSPFILE_PUB_texinfo_t;

#define BSPFILE_PUB_TEX_SPECIAL 1 // sky or slime, no lightmap or 256 subdivision

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct
{
	unsigned short v[2]; // vertex numbers
} BSPFILE_PUB_dedge_t;

#define BSPFILE_PUB_MAXLIGHTMAPS 4

typedef struct
{
	short planenum;
	short side;

	int firstedge; // we must support > 64k edges
	short numedges;
	short texinfo;

	// lighting info
	byte styles[BSPFILE_PUB_MAXLIGHTMAPS];
	int lightofs; // start of [numstyles*surfsize] samples
} BSPFILE_PUB_dface_t;


#define BSPFILE_PUB_AMBIENT_WATER 0
#define BSPFILE_PUB_AMBIENT_SKY 1
#define BSPFILE_PUB_AMBIENT_SLIME 2
#define BSPFILE_PUB_AMBIENT_LAVA 3

#define BSPFILE_PUB_NUM_AMBIENTS 4 // automatic ambient sounds

// leaf 0 is the generic SPFILE_PUB_CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
typedef struct
{
	int contents;
	int visofs; // -1 = no visibility info

	short mins[3]; // for frustum culling
	short maxs[3];

	unsigned short firstmarksurface;
	unsigned short nummarksurfaces;

	byte ambient_level[BSPFILE_PUB_NUM_AMBIENTS];
} BSPFILE_PUB_dleaf_t;


//============================================================================
/*
#ifndef QUAKE_GAME

#define ANGLE_UP -1
#define ANGLE_DOWN -2


// the utilities get to be lazy and just use large static arrays

extern int nummodels;
extern BSPFILE_PUB_dmodel_t dmodels[BSPFILE_PUB_MAX_MAP_MODELS];

extern int visdatasize;
extern byte dvisdata[BSPFILE_PUB_MAX_MAP_VISIBILITY];

extern int lightdatasize;
extern byte dlightdata[BSPFILE_PUB_MAX_MAP_LIGHTING];

extern int texdatasize;
extern byte dtexdata[BSPFILE_PUB_MAX_MAP_MIPTEX]; // (BSPFILE_PUB_dmiptexlump_t)

extern int entdatasize;
extern char dentdata[BSPFILE_PUB_MAX_MAP_ENTSTRING];

extern int numleafs;
extern BSPFILE_PUB_dleaf_t dleafs[BSPFILE_PUB_MAX_MAP_LEAFS];

extern int numplanes;
extern BSPFILE_PUB_dplane_t dplanes[BSPFILE_PUB_MAX_MAP_PLANES];

extern int numvertexes;
extern BSPFILE_PUB_dvertex_t dvertexes[BSPFILE_PUB_MAX_MAP_VERTS];

extern int numnodes;
extern BSPFILE_PUB_dnode_t dnodes[BSPFILE_PUB_MAX_MAP_NODES];

extern int numtexinfo;
extern BSPFILE_PUB_texinfo_t texinfo[BSPFILE_PUB_MAX_MAP_TEXINFO];

extern int numfaces;
extern BSPFILE_PUB_dface_t dfaces[BSPFILE_PUB_MAX_MAP_FACES];

extern int numclipnodes;
extern BSPFILE_PUB_dclipnode_t dclipnodes[BSPFILE_PUB_MAX_MAP_CLIPNODES];

extern int numedges;
extern BSPFILE_PUB_dedge_t dedges[BSPFILE_PUB_MAX_MAP_EDGES];

extern int nummarksurfaces;
extern unsigned short dmarksurfaces[BSPFILE_PUB_MAX_MAP_MARKSURFACES];

extern int numsurfedges;
extern int dsurfedges[BSPFILE_PUB_MAX_MAP_SURFEDGES];


void DecompressVis(byte* in, byte* decompressed);
int CompressVis(byte* vis, byte* dest);

void LoadBSPFile(char* filename);
void WriteBSPFile(char* filename);
void PrintBSPFileSizes(void);

//===============


typedef struct epair_s
{
	struct epair_s* next;
	char* key;
	char* value;
} epair_t;

typedef struct
{
	vec3_t origin;
	int firstbrush;
	int numbrushes;
	epair_t* epairs;
} entity_t;

extern int num_entities;
extern entity_t entities[BSPFILE_PUB_MAX_MAP_ENTITIES];

void ParseEntities(void);
void UnparseEntities(void);

void  SetKeyValue(entity_t* ent, char* key, char* value);
char* ValueForKey(entity_t* ent, char* key);
// will return "" if not present

vec_t FloatForKey(entity_t* ent, char* key);
void  GetVectorForKey(entity_t* ent, char* key, vec3_t vec);

epair_t* ParseEpair(void);

#endif
*/