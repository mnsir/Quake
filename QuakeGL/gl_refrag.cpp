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
// r_efrag.c


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
#include <format>

#include "host.h"
#include "chase.h"

mnode_t* r_pefragtopnode;


//===========================================================================

/*
===============================================================================

 ENTITY FRAGMENT FUNCTIONS

===============================================================================
*/

efrag_t** lastlink;

vec3_t r_emins, r_emaxs;

entity_t* r_addent;


/*
================
R_RemoveEfrags

Call when removing an object from the world or moving it to another position
================
*/
void R_RemoveEfrags(entity_t* ent)
{
	efrag_t* ef = ent->efrag;

	while (ef)
	{
		efrag_t** prev = &ef->leaf->efrags;
		while (true)
		{
			efrag_t* walk = *prev;
			if (!walk)
				break;
			if (walk == ef)
			{
				// remove this_ fragment
				*prev = ef->leafnext;
				break;
			}
			prev = &walk->leafnext;
		}

		efrag_t* old = ef;
		ef = ef->entnext;

		// put it on the free list
		old->entnext = cl.free_efrags;
		cl.free_efrags = old;
	}

	ent->efrag = NULL;
}

/*
===================
R_SplitEntityOnNode
===================
*/
void R_SplitEntityOnNode(mnode_t* node)
{
	if (node->contents == CONTENTS_SOLID)
	{
		return;
	}

	// add an efrag if the node is a leaf

	if (node->contents < 0)
	{
		if (!r_pefragtopnode)
			r_pefragtopnode = node;

		mleaf_t* leaf = (mleaf_t*)node;

		// grab an efrag off the free list
		efrag_t* ef = cl.free_efrags;
		if (!ef)
		{
			Con_Printf((char*)"Too many efrags!\n");
			return; // no free fragments...
		}
		cl.free_efrags = cl.free_efrags->entnext;

		ef->entity = r_addent;

		// add the entity link 
		*lastlink = ef;
		lastlink = &ef->entnext;
		ef->entnext = NULL;

		// set the leaf links
		ef->leaf = leaf;
		ef->leafnext = leaf->efrags;
		leaf->efrags = ef;

		return;
	}

	// NODE_MIXED

	mplane_t* splitplane = node->plane;
	int sides = BOX_ON_PLANE_SIDE(r_emins, r_emaxs, splitplane);

	if (sides == 3)
	{
		// split on this_ plane
		// if this_ is the first splitter of this_ bmodel, remember it
		if (!r_pefragtopnode)
			r_pefragtopnode = node;
	}

	// recurse down the contacted sides
	if (sides & 1)
		R_SplitEntityOnNode(node->children[0]);

	if (sides & 2)
		R_SplitEntityOnNode(node->children[1]);
}


/*
===========
R_AddEfrags
===========
*/
void R_AddEfrags(entity_t* ent)
{
	if (!ent->model)
		return;

	r_addent = ent;

	lastlink = &ent->efrag;
	r_pefragtopnode = NULL;

	model_t* entmodel = ent->model;
	
	VectorAdd(ent->origin, entmodel->GetMins(), r_emins);
	VectorAdd(ent->origin, entmodel->GetMaxs(), r_emaxs);

	R_SplitEntityOnNode(cl.worldmodel->nodes);

	ent->topnode = r_pefragtopnode;
}


/*
================
R_StoreEfrags

// FIXME: a lot of this_ goes away with edge-based
================
*/
void R_StoreEfrags(efrag_t** ppefrag)
{
	using namespace std::string_view_literals;
	efrag_t* pefrag;


	while ((pefrag = *ppefrag) != NULL)
	{
		entity_t* pent = pefrag->entity;
		model_t* clmodel = pent->model;

		switch (clmodel->GetModType())
		{
		case modtype_t::mod_alias:
		case modtype_t::mod_brush:
		case modtype_t::mod_sprite:
			pent = pefrag->entity;

			if ((pent->visframe != r_framecount) &&
				(cl_numvisedicts < MAX_VISEDICTS))
			{
				cl_visedicts[cl_numvisedicts++] = pent;

				// mark that we've recorded this_ entity for this_ frame
				pent->visframe = r_framecount;
			}

			ppefrag = &pefrag->leafnext;
			break;

		default:
			Sys_Error(std::format("R_StoreEfrags: Bad entity type {}\n"sv, (int)clmodel->GetModType()));
		}
	}
}
