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

// draw.c -- this_ is the only file outside the refresh that touches the
// vid buffer

#include "draw.h"

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

#include "host.h"
#include "chase.h"

#define GL_COLOR_INDEX8_EXT     0x80E5

extern unsigned char d_15to8table[65536];

cvar_t gl_nobind = {(char*)"gl_nobind", (char*)"0"};
cvar_t gl_max_size = {(char*)"gl_max_size", (char*)"1024"};
cvar_t gl_picmip = {(char*)"gl_picmip", (char*)"0"};

byte* draw_chars; // 8*8 graphic characters
qpic_t* draw_disc;
qpic_t* draw_backtile;

int translate_texture;
int char_texture;

typedef struct
{
	int texnum;
	float sl, tl, sh, th;
} glpic_t;

byte conback_buffer[sizeof(qpic_t) + sizeof(glpic_t)];
qpic_t* conback = (qpic_t*)&conback_buffer;

int gl_lightmap_format = 4;
int gl_solid_format = 3;
int gl_alpha_format = 4;

int gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int gl_filter_max = GL_LINEAR;


int texels;

typedef struct
{
	int texnum;
	char identifier[64];
	int width, height;
	bool mipmap;
} gltexture_t;

#define MAX_GLTEXTURES 1024
gltexture_t gltextures[MAX_GLTEXTURES];
int numgltextures;

int GL_LoadPicTexture(qpic_t* pic);

void GL_Bind(int texnum)
{
	if (gl_nobind.value)
		texnum = char_texture;
	if (currenttexture == texnum)
		return;
	currenttexture = texnum;
	bindTexFunc(GL_TEXTURE_2D, texnum);
}


/*
=============================================================================

  scrap allocation

  Allocate all the little status bar obejcts into a single texture
  to crutch up stupid hardware / drivers

=============================================================================
*/

#define MAX_SCRAPS 2
#define BLOCK_WIDTH 256
#define BLOCK_HEIGHT 256

int scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte scrap_texels[MAX_SCRAPS][BLOCK_WIDTH * BLOCK_HEIGHT * 4];
bool scrap_dirty;
int scrap_texnum;

// returns a texture number and the position inside it
int Scrap_AllocBlock(int w, int h, int* x, int* y)
{
	using namespace std::string_view_literals;
	int i, j;
	int bestx;

	for (int texnum = 0; texnum < MAX_SCRAPS; texnum++)
	{
		int best = BLOCK_HEIGHT;

		for (i = 0; i < BLOCK_WIDTH - w; i++)
		{
			int best2 = 0;

			for (j = 0; j < w; j++)
			{
				if (scrap_allocated[texnum][i + j] >= best)
					break;
				if (scrap_allocated[texnum][i + j] > best2)
					best2 = scrap_allocated[texnum][i + j];
			}
			if (j == w)
			{
				// this_ is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i = 0; i < w; i++)
			scrap_allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	Sys_Error("Scrap_AllocBlock: full"sv);
}

int scrap_uploads;

void Scrap_Upload()
{
	scrap_uploads++;

	for (int texnum = 0; texnum < MAX_SCRAPS; texnum++)
	{
		GL_Bind(scrap_texnum + texnum);
		GL_Upload8(scrap_texels[texnum], BLOCK_WIDTH, BLOCK_HEIGHT, false, true);
	}
	scrap_dirty = false;
}

//=============================================================================
/* Support Routines */

typedef struct cachepic_s
{
	char name[MAX_QPATH];
	qpic_t pic;
	byte padding[32]; // for appended glpic
} cachepic_t;

#define MAX_CACHED_PICS 128
cachepic_t menu_cachepics[MAX_CACHED_PICS];
int menu_numcachepics;

byte menuplyr_pixels[4096];

int pic_texels;
int pic_count;

qpic_t* Draw_PicFromWad(char* name)
{
	qpic_t* p = static_cast<qpic_t*>(W_GetLumpName(name));
	glpic_t* gl = (glpic_t*)p->data;

	// load little ones into the scrap
	if (p->width < 64 && p->height < 64)
	{
		int x, y;

		int texnum = Scrap_AllocBlock(p->width, p->height, &x, &y);
		scrap_dirty = true;
		int k = 0;
		for (int i = 0; i < p->height; i++)
			for (int j = 0; j < p->width; j++, k++)
				scrap_texels[texnum][(y + i) * BLOCK_WIDTH + x + j] = p->data[k];
		texnum += scrap_texnum;
		gl->texnum = texnum;
		gl->sl = (x + 0.01) / (float)BLOCK_WIDTH;
		gl->sh = (x + p->width - 0.01) / (float)BLOCK_WIDTH;
		gl->tl = (y + 0.01) / (float)BLOCK_WIDTH;
		gl->th = (y + p->height - 0.01) / (float)BLOCK_WIDTH;

		pic_count++;
		pic_texels += p->width * p->height;
	}
	else
	{
		gl->texnum = GL_LoadPicTexture(p);
		gl->sl = 0;
		gl->sh = 1;
		gl->tl = 0;
		gl->th = 1;
	}
	return p;
}


/*
================
Draw_CachePic
================
*/
qpic_t* Draw_CachePic(char* path)
{
	using namespace std::string_view_literals;
	cachepic_t* pic;
	int i;

	for (pic = menu_cachepics, i = 0; i < menu_numcachepics; pic++, i++)
		if (!strcmp(path, pic->name))
			return &pic->pic;

	if (menu_numcachepics == MAX_CACHED_PICS)
		Sys_Error("menu_numcachepics == MAX_CACHED_PICS"sv);
	menu_numcachepics++;
	strcpy(pic->name, path);

	//
	// load the pic from disk
	//
	qpic_t* dat = (qpic_t*)COM_LoadTempFile(path);
	if (!dat)
		Sys_Error(std::format("Draw_CachePic: failed to load {}"sv, path));
	SwapPic(dat);

	// HACK HACK HACK --- we need to keep the bytes for
	// the translatable player picture just for the menu
	// configuration dialog
	if (!strcmp(path, "gfx/menuplyr.lmp"))
		memcpy(menuplyr_pixels, dat->data, dat->width * dat->height);

	pic->pic.width = dat->width;
	pic->pic.height = dat->height;

	glpic_t* gl = (glpic_t*)pic->pic.data;
	gl->texnum = GL_LoadPicTexture(dat);
	gl->sl = 0;
	gl->sh = 1;
	gl->tl = 0;
	gl->th = 1;

	return &pic->pic;
}


void Draw_CharToConback(int num, byte* dest)
{
	int row = num >> 4;
	int col = num & 15;
	byte* source = draw_chars + (row << 10) + (col << 3);

	int drawline = 8;

	while (drawline--)
	{
		for (int x = 0; x < 8; x++)
			if (source[x] != 255)
				dest[x] = 0x60 + source[x];
		source += 128;
		dest += 320;
	}
}

typedef struct
{
	char* name;
	int minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{(char*)"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{(char*)"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{(char*)"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{(char*)"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{(char*)"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{(char*)"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

/*
===============
Draw_TextureMode_f
===============
*/
void Draw_TextureMode_f()
{
	int i;
	gltexture_t* glt;

	if (Cmd_Argc() == 1)
	{
		for (i = 0; i < 6; i++)
			if (gl_filter_min == modes[i].minimize)
			{
				Con_Printf((char*)"%s\n", modes[i].name);
				return;
			}
		Con_Printf((char*)"current filter is unknown???\n");
		return;
	}

	for (i = 0; i < 6; i++)
	{
		if (!Q_strcasecmp(modes[i].name, Cmd_Argv(1)))
			break;
	}
	if (i == 6)
	{
		Con_Printf((char*)"bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for (i = 0, glt = gltextures; i < numgltextures; i++, glt++)
	{
		if (glt->mipmap)
		{
			GL_Bind(glt->texnum);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

/*
===============
Draw_Init
===============
*/
void Draw_Init()
{
	using namespace std::string_view_literals;
	byte*src;
	char ver[40];
	int f, fstep;


	Cvar_RegisterVariable(&gl_nobind);
	Cvar_RegisterVariable(&gl_max_size);
	Cvar_RegisterVariable(&gl_picmip);

	// 3dfx can only handle 256 wide textures
	if (!Q_strncasecmp((char*)gl_renderer, (char*)"3dfx", 4) ||
		strstr((char*)gl_renderer, "Glide"))
		Cvar_Set((char*)"gl_max_size", (char*)"256");

	Cmd_AddCommand((char*)"gl_texturemode", &Draw_TextureMode_f);

	// load the console background and the charset
	// by hand, because we need to write the version
	// string into the background before turning
	// it into a texture
	draw_chars = static_cast<byte*>(W_GetLumpName((char*)"conchars"));
	for (int i = 0; i < 256 * 64; i++)
		if (draw_chars[i] == 0)
			draw_chars[i] = 255; // proper transparent color

	// now turn them into textures
	char_texture = GL_LoadTexture((char*)"charset", 128, 128, draw_chars, false, true);

	int start = Hunk_LowMark();

	qpic_t* cb = (qpic_t*)COM_LoadTempFile((char*)"gfx/conback.lmp");
	if (!cb)
		Sys_Error("Couldn't load gfx/conback.lmp"sv);
	SwapPic(cb);

	// hack the version number directly into the pic
	sprintf(ver, (char*)"(gl %4.2f) %4.2f", (float)GLQUAKE_VERSION, (float)VERSION);
	byte* dest = cb->data + 320 * 186 + 320 - 11 - 8 * strlen(ver);
	int y = strlen(ver);
	for (int x = 0; x < y; x++)
		Draw_CharToConback(ver[x], dest + (x << 3));
	
	conback->width = cb->width;
	conback->height = cb->height;
	byte* ncdata = cb->data;

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glpic_t* gl = (glpic_t*)conback->data;
	gl->texnum = GL_LoadTexture((char*)"conback", conback->width, conback->height, ncdata, false, false);
	gl->sl = 0;
	gl->sh = 1;
	gl->tl = 0;
	gl->th = 1;
	conback->width = vid.width;
	conback->height = vid.height;

	// free loaded console
	Hunk_FreeToLowMark(start);

	// save a texture slot for translated picture
	translate_texture = texture_extension_number++;

	// save slots for scraps
	scrap_texnum = texture_extension_number;
	texture_extension_number += MAX_SCRAPS;

	//
	// get the other pics we need
	//
	draw_disc = Draw_PicFromWad((char*)"disc");
	draw_backtile = Draw_PicFromWad((char*)"backtile");
}


/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Character(int x, int y, int num)
{
	byte* dest;
	byte* source;
	unsigned short* pusdest;
	int drawline;

	if (num == 32)
		return; // space

	num &= 255;

	if (y <= -8)
		return; // totally off screen

	int row = num >> 4;
	int col = num & 15;

	float frow = row * 0.0625;
	float fcol = col * 0.0625;
	float size = 0.0625;

	GL_Bind(char_texture);

	glBegin(GL_QUADS);
	glTexCoord2f(fcol, frow);
	glVertex2f(x, y);
	glTexCoord2f(fcol + size, frow);
	glVertex2f(x + 8, y);
	glTexCoord2f(fcol + size, frow + size);
	glVertex2f(x + 8, y + 8);
	glTexCoord2f(fcol, frow + size);
	glVertex2f(x, y + 8);
	glEnd();
}

/*
================
Draw_String
================
*/
void Draw_String(int x, int y, char* str)
{
	while (*str)
	{
		Draw_Character(x, y, *str);
		str++;
		x += 8;
	}
}

/*
================
Draw_DebugChar

Draws a single character directly to the upper right corner of the screen.
This is for debugging lockups by drawing different chars in different parts
of the code.
================
*/
void Draw_DebugChar(char num)
{
}

/*
=============
Draw_AlphaPic
=============
*/
void Draw_AlphaPic(int x, int y, qpic_t* pic, float alpha)
{
	byte *dest, *source;
	unsigned short* pusdest;
	int v, u;

	if (scrap_dirty)
		Scrap_Upload();
	glpic_t* gl = (glpic_t*)pic->data;
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// glCullFace(GL_FRONT);
	glColor4f(1, 1, 1, alpha);
	GL_Bind(gl->texnum);
	glBegin(GL_QUADS);
	glTexCoord2f(gl->sl, gl->tl);
	glVertex2f(x, y);
	glTexCoord2f(gl->sh, gl->tl);
	glVertex2f(x + pic->width, y);
	glTexCoord2f(gl->sh, gl->th);
	glVertex2f(x + pic->width, y + pic->height);
	glTexCoord2f(gl->sl, gl->th);
	glVertex2f(x, y + pic->height);
	glEnd();
	glColor4f(1, 1, 1, 1);
	glEnable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
}


/*
=============
Draw_Pic
=============
*/
void Draw_Pic(int x, int y, qpic_t* pic)
{
	byte *dest, *source;
	unsigned short* pusdest;
	int v, u;

	if (scrap_dirty)
		Scrap_Upload();
	glpic_t* gl = (glpic_t*)pic->data;
	glColor4f(1, 1, 1, 1);
	GL_Bind(gl->texnum);
	glBegin(GL_QUADS);
	glTexCoord2f(gl->sl, gl->tl);
	glVertex2f(x, y);
	glTexCoord2f(gl->sh, gl->tl);
	glVertex2f(x + pic->width, y);
	glTexCoord2f(gl->sh, gl->th);
	glVertex2f(x + pic->width, y + pic->height);
	glTexCoord2f(gl->sl, gl->th);
	glVertex2f(x, y + pic->height);
	glEnd();
}


/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic(int x, int y, qpic_t* pic)
{
	using namespace std::string_view_literals;
	byte *dest, *source, tbyte;
	unsigned short* pusdest;
	int v, u;

	if (x < 0 || (unsigned)(x + pic->width) > vid.width || y < 0 ||
		(unsigned)(y + pic->height) > vid.height)
	{
		Sys_Error("Draw_TransPic: bad coordinates"sv);
	}

	Draw_Pic(x, y, pic);
}


/*
=============
Draw_TransPicTranslate

Only used for the player color selection menu
=============
*/
void Draw_TransPicTranslate(int x, int y, qpic_t* pic, byte* translation)
{
	unsigned trans[64 * 64];

	GL_Bind(translate_texture);

	int c = pic->width * pic->height;

	unsigned* dest = trans;
	for (int v = 0; v < 64; v++, dest += 64)
	{
		byte* src = &menuplyr_pixels[((v * pic->height) >> 6) * pic->width];
		for (int u = 0; u < 64; u++)
		{
			int p = src[(u * pic->width) >> 6];
			if (p == 255)
				dest[u] = p;
			else
				dest[u] = d_8to24table[translation[p]];
		}
	}

	glTexImage2D(GL_TEXTURE_2D, 0, gl_alpha_format, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glColor3f(1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(x, y);
	glTexCoord2f(1, 0);
	glVertex2f(x + pic->width, y);
	glTexCoord2f(1, 1);
	glVertex2f(x + pic->width, y + pic->height);
	glTexCoord2f(0, 1);
	glVertex2f(x, y + pic->height);
	glEnd();
}


/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground(int lines)
{
	int y = (vid.height * 3) >> 2;

	if (lines > y)
		Draw_Pic(0, lines - vid.height, conback);
	else
		Draw_AlphaPic(0, lines - vid.height, conback, (float)(1.2 * lines) / y);
}


/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear(int x, int y, int w, int h)
{
	glColor3f(1, 1, 1);
	GL_Bind(*(int*)draw_backtile->data);
	glBegin(GL_QUADS);
	glTexCoord2f(x / 64.0, y / 64.0);
	glVertex2f(x, y);
	glTexCoord2f((x + w) / 64.0, y / 64.0);
	glVertex2f(x + w, y);
	glTexCoord2f((x + w) / 64.0, (y + h) / 64.0);
	glVertex2f(x + w, y + h);
	glTexCoord2f(x / 64.0, (y + h) / 64.0);
	glVertex2f(x, y + h);
	glEnd();
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill(int x, int y, int w, int h, int c)
{
	glDisable(GL_TEXTURE_2D);
	glColor3f(host_basepal[c * 3] / 255.0,
	          host_basepal[c * 3 + 1] / 255.0,
	          host_basepal[c * 3 + 2] / 255.0);

	glBegin(GL_QUADS);

	glVertex2f(x, y);
	glVertex2f(x + w, y);
	glVertex2f(x + w, y + h);
	glVertex2f(x, y + h);

	glEnd();
	glColor3f(1, 1, 1);
	glEnable(GL_TEXTURE_2D);
}

//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen()
{
	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glColor4f(0, 0, 0, 0.8);
	glBegin(GL_QUADS);

	glVertex2f(0, 0);
	glVertex2f(vid.width, 0);
	glVertex2f(vid.width, vid.height);
	glVertex2f(0, vid.height);

	glEnd();
	glColor4f(1, 1, 1, 1);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	Sbar_Changed();
}

//=============================================================================

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void Draw_BeginDisc()
{
	if (!draw_disc)
		return;
	glDrawBuffer(GL_FRONT);
	Draw_Pic(vid.width - 24, 0, draw_disc);
	glDrawBuffer(GL_BACK);
}


/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc()
{
}

/*
================
GL_Set2D

Setup as if the screen was 320*200
================
*/
void GL_Set2D()
{
	glViewport(glx, gly, glwidth, glheight);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, vid.width, vid.height, 0, -99999, 99999);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	// glDisable (GL_ALPHA_TEST);

	glColor4f(1, 1, 1, 1);
}

//====================================================================

/*
================
GL_FindTexture
================
*/
int GL_FindTexture(char* identifier)
{
	int i;
	gltexture_t* glt;

	for (i = 0, glt = gltextures; i < numgltextures; i++, glt++)
	{
		if (!strcmp(identifier, glt->identifier))
			return gltextures[i].texnum;
	}

	return -1;
}

/*
================
GL_ResampleTexture
================
*/
void GL_ResampleTexture(unsigned* in, int inwidth, int inheight, unsigned* out, int outwidth, int outheight)
{
	unsigned fracstep = inwidth * 0x10000 / outwidth;
	for (int i = 0; i < outheight; i++, out += outwidth)
	{
		unsigned* inrow = in + inwidth * (i * inheight / outheight);
		unsigned frac = fracstep >> 1;
		for (int j = 0; j < outwidth; j += 4)
		{
			out[j] = inrow[frac >> 16];
			frac += fracstep;
			out[j + 1] = inrow[frac >> 16];
			frac += fracstep;
			out[j + 2] = inrow[frac >> 16];
			frac += fracstep;
			out[j + 3] = inrow[frac >> 16];
			frac += fracstep;
		}
	}
}

/*
================
GL_Resample8BitTexture -- JACK
================
*/
void GL_Resample8BitTexture(unsigned char* in, int inwidth, int inheight, unsigned char* out, int outwidth,
                            int outheight)
{
	unsigned fracstep = inwidth * 0x10000 / outwidth;
	for (int i = 0; i < outheight; i++, out += outwidth)
	{
		unsigned char* inrow = in + inwidth * (i * inheight / outheight);
		unsigned frac = fracstep >> 1;
		for (int j = 0; j < outwidth; j += 4)
		{
			out[j] = inrow[frac >> 16];
			frac += fracstep;
			out[j + 1] = inrow[frac >> 16];
			frac += fracstep;
			out[j + 2] = inrow[frac >> 16];
			frac += fracstep;
			out[j + 3] = inrow[frac >> 16];
			frac += fracstep;
		}
	}
}


/*
================
GL_MipMap

Operates in place, quartering the size of the texture
================
*/
void GL_MipMap(byte* in, int width, int height)
{
	width <<= 2;
	height >>= 1;
	byte* out = in;
	for (int i = 0; i < height; i++, in += width)
	{
		for (int j = 0; j < width; j += 8, out += 4, in += 8)
		{
			out[0] = (in[0] + in[4] + in[width + 0] + in[width + 4]) >> 2;
			out[1] = (in[1] + in[5] + in[width + 1] + in[width + 5]) >> 2;
			out[2] = (in[2] + in[6] + in[width + 2] + in[width + 6]) >> 2;
			out[3] = (in[3] + in[7] + in[width + 3] + in[width + 7]) >> 2;
		}
	}
}

/*
================
GL_MipMap8Bit

Mipping for 8 bit textures
================
*/
void GL_MipMap8Bit(byte* in, int width, int height)
{
	// width <<=2;
	height >>= 1;
	byte* out = in;
	for (int i = 0; i < height; i++, in += width)
	{
		for (int j = 0; j < width; j += 2, out += 1, in += 2)
		{
			byte* at1 = (uint8_t*)(d_8to24table + in[0]);
			byte* at2 = (uint8_t*)(d_8to24table + in[1]);
			byte* at3 = (uint8_t*)(d_8to24table + in[width + 0]);
			byte* at4 = (uint8_t*)(d_8to24table + in[width + 1]);

			unsigned short r = (at1[0] + at2[0] + at3[0] + at4[0]);
			r >>= 5;
			unsigned short g = (at1[1] + at2[1] + at3[1] + at4[1]);
			g >>= 5;
			unsigned short b = (at1[2] + at2[2] + at3[2] + at4[2]);
			b >>= 5;

			out[0] = d_15to8table[(r << 0) + (g << 5) + (b << 10)];
		}
	}
}

/*
===============
GL_Upload32
===============
*/
void GL_Upload32(unsigned* data, int width, int height, bool mipmap, bool alpha)
{
	using namespace std::string_view_literals;
	static unsigned scaled[1024 * 512]; // [512*256];
	int scaled_width, scaled_height;

	for (scaled_width = 1; scaled_width < width; scaled_width <<= 1);
	for (scaled_height = 1; scaled_height < height; scaled_height <<= 1);

	scaled_width >>= (int)gl_picmip.value;
	scaled_height >>= (int)gl_picmip.value;

	if (scaled_width > gl_max_size.value)
		scaled_width = gl_max_size.value;
	if (scaled_height > gl_max_size.value)
		scaled_height = gl_max_size.value;

	if (scaled_width * scaled_height > sizeof(scaled) / 4)
		Sys_Error("GL_LoadTexture: too big"sv);

	int samples = alpha ? gl_alpha_format : gl_solid_format;
	
	texels += scaled_width * scaled_height;

	if (scaled_width == width && scaled_height == height)
	{
		if (!mipmap)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, samples, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			goto done;
		}
		memcpy(scaled, data, width * height * 4);
	}
	else
		GL_ResampleTexture(data, width, height, scaled, scaled_width, scaled_height);

	glTexImage2D(GL_TEXTURE_2D, 0, samples, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
	if (mipmap)
	{
		int miplevel = 0;
		while (scaled_width > 1 || scaled_height > 1)
		{
			GL_MipMap((uint8_t*)scaled, scaled_width, scaled_height);
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
				scaled_width = 1;
			if (scaled_height < 1)
				scaled_height = 1;
			miplevel++;
			glTexImage2D(GL_TEXTURE_2D, miplevel, samples, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
			             scaled);
		}
	}
done:;


	if (mipmap)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
}

void GL_Upload8_EXT(uint8_t* data, int width, int height, bool mipmap, bool alpha)
{
	using namespace std::string_view_literals;
	int p;
	static unsigned j;
	static unsigned char scaled[1024 * 512]; // [512*256];
	int scaled_width, scaled_height;

	int s = width * height;
	// if there are no transparent pixels, make it a 3 component
	// texture even if it was specified as otherwise
	if (alpha)
	{
		bool noalpha = true;
		for (int i = 0; i < s; i++)
		{
			if (data[i] == 255)
				noalpha = false;
		}

		if (alpha && noalpha)
			alpha = false;
	}
	for (scaled_width = 1; scaled_width < width; scaled_width <<= 1);
	for (scaled_height = 1; scaled_height < height; scaled_height <<= 1);

	scaled_width >>= (int)gl_picmip.value;
	scaled_height >>= (int)gl_picmip.value;

	if (scaled_width > gl_max_size.value)
		scaled_width = gl_max_size.value;
	if (scaled_height > gl_max_size.value)
		scaled_height = gl_max_size.value;

	if (scaled_width * scaled_height > sizeof(scaled))
		Sys_Error("GL_LoadTexture: too big"sv);

	int samples = 1; // alpha ? gl_alpha_format : gl_solid_format;

	texels += scaled_width * scaled_height;

	if (scaled_width == width && scaled_height == height)
	{
		if (!mipmap)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, scaled_width, scaled_height, 0, GL_COLOR_INDEX,
			             GL_UNSIGNED_BYTE, data);
			goto done;
		}
		memcpy(scaled, data, width * height);
	}
	else
		GL_Resample8BitTexture(data, width, height, scaled, scaled_width, scaled_height);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, scaled_width, scaled_height, 0, GL_COLOR_INDEX,
	             GL_UNSIGNED_BYTE, scaled);
	if (mipmap)
	{
		int miplevel = 0;
		while (scaled_width > 1 || scaled_height > 1)
		{
			GL_MipMap8Bit(scaled, scaled_width, scaled_height);
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
				scaled_width = 1;
			if (scaled_height < 1)
				scaled_height = 1;
			miplevel++;
			glTexImage2D(GL_TEXTURE_2D, miplevel, GL_COLOR_INDEX8_EXT, scaled_width, scaled_height, 0, GL_COLOR_INDEX,
			             GL_UNSIGNED_BYTE, scaled);
		}
	}
done:;


	if (mipmap)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
}

/*
===============
GL_Upload8
===============
*/
void GL_Upload8(uint8_t* data, int width, int height, bool mipmap, bool alpha)
{
	using namespace std::string_view_literals;
	static unsigned trans[640 * 480]; // FIXME, temporary
	int i;

	int s = width * height;
	// if there are no transparent pixels, make it a 3 component
	// texture even if it was specified as otherwise
	if (alpha)
	{
		bool noalpha = true;
		for (i = 0; i < s; i++)
		{
			int p = data[i];
			if (p == 255)
				noalpha = false;
			trans[i] = d_8to24table[p];
		}

		if (alpha && noalpha)
			alpha = false;
	}
	else
	{
		if (s & 3)
			Sys_Error("GL_Upload8: s&3"sv);
		for (i = 0; i < s; i += 4)
		{
			trans[i] = d_8to24table[data[i]];
			trans[i + 1] = d_8to24table[data[i + 1]];
			trans[i + 2] = d_8to24table[data[i + 2]];
			trans[i + 3] = d_8to24table[data[i + 3]];
		}
	}

	if (VID_Is8bit() && !alpha && (data != scrap_texels[0]))
	{
		GL_Upload8_EXT(data, width, height, mipmap, alpha);
		return;
	}
	GL_Upload32(trans, width, height, mipmap, alpha);
}

/*
================
GL_LoadTexture
================
*/
int GL_LoadTexture(char* identifier, int width, int height, uint8_t* data, bool mipmap, bool alpha)
{
	using namespace std::string_view_literals;
	bool noalpha;
	int i, p, s;
	gltexture_t* glt;

	// see if the texture is allready present
	if (identifier[0])
	{
		for (i = 0, glt = gltextures; i < numgltextures; i++, glt++)
		{
			if (!strcmp(identifier, glt->identifier))
			{
				if (width != glt->width || height != glt->height)
					Sys_Error("GL_LoadTexture: cache mismatch"sv);
				return gltextures[i].texnum;
			}
		}
	}
	else
	{
		glt = &gltextures[numgltextures];
		numgltextures++;
	}

	strcpy(glt->identifier, identifier);
	glt->texnum = texture_extension_number;
	glt->width = width;
	glt->height = height;
	glt->mipmap = mipmap;

	GL_Bind(texture_extension_number);

	GL_Upload8(data, width, height, mipmap, alpha);

	texture_extension_number++;

	return texture_extension_number - 1;
}

/*
================
GL_LoadPicTexture
================
*/
int GL_LoadPicTexture(qpic_t* pic)
{
	return GL_LoadTexture((char*)"", pic->width, pic->height, pic->data, false, true);
}

/****************************************/

static GLenum oldtarget = TEXTURE0_SGIS;

void GL_SelectTexture(GLenum target)
{
	if (!gl_mtexable)
		return;
	qglSelectTextureSGIS(target);
	if (target == oldtarget)
		return;
	cnttextures[oldtarget - TEXTURE0_SGIS] = currenttexture;
	currenttexture = cnttextures[target - TEXTURE0_SGIS];
	oldtarget = target;
}
