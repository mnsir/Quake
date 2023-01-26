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
// d_polyset.c: routines for drawing sets of polygons sharing the same
// texture (used for Alias models)

#include <cmath>

#include "d_iface.h"
#include "d_local.h"
#include <cstdint>

#include "sys.h"
#include <format>


// TODO: put in span spilling to shrink list size
// !!! if this_ is changed, it must be changed in d_polysa.s too !!!
#define DPS_MAXSPANS MAXHEIGHT+1
// 1 extra for spanpackage that marks end

// !!! if this_ is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	void* pdest;
	short* pz;
	int count;
	byte* ptex;
	int sfrac, tfrac, light, zi;
} spanpackage_t;

typedef struct
{
	int isflattop;
	int numleftedges;
	int* pleftedgevert0;
	int* pleftedgevert1;
	int* pleftedgevert2;
	int numrightedges;
	int* prightedgevert0;
	int* prightedgevert1;
	int* prightedgevert2;
} edgetable;

int r_p0[6], r_p1[6], r_p2[6];

uint8_t* d_pcolormap;

int d_aflatcolor;
int d_xdenom;

edgetable* pedgetable;

edgetable edgetables[12] = {
	{0, 1, r_p0, r_p2, NULL, 2, r_p0, r_p1, r_p2},
	{0, 2, r_p1, r_p0, r_p2, 1, r_p1, r_p2, NULL},
	{1, 1, r_p0, r_p2, NULL, 1, r_p1, r_p2, NULL},
	{0, 1, r_p1, r_p0, NULL, 2, r_p1, r_p2, r_p0},
	{0, 2, r_p0, r_p2, r_p1, 1, r_p0, r_p1, NULL},
	{0, 1, r_p2, r_p1, NULL, 1, r_p2, r_p0, NULL},
	{0, 1, r_p2, r_p1, NULL, 2, r_p2, r_p0, r_p1},
	{0, 2, r_p2, r_p1, r_p0, 1, r_p2, r_p0, NULL},
	{0, 1, r_p1, r_p0, NULL, 1, r_p1, r_p2, NULL},
	{1, 1, r_p2, r_p1, NULL, 1, r_p0, r_p1, NULL},
	{1, 1, r_p1, r_p0, NULL, 1, r_p2, r_p0, NULL},
	{0, 1, r_p0, r_p2, NULL, 1, r_p0, r_p1, NULL},
};

// FIXME: some of these can become statics
int a_sstepxfrac, a_tstepxfrac, r_lstepx, a_ststepxwhole;
int r_sstepx, r_tstepx, r_lstepy, r_sstepy, r_tstepy;
int r_zistepx, r_zistepy;
int d_aspancount, d_countextrastep;

spanpackage_t* a_spans;
spanpackage_t* d_pedgespanpackage;
static int ystart;
uint8_t*d_pdest, *d_ptex;
short* d_pz;
int d_sfrac, d_tfrac, d_light, d_zi;
int d_ptexextrastep, d_sfracextrastep;
int d_tfracextrastep, d_lightextrastep, d_pdestextrastep;
int d_lightbasestep, d_pdestbasestep, d_ptexbasestep;
int d_sfracbasestep, d_tfracbasestep;
int d_ziextrastep, d_zibasestep;
int d_pzextrastep, d_pzbasestep;

typedef struct
{
	int quotient;
	int remainder;
} adivtab_t;

// table of quotients and remainders for [-15...16] / [-15...16]
static adivtab_t adivtab[32 * 32] = {
	// numerator = -15
	{1, 0},
	{1, -1},
	{1, -2},
	{1, -3},
	{1, -4},
	{1, -5},
	{1, -6},
	{1, -7},
	{2, -1},
	{2, -3},
	{3, 0},
	{3, -3},
	{5, 0},
	{7, -1},
	{15, 0},
	{0, 0},
	{-15, 0},
	{-8, 1},
	{-5, 0},
	{-4, 1},
	{-3, 0},
	{-3, 3},
	{-3, 6},
	{-2, 1},
	{-2, 3},
	{-2, 5},
	{-2, 7},
	{-2, 9},
	{-2, 11},
	{-2, 13},
	{-1, 0},
	{-1, 1},
	// numerator = -14
	{0, -14},
	{1, 0},
	{1, -1},
	{1, -2},
	{1, -3},
	{1, -4},
	{1, -5},
	{1, -6},
	{2, 0},
	{2, -2},
	{2, -4},
	{3, -2},
	{4, -2},
	{7, 0},
	{14, 0},
	{0, 0},
	{-14, 0},
	{-7, 0},
	{-5, 1},
	{-4, 2},
	{-3, 1},
	{-3, 4},
	{-2, 0},
	{-2, 2},
	{-2, 4},
	{-2, 6},
	{-2, 8},
	{-2, 10},
	{-2, 12},
	{-1, 0},
	{-1, 1},
	{-1, 2},
	// numerator = -13
	{0, -13},
	{0, -13},
	{1, 0},
	{1, -1},
	{1, -2},
	{1, -3},
	{1, -4},
	{1, -5},
	{1, -6},
	{2, -1},
	{2, -3},
	{3, -1},
	{4, -1},
	{6, -1},
	{13, 0},
	{0, 0},
	{-13, 0},
	{-7, 1},
	{-5, 2},
	{-4, 3},
	{-3, 2},
	{-3, 5},
	{-2, 1},
	{-2, 3},
	{-2, 5},
	{-2, 7},
	{-2, 9},
	{-2, 11},
	{-1, 0},
	{-1, 1},
	{-1, 2},
	{-1, 3},
	// numerator = -12
	{0, -12},
	{0, -12},
	{0, -12},
	{1, 0},
	{1, -1},
	{1, -2},
	{1, -3},
	{1, -4},
	{1, -5},
	{2, 0},
	{2, -2},
	{3, 0},
	{4, 0},
	{6, 0},
	{12, 0},
	{0, 0},
	{-12, 0},
	{-6, 0},
	{-4, 0},
	{-3, 0},
	{-3, 3},
	{-2, 0},
	{-2, 2},
	{-2, 4},
	{-2, 6},
	{-2, 8},
	{-2, 10},
	{-1, 0},
	{-1, 1},
	{-1, 2},
	{-1, 3},
	{-1, 4},
	// numerator = -11
	{0, -11},
	{0, -11},
	{0, -11},
	{0, -11},
	{1, 0},
	{1, -1},
	{1, -2},
	{1, -3},
	{1, -4},
	{1, -5},
	{2, -1},
	{2, -3},
	{3, -2},
	{5, -1},
	{11, 0},
	{0, 0},
	{-11, 0},
	{-6, 1},
	{-4, 1},
	{-3, 1},
	{-3, 4},
	{-2, 1},
	{-2, 3},
	{-2, 5},
	{-2, 7},
	{-2, 9},
	{-1, 0},
	{-1, 1},
	{-1, 2},
	{-1, 3},
	{-1, 4},
	{-1, 5},
	// numerator = -10
	{0, -10},
	{0, -10},
	{0, -10},
	{0, -10},
	{0, -10},
	{1, 0},
	{1, -1},
	{1, -2},
	{1, -3},
	{1, -4},
	{2, 0},
	{2, -2},
	{3, -1},
	{5, 0},
	{10, 0},
	{0, 0},
	{-10, 0},
	{-5, 0},
	{-4, 2},
	{-3, 2},
	{-2, 0},
	{-2, 2},
	{-2, 4},
	{-2, 6},
	{-2, 8},
	{-1, 0},
	{-1, 1},
	{-1, 2},
	{-1, 3},
	{-1, 4},
	{-1, 5},
	{-1, 6},
	// numerator = -9
	{0, -9},
	{0, -9},
	{0, -9},
	{0, -9},
	{0, -9},
	{0, -9},
	{1, 0},
	{1, -1},
	{1, -2},
	{1, -3},
	{1, -4},
	{2, -1},
	{3, 0},
	{4, -1},
	{9, 0},
	{0, 0},
	{-9, 0},
	{-5, 1},
	{-3, 0},
	{-3, 3},
	{-2, 1},
	{-2, 3},
	{-2, 5},
	{-2, 7},
	{-1, 0},
	{-1, 1},
	{-1, 2},
	{-1, 3},
	{-1, 4},
	{-1, 5},
	{-1, 6},
	{-1, 7},
	// numerator = -8
	{0, -8},
	{0, -8},
	{0, -8},
	{0, -8},
	{0, -8},
	{0, -8},
	{0, -8},
	{1, 0},
	{1, -1},
	{1, -2},
	{1, -3},
	{2, 0},
	{2, -2},
	{4, 0},
	{8, 0},
	{0, 0},
	{-8, 0},
	{-4, 0},
	{-3, 1},
	{-2, 0},
	{-2, 2},
	{-2, 4},
	{-2, 6},
	{-1, 0},
	{-1, 1},
	{-1, 2},
	{-1, 3},
	{-1, 4},
	{-1, 5},
	{-1, 6},
	{-1, 7},
	{-1, 8},
	// numerator = -7
	{0, -7},
	{0, -7},
	{0, -7},
	{0, -7},
	{0, -7},
	{0, -7},
	{0, -7},
	{0, -7},
	{1, 0},
	{1, -1},
	{1, -2},
	{1, -3},
	{2, -1},
	{3, -1},
	{7, 0},
	{0, 0},
	{-7, 0},
	{-4, 1},
	{-3, 2},
	{-2, 1},
	{-2, 3},
	{-2, 5},
	{-1, 0},
	{-1, 1},
	{-1, 2},
	{-1, 3},
	{-1, 4},
	{-1, 5},
	{-1, 6},
	{-1, 7},
	{-1, 8},
	{-1, 9},
	// numerator = -6
	{0, -6},
	{0, -6},
	{0, -6},
	{0, -6},
	{0, -6},
	{0, -6},
	{0, -6},
	{0, -6},
	{0, -6},
	{1, 0},
	{1, -1},
	{1, -2},
	{2, 0},
	{3, 0},
	{6, 0},
	{0, 0},
	{-6, 0},
	{-3, 0},
	{-2, 0},
	{-2, 2},
	{-2, 4},
	{-1, 0},
	{-1, 1},
	{-1, 2},
	{-1, 3},
	{-1, 4},
	{-1, 5},
	{-1, 6},
	{-1, 7},
	{-1, 8},
	{-1, 9},
	{-1, 10},
	// numerator = -5
	{0, -5},
	{0, -5},
	{0, -5},
	{0, -5},
	{0, -5},
	{0, -5},
	{0, -5},
	{0, -5},
	{0, -5},
	{0, -5},
	{1, 0},
	{1, -1},
	{1, -2},
	{2, -1},
	{5, 0},
	{0, 0},
	{-5, 0},
	{-3, 1},
	{-2, 1},
	{-2, 3},
	{-1, 0},
	{-1, 1},
	{-1, 2},
	{-1, 3},
	{-1, 4},
	{-1, 5},
	{-1, 6},
	{-1, 7},
	{-1, 8},
	{-1, 9},
	{-1, 10},
	{-1, 11},
	// numerator = -4
	{0, -4},
	{0, -4},
	{0, -4},
	{0, -4},
	{0, -4},
	{0, -4},
	{0, -4},
	{0, -4},
	{0, -4},
	{0, -4},
	{0, -4},
	{1, 0},
	{1, -1},
	{2, 0},
	{4, 0},
	{0, 0},
	{-4, 0},
	{-2, 0},
	{-2, 2},
	{-1, 0},
	{-1, 1},
	{-1, 2},
	{-1, 3},
	{-1, 4},
	{-1, 5},
	{-1, 6},
	{-1, 7},
	{-1, 8},
	{-1, 9},
	{-1, 10},
	{-1, 11},
	{-1, 12},
	// numerator = -3
	{0, -3},
	{0, -3},
	{0, -3},
	{0, -3},
	{0, -3},
	{0, -3},
	{0, -3},
	{0, -3},
	{0, -3},
	{0, -3},
	{0, -3},
	{0, -3},
	{1, 0},
	{1, -1},
	{3, 0},
	{0, 0},
	{-3, 0},
	{-2, 1},
	{-1, 0},
	{-1, 1},
	{-1, 2},
	{-1, 3},
	{-1, 4},
	{-1, 5},
	{-1, 6},
	{-1, 7},
	{-1, 8},
	{-1, 9},
	{-1, 10},
	{-1, 11},
	{-1, 12},
	{-1, 13},
	// numerator = -2
	{0, -2},
	{0, -2},
	{0, -2},
	{0, -2},
	{0, -2},
	{0, -2},
	{0, -2},
	{0, -2},
	{0, -2},
	{0, -2},
	{0, -2},
	{0, -2},
	{0, -2},
	{1, 0},
	{2, 0},
	{0, 0},
	{-2, 0},
	{-1, 0},
	{-1, 1},
	{-1, 2},
	{-1, 3},
	{-1, 4},
	{-1, 5},
	{-1, 6},
	{-1, 7},
	{-1, 8},
	{-1, 9},
	{-1, 10},
	{-1, 11},
	{-1, 12},
	{-1, 13},
	{-1, 14},
	// numerator = -1
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
	{1, 0},
	{0, 0},
	{-1, 0},
	{-1, 1},
	{-1, 2},
	{-1, 3},
	{-1, 4},
	{-1, 5},
	{-1, 6},
	{-1, 7},
	{-1, 8},
	{-1, 9},
	{-1, 10},
	{-1, 11},
	{-1, 12},
	{-1, 13},
	{-1, 14},
	{-1, 15},
	// numerator = 0
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	// numerator = 1
	{-1, -14},
	{-1, -13},
	{-1, -12},
	{-1, -11},
	{-1, -10},
	{-1, -9},
	{-1, -8},
	{-1, -7},
	{-1, -6},
	{-1, -5},
	{-1, -4},
	{-1, -3},
	{-1, -2},
	{-1, -1},
	{-1, 0},
	{0, 0},
	{1, 0},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
	{0, 1},
	// numerator = 2
	{-1, -13},
	{-1, -12},
	{-1, -11},
	{-1, -10},
	{-1, -9},
	{-1, -8},
	{-1, -7},
	{-1, -6},
	{-1, -5},
	{-1, -4},
	{-1, -3},
	{-1, -2},
	{-1, -1},
	{-1, 0},
	{-2, 0},
	{0, 0},
	{2, 0},
	{1, 0},
	{0, 2},
	{0, 2},
	{0, 2},
	{0, 2},
	{0, 2},
	{0, 2},
	{0, 2},
	{0, 2},
	{0, 2},
	{0, 2},
	{0, 2},
	{0, 2},
	{0, 2},
	{0, 2},
	// numerator = 3
	{-1, -12},
	{-1, -11},
	{-1, -10},
	{-1, -9},
	{-1, -8},
	{-1, -7},
	{-1, -6},
	{-1, -5},
	{-1, -4},
	{-1, -3},
	{-1, -2},
	{-1, -1},
	{-1, 0},
	{-2, -1},
	{-3, 0},
	{0, 0},
	{3, 0},
	{1, 1},
	{1, 0},
	{0, 3},
	{0, 3},
	{0, 3},
	{0, 3},
	{0, 3},
	{0, 3},
	{0, 3},
	{0, 3},
	{0, 3},
	{0, 3},
	{0, 3},
	{0, 3},
	{0, 3},
	// numerator = 4
	{-1, -11},
	{-1, -10},
	{-1, -9},
	{-1, -8},
	{-1, -7},
	{-1, -6},
	{-1, -5},
	{-1, -4},
	{-1, -3},
	{-1, -2},
	{-1, -1},
	{-1, 0},
	{-2, -2},
	{-2, 0},
	{-4, 0},
	{0, 0},
	{4, 0},
	{2, 0},
	{1, 1},
	{1, 0},
	{0, 4},
	{0, 4},
	{0, 4},
	{0, 4},
	{0, 4},
	{0, 4},
	{0, 4},
	{0, 4},
	{0, 4},
	{0, 4},
	{0, 4},
	{0, 4},
	// numerator = 5
	{-1, -10},
	{-1, -9},
	{-1, -8},
	{-1, -7},
	{-1, -6},
	{-1, -5},
	{-1, -4},
	{-1, -3},
	{-1, -2},
	{-1, -1},
	{-1, 0},
	{-2, -3},
	{-2, -1},
	{-3, -1},
	{-5, 0},
	{0, 0},
	{5, 0},
	{2, 1},
	{1, 2},
	{1, 1},
	{1, 0},
	{0, 5},
	{0, 5},
	{0, 5},
	{0, 5},
	{0, 5},
	{0, 5},
	{0, 5},
	{0, 5},
	{0, 5},
	{0, 5},
	{0, 5},
	// numerator = 6
	{-1, -9},
	{-1, -8},
	{-1, -7},
	{-1, -6},
	{-1, -5},
	{-1, -4},
	{-1, -3},
	{-1, -2},
	{-1, -1},
	{-1, 0},
	{-2, -4},
	{-2, -2},
	{-2, 0},
	{-3, 0},
	{-6, 0},
	{0, 0},
	{6, 0},
	{3, 0},
	{2, 0},
	{1, 2},
	{1, 1},
	{1, 0},
	{0, 6},
	{0, 6},
	{0, 6},
	{0, 6},
	{0, 6},
	{0, 6},
	{0, 6},
	{0, 6},
	{0, 6},
	{0, 6},
	// numerator = 7
	{-1, -8},
	{-1, -7},
	{-1, -6},
	{-1, -5},
	{-1, -4},
	{-1, -3},
	{-1, -2},
	{-1, -1},
	{-1, 0},
	{-2, -5},
	{-2, -3},
	{-2, -1},
	{-3, -2},
	{-4, -1},
	{-7, 0},
	{0, 0},
	{7, 0},
	{3, 1},
	{2, 1},
	{1, 3},
	{1, 2},
	{1, 1},
	{1, 0},
	{0, 7},
	{0, 7},
	{0, 7},
	{0, 7},
	{0, 7},
	{0, 7},
	{0, 7},
	{0, 7},
	{0, 7},
	// numerator = 8
	{-1, -7},
	{-1, -6},
	{-1, -5},
	{-1, -4},
	{-1, -3},
	{-1, -2},
	{-1, -1},
	{-1, 0},
	{-2, -6},
	{-2, -4},
	{-2, -2},
	{-2, 0},
	{-3, -1},
	{-4, 0},
	{-8, 0},
	{0, 0},
	{8, 0},
	{4, 0},
	{2, 2},
	{2, 0},
	{1, 3},
	{1, 2},
	{1, 1},
	{1, 0},
	{0, 8},
	{0, 8},
	{0, 8},
	{0, 8},
	{0, 8},
	{0, 8},
	{0, 8},
	{0, 8},
	// numerator = 9
	{-1, -6},
	{-1, -5},
	{-1, -4},
	{-1, -3},
	{-1, -2},
	{-1, -1},
	{-1, 0},
	{-2, -7},
	{-2, -5},
	{-2, -3},
	{-2, -1},
	{-3, -3},
	{-3, 0},
	{-5, -1},
	{-9, 0},
	{0, 0},
	{9, 0},
	{4, 1},
	{3, 0},
	{2, 1},
	{1, 4},
	{1, 3},
	{1, 2},
	{1, 1},
	{1, 0},
	{0, 9},
	{0, 9},
	{0, 9},
	{0, 9},
	{0, 9},
	{0, 9},
	{0, 9},
	// numerator = 10
	{-1, -5},
	{-1, -4},
	{-1, -3},
	{-1, -2},
	{-1, -1},
	{-1, 0},
	{-2, -8},
	{-2, -6},
	{-2, -4},
	{-2, -2},
	{-2, 0},
	{-3, -2},
	{-4, -2},
	{-5, 0},
	{-10, 0},
	{0, 0},
	{10, 0},
	{5, 0},
	{3, 1},
	{2, 2},
	{2, 0},
	{1, 4},
	{1, 3},
	{1, 2},
	{1, 1},
	{1, 0},
	{0, 10},
	{0, 10},
	{0, 10},
	{0, 10},
	{0, 10},
	{0, 10},
	// numerator = 11
	{-1, -4},
	{-1, -3},
	{-1, -2},
	{-1, -1},
	{-1, 0},
	{-2, -9},
	{-2, -7},
	{-2, -5},
	{-2, -3},
	{-2, -1},
	{-3, -4},
	{-3, -1},
	{-4, -1},
	{-6, -1},
	{-11, 0},
	{0, 0},
	{11, 0},
	{5, 1},
	{3, 2},
	{2, 3},
	{2, 1},
	{1, 5},
	{1, 4},
	{1, 3},
	{1, 2},
	{1, 1},
	{1, 0},
	{0, 11},
	{0, 11},
	{0, 11},
	{0, 11},
	{0, 11},
	// numerator = 12
	{-1, -3},
	{-1, -2},
	{-1, -1},
	{-1, 0},
	{-2, -10},
	{-2, -8},
	{-2, -6},
	{-2, -4},
	{-2, -2},
	{-2, 0},
	{-3, -3},
	{-3, 0},
	{-4, 0},
	{-6, 0},
	{-12, 0},
	{0, 0},
	{12, 0},
	{6, 0},
	{4, 0},
	{3, 0},
	{2, 2},
	{2, 0},
	{1, 5},
	{1, 4},
	{1, 3},
	{1, 2},
	{1, 1},
	{1, 0},
	{0, 12},
	{0, 12},
	{0, 12},
	{0, 12},
	// numerator = 13
	{-1, -2},
	{-1, -1},
	{-1, 0},
	{-2, -11},
	{-2, -9},
	{-2, -7},
	{-2, -5},
	{-2, -3},
	{-2, -1},
	{-3, -5},
	{-3, -2},
	{-4, -3},
	{-5, -2},
	{-7, -1},
	{-13, 0},
	{0, 0},
	{13, 0},
	{6, 1},
	{4, 1},
	{3, 1},
	{2, 3},
	{2, 1},
	{1, 6},
	{1, 5},
	{1, 4},
	{1, 3},
	{1, 2},
	{1, 1},
	{1, 0},
	{0, 13},
	{0, 13},
	{0, 13},
	// numerator = 14
	{-1, -1},
	{-1, 0},
	{-2, -12},
	{-2, -10},
	{-2, -8},
	{-2, -6},
	{-2, -4},
	{-2, -2},
	{-2, 0},
	{-3, -4},
	{-3, -1},
	{-4, -2},
	{-5, -1},
	{-7, 0},
	{-14, 0},
	{0, 0},
	{14, 0},
	{7, 0},
	{4, 2},
	{3, 2},
	{2, 4},
	{2, 2},
	{2, 0},
	{1, 6},
	{1, 5},
	{1, 4},
	{1, 3},
	{1, 2},
	{1, 1},
	{1, 0},
	{0, 14},
	{0, 14},
	// numerator = 15
	{-1, 0},
	{-2, -13},
	{-2, -11},
	{-2, -9},
	{-2, -7},
	{-2, -5},
	{-2, -3},
	{-2, -1},
	{-3, -6},
	{-3, -3},
	{-3, 0},
	{-4, -1},
	{-5, 0},
	{-8, -1},
	{-15, 0},
	{0, 0},
	{15, 0},
	{7, 1},
	{5, 0},
	{3, 3},
	{3, 0},
	{2, 3},
	{2, 1},
	{1, 7},
	{1, 6},
	{1, 5},
	{1, 4},
	{1, 3},
	{1, 2},
	{1, 1},
	{1, 0},
	{0, 15},
	// numerator = 16
	{-2, -14},
	{-2, -12},
	{-2, -10},
	{-2, -8},
	{-2, -6},
	{-2, -4},
	{-2, -2},
	{-2, 0},
	{-3, -5},
	{-3, -2},
	{-4, -4},
	{-4, 0},
	{-6, -2},
	{-8, 0},
	{-16, 0},
	{0, 0},
	{16, 0},
	{8, 0},
	{5, 1},
	{4, 0},
	{3, 1},
	{2, 4},
	{2, 2},
	{2, 0},
	{1, 7},
	{1, 6},
	{1, 5},
	{1, 4},
	{1, 3},
	{1, 2},
	{1, 1},
	{1, 0},

};

uint8_t* skintable[MAX_LBM_HEIGHT];
int skinwidth;
uint8_t* skinstart;

void D_PolysetDrawSpans8(spanpackage_t* pspanpackage);
void D_PolysetCalcGradients(int skinwidth);
void D_DrawSubdiv();
void D_DrawNonSubdiv();
void D_PolysetRecursiveTriangle(int* p1, int* p2, int* p3);
void D_PolysetSetEdgeTable();
void D_RasterizeAliasPolySmooth();
void D_PolysetScanLeftEdge(int height);


/*
================
D_PolysetDraw
================
*/
void D_PolysetDraw()
{
	spanpackage_t spans[DPS_MAXSPANS + 1 +
		((CACHE_SIZE - 1) / sizeof(spanpackage_t)) + 1];
	// one extra because of cache line pretouching

	a_spans = (spanpackage_t*)
		(((long)&spans[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));

	if (r_affinetridesc.drawtype)
	{
		D_DrawSubdiv();
	}
	else
	{
		D_DrawNonSubdiv();
	}
}


/*
================
D_PolysetDrawFinalVerts
================
*/
void D_PolysetDrawFinalVerts(finalvert_t* fv, int numverts)
{
	for (int i = 0; i < numverts; i++, fv++)
	{
		// valid triangle coordinates for filling can include the bottom and
		// right clip edges, due to the fill rule; these shouldn't be drawn
		if ((fv->v[0] < r_refdef.vrectright) &&
			(fv->v[1] < r_refdef.vrectbottom))
		{
			int z = fv->v[5] >> 16;
			short* zbuf = zspantable[fv->v[1]] + fv->v[0];
			if (z >= *zbuf)
			{
				*zbuf = z;
				int pix = skintable[fv->v[3] >> 16][fv->v[2] >> 16];
				pix = ((uint8_t*)acolormap)[pix + (fv->v[4] & 0xFF00)];
				d_viewbuffer[d_scantable[fv->v[1]] + fv->v[0]] = pix;
			}
		}
	}
}


/*
================
D_DrawSubdiv
================
*/
void D_DrawSubdiv()
{
	finalvert_t* pfv = r_affinetridesc.pfinalverts;
	mtriangle_t* ptri = r_affinetridesc.ptriangles;
	int lnumtriangles = r_affinetridesc.numtriangles;

	for (int i = 0; i < lnumtriangles; i++)
	{
		finalvert_t* index0 = pfv + ptri[i].vertindex[0];
		finalvert_t* index1 = pfv + ptri[i].vertindex[1];
		finalvert_t* index2 = pfv + ptri[i].vertindex[2];

		if (((index0->v[1] - index1->v[1]) *
			(index0->v[0] - index2->v[0]) -
			(index0->v[0] - index1->v[0]) *
			(index0->v[1] - index2->v[1])) >= 0)
		{
			continue;
		}

		d_pcolormap = &((uint8_t*)acolormap)[index0->v[4] & 0xFF00];

		if (ptri[i].facesfront)
		{
			D_PolysetRecursiveTriangle(index0->v, index1->v, index2->v);
		}
		else
		{
			int s0 = index0->v[2];
			int s1 = index1->v[2];
			int s2 = index2->v[2];

			if (index0->flags & ALIAS_ONSEAM)
				index0->v[2] += r_affinetridesc.seamfixupX16;
			if (index1->flags & ALIAS_ONSEAM)
				index1->v[2] += r_affinetridesc.seamfixupX16;
			if (index2->flags & ALIAS_ONSEAM)
				index2->v[2] += r_affinetridesc.seamfixupX16;

			D_PolysetRecursiveTriangle(index0->v, index1->v, index2->v);

			index0->v[2] = s0;
			index1->v[2] = s1;
			index2->v[2] = s2;
		}
	}
}


/*
================
D_DrawNonSubdiv
================
*/
void D_DrawNonSubdiv()
{
	finalvert_t* pfv = r_affinetridesc.pfinalverts;
	mtriangle_t* ptri = r_affinetridesc.ptriangles;
	int lnumtriangles = r_affinetridesc.numtriangles;

	for (int i = 0; i < lnumtriangles; i++, ptri++)
	{
		finalvert_t* index0 = pfv + ptri->vertindex[0];
		finalvert_t* index1 = pfv + ptri->vertindex[1];
		finalvert_t* index2 = pfv + ptri->vertindex[2];

		d_xdenom = (index0->v[1] - index1->v[1]) *
			(index0->v[0] - index2->v[0]) -
			(index0->v[0] - index1->v[0]) * (index0->v[1] - index2->v[1]);

		if (d_xdenom >= 0)
		{
			continue;
		}

		r_p0[0] = index0->v[0]; // u
		r_p0[1] = index0->v[1]; // v
		r_p0[2] = index0->v[2]; // s
		r_p0[3] = index0->v[3]; // t
		r_p0[4] = index0->v[4]; // light
		r_p0[5] = index0->v[5]; // iz

		r_p1[0] = index1->v[0];
		r_p1[1] = index1->v[1];
		r_p1[2] = index1->v[2];
		r_p1[3] = index1->v[3];
		r_p1[4] = index1->v[4];
		r_p1[5] = index1->v[5];

		r_p2[0] = index2->v[0];
		r_p2[1] = index2->v[1];
		r_p2[2] = index2->v[2];
		r_p2[3] = index2->v[3];
		r_p2[4] = index2->v[4];
		r_p2[5] = index2->v[5];

		if (!ptri->facesfront)
		{
			if (index0->flags & ALIAS_ONSEAM)
				r_p0[2] += r_affinetridesc.seamfixupX16;
			if (index1->flags & ALIAS_ONSEAM)
				r_p1[2] += r_affinetridesc.seamfixupX16;
			if (index2->flags & ALIAS_ONSEAM)
				r_p2[2] += r_affinetridesc.seamfixupX16;
		}

		D_PolysetSetEdgeTable();
		D_RasterizeAliasPolySmooth();
	}
}


/*
================
D_PolysetRecursiveTriangle
================
*/
void D_PolysetRecursiveTriangle(int* lp1, int* lp2, int* lp3)
{
	int z;
	int* temp;
	int new_[6];
	short* zbuf;

	int d = lp2[0] - lp1[0];
	if (d < -1 || d > 1)
		goto split;
	d = lp2[1] - lp1[1];
	if (d < -1 || d > 1)
		goto split;

	d = lp3[0] - lp2[0];
	if (d < -1 || d > 1)
		goto split2;
	d = lp3[1] - lp2[1];
	if (d < -1 || d > 1)
		goto split2;

	d = lp1[0] - lp3[0];
	if (d < -1 || d > 1)
		goto split3;
	d = lp1[1] - lp3[1];
	if (d < -1 || d > 1)
	{
	split3:
		temp = lp1;
		lp1 = lp3;
		lp3 = lp2;
		lp2 = temp;

		goto split;
	}

	return; // entire tri is filled

split2:
	temp = lp1;
	lp1 = lp2;
	lp2 = lp3;
	lp3 = temp;

split:
	// split this_ edge
	new_[0] = (lp1[0] + lp2[0]) >> 1;
	new_[1] = (lp1[1] + lp2[1]) >> 1;
	new_[2] = (lp1[2] + lp2[2]) >> 1;
	new_[3] = (lp1[3] + lp2[3]) >> 1;
	new_[5] = (lp1[5] + lp2[5]) >> 1;

	// draw the point if splitting a leading edge
	if (lp2[1] > lp1[1])
		goto nodraw;
	if ((lp2[1] == lp1[1]) && (lp2[0] < lp1[0]))
		goto nodraw;


	z = new_[5] >> 16;
	zbuf = zspantable[new_[1]] + new_[0];
	if (z >= *zbuf)
	{
		*zbuf = z;
		int pix = d_pcolormap[skintable[new_[3] >> 16][new_[2] >> 16]];
		d_viewbuffer[d_scantable[new_[1]] + new_[0]] = pix;
	}

nodraw:
	// recursively continue
	D_PolysetRecursiveTriangle(lp3, lp1, new_);
	D_PolysetRecursiveTriangle(lp3, new_, lp2);
}


/*
================
D_PolysetUpdateTables
================
*/
void D_PolysetUpdateTables()
{
	if (r_affinetridesc.skinwidth != skinwidth ||
		r_affinetridesc.pskin != skinstart)
	{
		skinwidth = r_affinetridesc.skinwidth;
		skinstart = (uint8_t*)r_affinetridesc.pskin;
		uint8_t* s = skinstart;
		for (int i = 0; i < MAX_LBM_HEIGHT; i++, s += skinwidth)
			skintable[i] = s;
	}
}


/*
===================
D_PolysetScanLeftEdge
====================
*/
void D_PolysetScanLeftEdge(int height)
{
	do
	{
		d_pedgespanpackage->pdest = d_pdest;
		d_pedgespanpackage->pz = d_pz;
		d_pedgespanpackage->count = d_aspancount;
		d_pedgespanpackage->ptex = d_ptex;

		d_pedgespanpackage->sfrac = d_sfrac;
		d_pedgespanpackage->tfrac = d_tfrac;

		// FIXME: need to clamp l, s, t, at both ends?
		d_pedgespanpackage->light = d_light;
		d_pedgespanpackage->zi = d_zi;

		d_pedgespanpackage++;

		errorterm += erroradjustup;
		if (errorterm >= 0)
		{
			d_pdest += d_pdestextrastep;
			d_pz += d_pzextrastep;
			d_aspancount += d_countextrastep;
			d_ptex += d_ptexextrastep;
			d_sfrac += d_sfracextrastep;
			d_ptex += d_sfrac >> 16;

			d_sfrac &= 0xFFFF;
			d_tfrac += d_tfracextrastep;
			if (d_tfrac & 0x10000)
			{
				d_ptex += r_affinetridesc.skinwidth;
				d_tfrac &= 0xFFFF;
			}
			d_light += d_lightextrastep;
			d_zi += d_ziextrastep;
			errorterm -= erroradjustdown;
		}
		else
		{
			d_pdest += d_pdestbasestep;
			d_pz += d_pzbasestep;
			d_aspancount += ubasestep;
			d_ptex += d_ptexbasestep;
			d_sfrac += d_sfracbasestep;
			d_ptex += d_sfrac >> 16;
			d_sfrac &= 0xFFFF;
			d_tfrac += d_tfracbasestep;
			if (d_tfrac & 0x10000)
			{
				d_ptex += r_affinetridesc.skinwidth;
				d_tfrac &= 0xFFFF;
			}
			d_light += d_lightbasestep;
			d_zi += d_zibasestep;
		}
	}
	while (--height);
}

/*
===================
FloorDivMod

Returns mathematically correct (floor-based) quotient and remainder for
numer and denom, both of which should contain no fractional part. The
quotient must fit in 32 bits.
====================
*/

void FloorDivMod(double numer, double denom, int* quotient,
	int* rem)
{
	using namespace std::string_view_literals;
	int q, r;
	double x;

	if (denom <= 0.0)
		Sys_Error(std::format("FloorDivMod: bad denominator {}\n"sv, denom));

	// if ((floor(numer) != numer) || (floor(denom) != denom))
	// Sys_Error ((char*)"FloorDivMod: non-integer numer or denom %f %f\n",
	// numer, denom);

	if (numer >= 0.0)
	{
		x = floor(numer / denom);
		q = (int)x;
		r = (int)floor(numer - (x * denom));
	}
	else
	{
		//
		// perform operations with positive values, and fix mod to make floor-based
		//
		x = floor(-numer / denom);
		q = -(int)x;
		r = (int)floor(-numer - (x * denom));
		if (r != 0)
		{
			q--;
			r = (int)denom - r;
		}
	}

	*quotient = q;
	*rem = r;
}


/*
===================
D_PolysetSetUpForLineScan
====================
*/
void D_PolysetSetUpForLineScan(fixed8_t startvertu, fixed8_t startvertv,
                               fixed8_t endvertu, fixed8_t endvertv)
{
	// TODO: implement x86 version

	errorterm = -1;

	int tm = endvertu - startvertu;
	int tn = endvertv - startvertv;

	if (((tm <= 16) && (tm >= -15)) &&
		((tn <= 16) && (tn >= -15)))
	{
		adivtab_t* ptemp = &adivtab[((tm + 15) << 5) + (tn + 15)];
		ubasestep = ptemp->quotient;
		erroradjustup = ptemp->remainder;
		erroradjustdown = tn;
	}
	else
	{
		double dm = tm;
		double dn = tn;

		FloorDivMod(dm, dn, &ubasestep, &erroradjustup);

		erroradjustdown = dn;
	}
}


/*
================
D_PolysetCalcGradients
================
*/
void D_PolysetCalcGradients(int skinwidth)
{
	float p00_minus_p20 = r_p0[0] - r_p2[0];
	float p01_minus_p21 = r_p0[1] - r_p2[1];
	float p10_minus_p20 = r_p1[0] - r_p2[0];
	float p11_minus_p21 = r_p1[1] - r_p2[1];

	float xstepdenominv = 1.0 / (float)d_xdenom;

	float ystepdenominv = -xstepdenominv;

	// ceil () for light so positive steps are exaggerated, negative steps
	// diminished,  pushing us away from underflow toward overflow. Underflow is
	// very visible, overflow is very unlikely, because of ambient lighting
	float t0 = r_p0[4] - r_p2[4];
	float t1 = r_p1[4] - r_p2[4];
	r_lstepx = (int)
		ceil((t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv);
	r_lstepy = (int)
		ceil((t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv);

	t0 = r_p0[2] - r_p2[2];
	t1 = r_p1[2] - r_p2[2];
	r_sstepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) *
		xstepdenominv);
	r_sstepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) *
		ystepdenominv);

	t0 = r_p0[3] - r_p2[3];
	t1 = r_p1[3] - r_p2[3];
	r_tstepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) *
		xstepdenominv);
	r_tstepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) *
		ystepdenominv);

	t0 = r_p0[5] - r_p2[5];
	t1 = r_p1[5] - r_p2[5];
	r_zistepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) *
		xstepdenominv);
	r_zistepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) *
		ystepdenominv);
	
	a_sstepxfrac = r_sstepx & 0xFFFF;
	a_tstepxfrac = r_tstepx & 0xFFFF;

	a_ststepxwhole = skinwidth * (r_tstepx >> 16) + (r_sstepx >> 16);
}


/*
================
D_PolysetDrawSpans8
================
*/
void D_PolysetDrawSpans8(spanpackage_t* pspanpackage)
{
	do
	{
		int lcount = d_aspancount - pspanpackage->count;

		errorterm += erroradjustup;
		if (errorterm >= 0)
		{
			d_aspancount += d_countextrastep;
			errorterm -= erroradjustdown;
		}
		else
		{
			d_aspancount += ubasestep;
		}

		if (lcount)
		{
			uint8_t* lpdest = (uint8_t*)pspanpackage->pdest;
			uint8_t* lptex = pspanpackage->ptex;
			short* lpz = pspanpackage->pz;
			int lsfrac = pspanpackage->sfrac;
			int ltfrac = pspanpackage->tfrac;
			int llight = pspanpackage->light;
			int lzi = pspanpackage->zi;

			do
			{
				if ((lzi >> 16) >= *lpz)
				{
					*lpdest = ((uint8_t*)acolormap)[*lptex + (llight & 0xFF00)];
					// gel mapping *lpdest = gelmap[*lpdest];
					*lpz = lzi >> 16;
				}
				lpdest++;
				lzi += r_zistepx;
				lpz++;
				llight += r_lstepx;
				lptex += a_ststepxwhole;
				lsfrac += a_sstepxfrac;
				lptex += lsfrac >> 16;
				lsfrac &= 0xFFFF;
				ltfrac += a_tstepxfrac;
				if (ltfrac & 0x10000)
				{
					lptex += r_affinetridesc.skinwidth;
					ltfrac &= 0xFFFF;
				}
			}
			while (--lcount);
		}

		pspanpackage++;
	}
	while (pspanpackage->count != -999999);
}


/*
================
D_PolysetFillSpans8
================
*/
void D_PolysetFillSpans8(spanpackage_t* pspanpackage)
{
	// FIXME: do z buffering

	int color = d_aflatcolor++;

	while (true)
	{
		int lcount = pspanpackage->count;

		if (lcount == -1)
			return;

		if (lcount)
		{
			uint8_t* lpdest = (uint8_t*)pspanpackage->pdest;

			do
			{
				*lpdest++ = color;
			}
			while (--lcount);
		}

		pspanpackage++;
	}
}

/*
================
D_RasterizeAliasPolySmooth
================
*/
void D_RasterizeAliasPolySmooth()
{
	int working_lstepx;

	int* plefttop = pedgetable->pleftedgevert0;
	int* prighttop = pedgetable->prightedgevert0;

	int* pleftbottom = pedgetable->pleftedgevert1;
	int* prightbottom = pedgetable->prightedgevert1;

	int initialleftheight = pleftbottom[1] - plefttop[1];
	int initialrightheight = prightbottom[1] - prighttop[1];

	//
	// set the s, t, and light gradients, which are consistent across the triangle
	// because being a triangle, things are affine
	//
	D_PolysetCalcGradients(r_affinetridesc.skinwidth);

	//
	// rasterize the polygon
	//

	//
	// scan out the top (and possibly only) part of the left edge
	//
	d_pedgespanpackage = a_spans;

	ystart = plefttop[1];
	d_aspancount = plefttop[0] - prighttop[0];

	d_ptex = (uint8_t*)r_affinetridesc.pskin + (plefttop[2] >> 16) +
		(plefttop[3] >> 16) * r_affinetridesc.skinwidth;
	d_sfrac = plefttop[2] & 0xFFFF;
	d_tfrac = plefttop[3] & 0xFFFF;
	d_light = plefttop[4];
	d_zi = plefttop[5];

	d_pdest = d_viewbuffer +
		ystart * screenwidth + plefttop[0];
	d_pz = d_pzbuffer + ystart * d_zwidth + plefttop[0];

	if (initialleftheight == 1)
	{
		d_pedgespanpackage->pdest = d_pdest;
		d_pedgespanpackage->pz = d_pz;
		d_pedgespanpackage->count = d_aspancount;
		d_pedgespanpackage->ptex = d_ptex;

		d_pedgespanpackage->sfrac = d_sfrac;
		d_pedgespanpackage->tfrac = d_tfrac;

		// FIXME: need to clamp l, s, t, at both ends?
		d_pedgespanpackage->light = d_light;
		d_pedgespanpackage->zi = d_zi;

		d_pedgespanpackage++;
	}
	else
	{
		D_PolysetSetUpForLineScan(plefttop[0], plefttop[1],
		                          pleftbottom[0], pleftbottom[1]);
		
		d_pzbasestep = d_zwidth + ubasestep;
		d_pzextrastep = d_pzbasestep + 1;

		d_pdestbasestep = screenwidth + ubasestep;
		d_pdestextrastep = d_pdestbasestep + 1;

		// TODO: can reuse partial expressions here

		// for negative steps in x along left edge, bias toward overflow rather than
		// underflow (sort of turning the floor () we did in the gradient calcs into
		// ceil (), but plus a little bit)
		if (ubasestep < 0)
			working_lstepx = r_lstepx - 1;
		else
			working_lstepx = r_lstepx;

		d_countextrastep = ubasestep + 1;
		d_ptexbasestep = ((r_sstepy + r_sstepx * ubasestep) >> 16) +
			((r_tstepy + r_tstepx * ubasestep) >> 16) *
			r_affinetridesc.skinwidth;
		d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) & 0xFFFF;
		d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) & 0xFFFF;
		d_lightbasestep = r_lstepy + working_lstepx * ubasestep;
		d_zibasestep = r_zistepy + r_zistepx * ubasestep;

		d_ptexextrastep = ((r_sstepy + r_sstepx * d_countextrastep) >> 16) +
			((r_tstepy + r_tstepx * d_countextrastep) >> 16) *
			r_affinetridesc.skinwidth;
		d_sfracextrastep = (r_sstepy + r_sstepx * d_countextrastep) & 0xFFFF;
		d_tfracextrastep = (r_tstepy + r_tstepx * d_countextrastep) & 0xFFFF;
		d_lightextrastep = d_lightbasestep + working_lstepx;
		d_ziextrastep = d_zibasestep + r_zistepx;

		D_PolysetScanLeftEdge(initialleftheight);
	}

	//
	// scan out the bottom part of the left edge, if it exists
	//
	if (pedgetable->numleftedges == 2)
	{
		plefttop = pleftbottom;
		pleftbottom = pedgetable->pleftedgevert2;

		int height = pleftbottom[1] - plefttop[1];

		// TODO: make this_ a function; modularize this_ function in general

		ystart = plefttop[1];
		d_aspancount = plefttop[0] - prighttop[0];
		d_ptex = (uint8_t*)r_affinetridesc.pskin + (plefttop[2] >> 16) +
			(plefttop[3] >> 16) * r_affinetridesc.skinwidth;
		d_sfrac = 0;
		d_tfrac = 0;
		d_light = plefttop[4];
		d_zi = plefttop[5];

		d_pdest = d_viewbuffer + ystart * screenwidth + plefttop[0];
		d_pz = d_pzbuffer + ystart * d_zwidth + plefttop[0];

		if (height == 1)
		{
			d_pedgespanpackage->pdest = d_pdest;
			d_pedgespanpackage->pz = d_pz;
			d_pedgespanpackage->count = d_aspancount;
			d_pedgespanpackage->ptex = d_ptex;

			d_pedgespanpackage->sfrac = d_sfrac;
			d_pedgespanpackage->tfrac = d_tfrac;

			// FIXME: need to clamp l, s, t, at both ends?
			d_pedgespanpackage->light = d_light;
			d_pedgespanpackage->zi = d_zi;

			d_pedgespanpackage++;
		}
		else
		{
			D_PolysetSetUpForLineScan(plefttop[0], plefttop[1],
			                          pleftbottom[0], pleftbottom[1]);

			d_pdestbasestep = screenwidth + ubasestep;
			d_pdestextrastep = d_pdestbasestep + 1;
			
			d_pzbasestep = d_zwidth + ubasestep;
			d_pzextrastep = d_pzbasestep + 1;

			if (ubasestep < 0)
				working_lstepx = r_lstepx - 1;
			else
				working_lstepx = r_lstepx;

			d_countextrastep = ubasestep + 1;
			d_ptexbasestep = ((r_sstepy + r_sstepx * ubasestep) >> 16) +
				((r_tstepy + r_tstepx * ubasestep) >> 16) *
				r_affinetridesc.skinwidth;
			d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) & 0xFFFF;
			d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) & 0xFFFF;
			d_lightbasestep = r_lstepy + working_lstepx * ubasestep;
			d_zibasestep = r_zistepy + r_zistepx * ubasestep;

			d_ptexextrastep = ((r_sstepy + r_sstepx * d_countextrastep) >> 16) +
				((r_tstepy + r_tstepx * d_countextrastep) >> 16) *
				r_affinetridesc.skinwidth;
			d_sfracextrastep = (r_sstepy + r_sstepx * d_countextrastep) & 0xFFFF;
			d_tfracextrastep = (r_tstepy + r_tstepx * d_countextrastep) & 0xFFFF;
			d_lightextrastep = d_lightbasestep + working_lstepx;
			d_ziextrastep = d_zibasestep + r_zistepx;

			D_PolysetScanLeftEdge(height);
		}
	}

	// scan out the top (and possibly only) part of the right edge, updating the
	// count field
	d_pedgespanpackage = a_spans;

	D_PolysetSetUpForLineScan(prighttop[0], prighttop[1],
	                          prightbottom[0], prightbottom[1]);
	d_aspancount = 0;
	d_countextrastep = ubasestep + 1;
	int originalcount = a_spans[initialrightheight].count;
	a_spans[initialrightheight].count = -999999; // mark end of the spanpackages
	D_PolysetDrawSpans8(a_spans);

	// scan out the bottom part of the right edge, if it exists
	if (pedgetable->numrightedges == 2)
	{
		spanpackage_t* pstart = a_spans + initialrightheight;
		pstart->count = originalcount;

		d_aspancount = prightbottom[0] - prighttop[0];

		prighttop = prightbottom;
		prightbottom = pedgetable->prightedgevert2;

		int height = prightbottom[1] - prighttop[1];

		D_PolysetSetUpForLineScan(prighttop[0], prighttop[1],
		                          prightbottom[0], prightbottom[1]);

		d_countextrastep = ubasestep + 1;
		a_spans[initialrightheight + height].count = -999999;
		// mark end of the spanpackages
		D_PolysetDrawSpans8(pstart);
	}
}


/*
================
D_PolysetSetEdgeTable
================
*/
void D_PolysetSetEdgeTable()
{
	int edgetableindex = 0; // assume the vertices are already in
	//  top to bottom order

	//
	// determine which edges are right & left, and the order in which
	// to rasterize them
	//
	if (r_p0[1] >= r_p1[1])
	{
		if (r_p0[1] == r_p1[1])
		{
			if (r_p0[1] < r_p2[1])
				pedgetable = &edgetables[2];
			else
				pedgetable = &edgetables[5];

			return;
		}
		edgetableindex = 1;
	}

	if (r_p0[1] == r_p2[1])
	{
		if (edgetableindex)
			pedgetable = &edgetables[8];
		else
			pedgetable = &edgetables[9];

		return;
	}
	if (r_p1[1] == r_p2[1])
	{
		if (edgetableindex)
			pedgetable = &edgetables[10];
		else
			pedgetable = &edgetables[11];

		return;
	}

	if (r_p0[1] > r_p2[1])
		edgetableindex += 2;

	if (r_p1[1] > r_p2[1])
		edgetableindex += 4;

	pedgetable = &edgetables[edgetableindex];
}

