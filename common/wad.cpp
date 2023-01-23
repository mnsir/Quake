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
// wad.c

#include "wad.h"

#include <format>

#include "common.h"
#include "sys.h"


int wad_numlumps;
lumpinfo_t* wad_lumps;
byte* wad_base;

void SwapPic(qpic_t* pic);

/*
==================
W_CleanupName

Lowercases name and pads with spaces and a terminating 0 to the length of
lumpinfo_t->name.
Used so lumpname lookups can proceed rapidly by comparing 4 chars at a time
Space padding is so names can be printed nicely in tables.
Can safely be performed in place.
==================
*/
void W_CleanupName(char* in, char* out)
{
	int i;

	for (i = 0; i < 16; i++)
	{
		int c = in[i];
		if (!c)
			break;

		if (c >= 'A' && c <= 'Z')
			c += ('a' - 'A');
		out[i] = c;
	}

	for (; i < 16; i++)
		out[i] = 0;
}


/*
====================
W_LoadWadFile
====================
*/
void W_LoadWadFile(char* filename)
{
	using namespace std::string_view_literals;
	lumpinfo_t* lump_p;
	unsigned i;

	wad_base = COM_LoadHunkFile(filename);
	if (!wad_base)
		Sys_Error(std::format("W_LoadWadFile: couldn't load {}"sv, filename));

	wadinfo_t* header = (wadinfo_t*)wad_base;

	if (header->identification[0] != 'W'
		|| header->identification[1] != 'A'
		|| header->identification[2] != 'D'
		|| header->identification[3] != '2')
		Sys_Error(std::format("Wad file {} doesn't have WAD2 id\n"sv, filename));

	wad_numlumps = LittleLong(header->numlumps);
	int infotableofs = LittleLong(header->infotableofs);
	wad_lumps = (lumpinfo_t*)(wad_base + infotableofs);

	for (i = 0, lump_p = wad_lumps; i < wad_numlumps; i++, lump_p++)
	{
		lump_p->filepos = LittleLong(lump_p->filepos);
		lump_p->size = LittleLong(lump_p->size);
		W_CleanupName(lump_p->name, lump_p->name);
		if (lump_p->type == TYP_QPIC)
			SwapPic((qpic_t*)(wad_base + lump_p->filepos));
	}
}


/*
=============
W_GetLumpinfo
=============
*/
lumpinfo_t* W_GetLumpinfo(char* name)
{
	using namespace std::string_view_literals;
	int i;
	lumpinfo_t* lump_p;
	char clean[16];

	W_CleanupName(name, clean);

	for (lump_p = wad_lumps, i = 0; i < wad_numlumps; i++, lump_p++)
	{
		if (!strcmp(clean, lump_p->name))
			return lump_p;
	}

	Sys_Error(std::format("W_GetLumpinfo: {} not found"sv, name));
	return NULL;
}

void* W_GetLumpName(char* name)
{
	lumpinfo_t* lump = W_GetLumpinfo(name);

	return wad_base + lump->filepos;
}

void* W_GetLumpNum(int num)
{
	using namespace std::string_view_literals;
	if (num < 0 || num > wad_numlumps)
		Sys_Error(std::format("W_GetLumpNum: bad number: {}"sv, num));

	lumpinfo_t* lump = wad_lumps + num;

	return wad_base + lump->filepos;
}

/*
=============================================================================

automatic byte swapping

=============================================================================
*/

void SwapPic(qpic_t* pic)
{
	pic->width = LittleLong(pic->width);
	pic->height = LittleLong(pic->height);
}
