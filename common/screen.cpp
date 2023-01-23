// screen.c -- master for refresh, status bar, console, chat, notify, etc
#include "screen.h"

#include <format>

#ifdef GLQUAKE
void GL_Set2D(void); // TODO убрать в хидер
#endif
#include <cmath>

#include "cl_main.h"
#include "cmd.h"
#include "console.h"
#include "client.h"
#include "draw.h"
#include "host.h"
#include "keys.h"
#ifdef GLQUAKE
#include "glquake.h"
#else
#include "d_iface.h"
#endif
#include "sbar.h"
#include "snd_dma.h"
#include "sys.h"
#include "menu.h"
#include "view.h"
#include "vid.h"
#include "wad.h"

// public

float scr_con_current;
float scr_conlines; // lines of console to display
int scr_fullupdate;
int clearnotify;
bool scr_disabled_for_loading;
bool scr_skipupdate;
cvar_t scr_viewsize = { (char*)"viewsize", (char*)"100", true };

// only the refresh window will be updated unless these variables are flagged 
int scr_copytop;
int scr_copyeverything;
bool block_drawing;

// private

float oldscreensize, oldfov;
cvar_t scr_fov = { (char*)"fov", (char*)"90" }; // 10 - 170
cvar_t scr_conspeed = { (char*)"scr_conspeed", (char*)"300" };
cvar_t scr_centertime = { (char*)"scr_centertime", (char*)"2" };
cvar_t scr_showram = { (char*)"showram", (char*)"1" };
cvar_t scr_showturtle = { (char*)"showturtle", (char*)"0" };
cvar_t scr_showpause = { (char*)"showpause", (char*)"1" };
cvar_t scr_printspeed = { (char*)"scr_printspeed", (char*)"8" };
bool scr_initialized; // ready to draw

qpic_t* scr_ram;
qpic_t* scr_net;
qpic_t* scr_turtle;

int clearconsole;

/*
===============================================================================
CENTER PRINTING
===============================================================================
*/

char scr_centerstring[1024];
float scr_centertime_start; // for slow victory printing
float scr_centertime_off;
int scr_center_lines;
int scr_erase_lines;
int scr_erase_center;

extern viddef_t vid; // global video state
vrect_t scr_vrect;
bool scr_drawloading;
float scr_disabled_time;
char* scr_notifystring;
bool scr_drawdialog;

#ifdef GLQUAKE
int glx, gly, glwidth, glheight;
cvar_t gl_triplebuffer = { (char*)"gl_triplebuffer", (char*)"1", true };
#endif



// private functions


void SCR_SizeUp_f(void)
{
	Cvar_SetValue((char*)"viewsize", scr_viewsize.value + 10);
	vid.recalc_refdef = 1;
}


void SCR_SizeDown_f(void)
{
	Cvar_SetValue((char*)"viewsize", scr_viewsize.value - 10);
	vid.recalc_refdef = 1;
}


void WritePCXfile(char* filename, byte* data, int width, int height,
	int rowbytes, byte* palette)
{
	typedef struct
	{
		char manufacturer;
		char version;
		char encoding;
		char bits_per_pixel;
		unsigned short xmin, ymin, xmax, ymax;
		unsigned short hres, vres;
		unsigned char palette[48];
		char reserved;
		char color_planes;
		unsigned short bytes_per_line;
		unsigned short palette_type;
		char filler[58];
		unsigned char data; // unbounded
	} pcx_t;

	int i;

	pcx_t* pcx = static_cast<pcx_t*>(Hunk_TempAlloc(width * height * 2 + 1000));
	if (pcx == NULL)
	{
		Con_Printf((char*)"SCR_ScreenShot_f: not enough memory\n");
		return;
	}

	pcx->manufacturer = 0x0a; // PCX id
	pcx->version = 5; // 256 color
	pcx->encoding = 1; // uncompressed
	pcx->bits_per_pixel = 8; // 256 color
	pcx->xmin = 0;
	pcx->ymin = 0;
	pcx->xmax = LittleShort((short)(width - 1));
	pcx->ymax = LittleShort((short)(height - 1));
	pcx->hres = LittleShort((short)width);
	pcx->vres = LittleShort((short)height);
	Q_memset(pcx->palette, 0, sizeof(pcx->palette));
	pcx->color_planes = 1; // chunky image
	pcx->bytes_per_line = LittleShort((short)width);
	pcx->palette_type = LittleShort(2); // not a grey scale
	Q_memset(pcx->filler, 0, sizeof(pcx->filler));

	// pack the image
	byte* pack = &pcx->data;

	for (i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			if ((*data & 0xc0) != 0xc0)
				*pack++ = *data++;
			else
			{
				*pack++ = 0xc1;
				*pack++ = *data++;
			}
		}

		data += rowbytes - width;
	}

	// write the palette
	*pack++ = 0x0c; // palette ID byte
	for (i = 0; i < 768; i++)
		*pack++ = *palette++;

	// write output file 
	int length = pack - (uint8_t*)pcx;
	COM_WriteFile(filename, pcx, length);
}


void SCR_ScreenShot_f(void)
{
#ifdef GLQUAKE
	char pcxname[80];
	char checkname[MAX_OSPATH];
	int i;
	// 
	// find a file name to save it to 
	// 
	strcpy(pcxname, "quake00.tga");

	for (i = 0; i <= 99; i++)
	{
		pcxname[5] = i / 10 + '0';
		pcxname[6] = i % 10 + '0';
		sprintf(checkname, (char*)"%s/%s", com_gamedir, pcxname);
		if (Sys_FileTime(checkname) == -1)
			break; // file doesn't exist
	}
	if (i == 100)
	{
		Con_Printf((char*)"SCR_ScreenShot_f: Couldn't create a PCX file\n");
		return;
	}


	byte* buffer = static_cast<byte*>(malloc(glwidth * glheight * 3 + 18));
	memset(buffer, 0, 18);
	buffer[2] = 2; // uncompressed type
	buffer[12] = glwidth & 255;
	buffer[13] = glwidth >> 8;
	buffer[14] = glheight & 255;
	buffer[15] = glheight >> 8;
	buffer[16] = 24; // pixel size

	glReadPixels(glx, gly, glwidth, glheight, GL_RGB, GL_UNSIGNED_BYTE, buffer + 18);

	// swap rgb to bgr
	int c = 18 + glwidth * glheight * 3;
	for (i = 18; i < c; i += 3)
	{
		int temp = buffer[i];
		buffer[i] = buffer[i + 2];
		buffer[i + 2] = temp;
	}
	COM_WriteFile(pcxname, buffer, glwidth * glheight * 3 + 18);

	free(buffer);
	Con_Printf((char*)"Wrote %s\n", pcxname);
#else
	int i;
	char pcxname[80];
	char checkname[MAX_OSPATH];

	// 
	// find a file name to save it to 
	// 
	strcpy(pcxname, "quake00.pcx");

	for (i = 0; i <= 99; i++)
	{
		pcxname[5] = i / 10 + '0';
		pcxname[6] = i % 10 + '0';
		sprintf(checkname, (char*)"%s/%s", com_gamedir, pcxname);
		if (Sys_FileTime(checkname) == -1)
			break; // file doesn't exist
	}
	if (i == 100)
	{
		Con_Printf((char*)"SCR_ScreenShot_f: Couldn't create a PCX file\n");
		return;
	}

	// 
	// save the pcx file 
	// 
	D_EnableBackBufferAccess(); // enable direct drawing of console to back
	//  buffer

	WritePCXfile(pcxname, vid.buffer, vid.width, vid.height, vid.rowbytes,
		host_basepal);

	D_DisableBackBufferAccess(); // for adapters that can't stay mapped in
	//  for linear writes all the time

	Con_Printf((char*)"Wrote %s\n", pcxname);
#endif
}


void SCR_DrawCenterString(void)
{
	int l;
	int y;
	int remaining;

	// the finale prints the characters one at a time
	if (cl.intermission)
		remaining = scr_printspeed.value * (cl.time - scr_centertime_start);
	else
		remaining = 9999;

	scr_erase_center = 0;
	char* start = scr_centerstring;

	if (scr_center_lines <= 4)
		y = vid.height * 0.35;
	else
		y = 48;

	do
	{
		// scan the width of the line
		for (l = 0; l < 40; l++)
			if (start[l] == '\n' || !start[l])
				break;
		int x = (vid.width - l * 8) / 2;
		for (int j = 0; j < l; j++, x += 8)
		{
			Draw_Character(x, y, start[j]);
			if (!remaining--)
				return;
		}

		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++; // skip the \n
	} while (true);
}


void SCR_CheckDrawCenterString(void)
{
	scr_copytop = 1;
	if (scr_center_lines > scr_erase_lines)
		scr_erase_lines = scr_center_lines;

	scr_centertime_off -= host_frametime;

	if (scr_centertime_off <= 0 && !cl.intermission)
		return;
	if (key_dest != key_game)
		return;

	SCR_DrawCenterString();
}


float CalcFov(float fov_x, float width, float height)
{
	using namespace std::string_view_literals;
	if (fov_x < 1 || fov_x > 179)
		Sys_Error(std::format("Bad fov: {}"sv, fov_x));

	float x = width / tan(fov_x / 360 * M_PI);

	float a = atan(height / x);

	a = a * 360 / M_PI;

	return a;
}


/*
=================
Must be called whenever vid changes
Internal use only
=================
*/
static void SCR_CalcRefdef(void)
{
#ifdef GLQUAKE
	vrect_t vrect;
	float size;
	bool full = false;


	scr_fullupdate = 0; // force a background redraw
	vid.recalc_refdef = 0;

	// force the status bar to redraw
	Sbar_Changed();

	//========================================

	// bound viewsize
	if (scr_viewsize.value < 30)
		Cvar_Set((char*)"viewsize", (char*)"30");
	if (scr_viewsize.value > 120)
		Cvar_Set((char*)"viewsize", (char*)"120");

	// bound field of view
	if (scr_fov.value < 10)
		Cvar_Set((char*)"fov", (char*)"10");
	if (scr_fov.value > 170)
		Cvar_Set((char*)"fov", (char*)"170");

	// intermission is always full screen 
	if (cl.intermission)
		size = 120;
	else
		size = scr_viewsize.value;

	if (size >= 120)
		sb_lines = 0; // no status bar at all
	else if (size >= 110)
		sb_lines = 24; // no inventory
	else
		sb_lines = 24 + 16 + 8;

	if (scr_viewsize.value >= 100.0)
	{
		full = true;
		size = 100.0;
	}
	else
		size = scr_viewsize.value;
	if (cl.intermission)
	{
		full = true;
		size = 100;
		sb_lines = 0;
	}
	size /= 100.0;

	int h = vid.height - sb_lines;

	r_refdef.vrect.width = vid.width * size;
	if (r_refdef.vrect.width < 96)
	{
		size = 96.0 / r_refdef.vrect.width;
		r_refdef.vrect.width = 96; // min for icons
	}

	r_refdef.vrect.height = vid.height * size;
	if (r_refdef.vrect.height > vid.height - sb_lines)
		r_refdef.vrect.height = vid.height - sb_lines;
	if (r_refdef.vrect.height > vid.height)
		r_refdef.vrect.height = vid.height;
	r_refdef.vrect.x = (vid.width - r_refdef.vrect.width) / 2;
	if (full)
		r_refdef.vrect.y = 0;
	else
		r_refdef.vrect.y = (h - r_refdef.vrect.height) / 2;

	r_refdef.fov_x = scr_fov.value;
	r_refdef.fov_y = CalcFov(r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height);

	scr_vrect = r_refdef.vrect;
#else
	vrect_t vrect;
	float size;

	scr_fullupdate = 0; // force a background redraw
	vid.recalc_refdef = 0;

	// force the status bar to redraw
	Sbar_Changed();

	//========================================

	// bound viewsize
	if (scr_viewsize.value < 30)
		Cvar_Set((char*)"viewsize", (char*)"30");
	if (scr_viewsize.value > 120)
		Cvar_Set((char*)"viewsize", (char*)"120");

	// bound field of view
	if (scr_fov.value < 10)
		Cvar_Set((char*)"fov", (char*)"10");
	if (scr_fov.value > 170)
		Cvar_Set((char*)"fov", (char*)"170");

	r_refdef.fov_x = scr_fov.value;
	r_refdef.fov_y = CalcFov(r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height);

	// intermission is always full screen 
	if (cl.intermission)
		size = 120;
	else
		size = scr_viewsize.value;

	if (size >= 120)
		sb_lines = 0; // no status bar at all
	else if (size >= 110)
		sb_lines = 24; // no inventory
	else
		sb_lines = 24 + 16 + 8;

	// these calculations mirror those in R_Init() for r_refdef, but take no
	// account of water warping
	vrect.x = 0;
	vrect.y = 0;
	vrect.width = vid.width;
	vrect.height = vid.height;

	R_SetVrect(&vrect, &scr_vrect, sb_lines);

	// guard against going from one mode to another that's less than half the
	// vertical resolution
	if (scr_con_current > vid.height)
		scr_con_current = vid.height;

	// notify the refresh of the change
	R_ViewChanged(&vrect, sb_lines, vid.aspect);
#endif
}


void SCR_DrawRam(void)
{
	if (!scr_showram.value)
		return;

	if (!r_cache_thrash)
		return;

	Draw_Pic(scr_vrect.x + 32, scr_vrect.y, scr_ram);
}


void SCR_DrawTurtle(void)
{
	static int count;

	if (!scr_showturtle.value)
		return;

	if (host_frametime < 0.1)
	{
		count = 0;
		return;
	}

	count++;
	if (count < 3)
		return;

	Draw_Pic(scr_vrect.x, scr_vrect.y, scr_turtle);
}


void SCR_DrawNet(void)
{
	if (realtime - cl.last_received_message < 0.3)
		return;
	if (cls.demoplayback)
		return;

	Draw_Pic(scr_vrect.x + 64, scr_vrect.y, scr_net);
}


void SCR_DrawPause(void)
{
	if (!scr_showpause.value) // turn off for screenshots
		return;

	if (!cl.paused)
		return;

	qpic_t* pic = Draw_CachePic((char*)"gfx/pause.lmp");
	Draw_Pic((vid.width - pic->width) / 2,
		(vid.height - 48 - pic->height) / 2, pic);
}


void SCR_DrawLoading(void)
{
	if (!scr_drawloading)
		return;

	qpic_t* pic = Draw_CachePic((char*)"gfx/loading.lmp");
	Draw_Pic((vid.width - pic->width) / 2,
		(vid.height - 48 - pic->height) / 2, pic);
}

void SCR_SetUpToDrawConsole(void)
{
	Con_CheckResize();

	if (scr_drawloading)
		return; // never a console with loading plaque

	// decide on the height of the console
	con_forcedup = !cl.worldmodel || cls.signon != SIGNONS;

	if (con_forcedup)
	{
		scr_conlines = vid.height; // full screen
		scr_con_current = scr_conlines;
	}
	else if (key_dest == key_console)
		scr_conlines = vid.height / 2; // half screen
	else
		scr_conlines = 0; // none visible

	if (scr_conlines < scr_con_current)
	{
		scr_con_current -= scr_conspeed.value * host_frametime;
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;
	}
	else if (scr_conlines > scr_con_current)
	{
		scr_con_current += scr_conspeed.value * host_frametime;
		if (scr_conlines < scr_con_current)
			scr_con_current = scr_conlines;
	}

	if (clearconsole++ < vid.numpages)
	{
#ifdef GLQUAKE
#else
		scr_copytop = 1;
		Draw_TileClear(0, (int)scr_con_current, vid.width, vid.height - (int)scr_con_current);
#endif
		Sbar_Changed();
	}
	else if (clearnotify++ < vid.numpages)
	{
#ifdef GLQUAKE
#else
		scr_copytop = 1;
		Draw_TileClear(0, 0, vid.width, con_notifylines);
#endif
	}
	else
		con_notifylines = 0;
}


void SCR_DrawConsole(void)
{
	if (scr_con_current)
	{
		scr_copyeverything = 1;
		Con_DrawConsole(scr_con_current, true);
		clearconsole = 0;
	}
	else
	{
		if (key_dest == key_game || key_dest == key_message)
			Con_DrawNotify(); // only draw notify in game
	}
}


void SCR_DrawNotifyString(void)
{
	int l;

	char* start = scr_notifystring;

	int y = vid.height * 0.35;

	do
	{
		// scan the width of the line
		for (l = 0; l < 40; l++)
			if (start[l] == '\n' || !start[l])
				break;
		int x = (vid.width - l * 8) / 2;
		for (int j = 0; j < l; j++, x += 8)
			Draw_Character(x, y, start[j]);

		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++; // skip the \n
	} while (true);
}

// public functions

void SCR_Init(void)
{
	Cvar_RegisterVariable(&scr_fov);
	Cvar_RegisterVariable(&scr_viewsize);
	Cvar_RegisterVariable(&scr_conspeed);
	Cvar_RegisterVariable(&scr_showram);
	Cvar_RegisterVariable(&scr_showturtle);
	Cvar_RegisterVariable(&scr_showpause);
	Cvar_RegisterVariable(&scr_centertime);
	Cvar_RegisterVariable(&scr_printspeed);
#ifdef GLQUAKE
	Cvar_RegisterVariable(&gl_triplebuffer);
#endif
	//
	// register our commands
	//
	Cmd_AddCommand((char*)"screenshot", SCR_ScreenShot_f);
	Cmd_AddCommand((char*)"sizeup", SCR_SizeUp_f);
	Cmd_AddCommand((char*)"sizedown", SCR_SizeDown_f);

	scr_ram = Draw_PicFromWad((char*)"ram");
	scr_net = Draw_PicFromWad((char*)"net");
	scr_turtle = Draw_PicFromWad((char*)"turtle");

	scr_initialized = true;
}


/*
This is called every frame, and can also be called explicitly to flush
text to the screen.

WARNING: be very careful calling this_ from elsewhere, because the refresh
needs almost the entire 256k of stack space!
*/
void SCR_UpdateScreen(void)
{
#ifdef GLQUAKE
	static float oldscr_viewsize;
	vrect_t vrect;

	if (block_drawing)
		return;

	vid.numpages = 2 + gl_triplebuffer.value;

	scr_copytop = 0;
	scr_copyeverything = 0;

	if (scr_disabled_for_loading)
	{
		if (realtime - scr_disabled_time > 60)
		{
			scr_disabled_for_loading = false;
			Con_Printf((char*)"load failed.\n");
		}
		else
			return;
	}

	if (!scr_initialized || !con_initialized)
		return; // not initialized yet


	GL_BeginRendering(&glx, &gly, &glwidth, &glheight);

	//
	// determine size of refresh window
	//
	if (oldfov != scr_fov.value)
	{
		oldfov = scr_fov.value;
		vid.recalc_refdef = true;
	}

	if (oldscreensize != scr_viewsize.value)
	{
		oldscreensize = scr_viewsize.value;
		vid.recalc_refdef = true;
	}

	if (vid.recalc_refdef)
		SCR_CalcRefdef();

	//
	// do 3D refresh drawing, and then update the screen
	//
	SCR_SetUpToDrawConsole();

	V_RenderView();

	GL_Set2D();

	//
	// draw any areas not covered by the refresh
	//
	//SCR_TileClear();
	//void SCR_TileClear(void)
	do {
		if (r_refdef.vrect.x > 0)
		{
			// left
			Draw_TileClear(0, 0, r_refdef.vrect.x, vid.height - sb_lines);
			// right
			Draw_TileClear(r_refdef.vrect.x + r_refdef.vrect.width, 0,
				vid.width - r_refdef.vrect.x + r_refdef.vrect.width,
				vid.height - sb_lines);
		}
		if (r_refdef.vrect.y > 0)
		{
			// top
			Draw_TileClear(r_refdef.vrect.x, 0,
				r_refdef.vrect.x + r_refdef.vrect.width,
				r_refdef.vrect.y);
			// bottom
			Draw_TileClear(r_refdef.vrect.x,
				r_refdef.vrect.y + r_refdef.vrect.height,
				r_refdef.vrect.width,
				vid.height - sb_lines -
				(r_refdef.vrect.height + r_refdef.vrect.y));
		}
	}while (false);

	if (scr_drawdialog)
	{
		Sbar_Draw();
		Draw_FadeScreen();
		SCR_DrawNotifyString();
		scr_copyeverything = true;
	}
	else if (scr_drawloading)
	{
		SCR_DrawLoading();
		Sbar_Draw();
	}
	else if (cl.intermission == 1 && key_dest == key_game)
	{
		Sbar_IntermissionOverlay();
	}
	else if (cl.intermission == 2 && key_dest == key_game)
	{
		Sbar_FinaleOverlay();
		SCR_CheckDrawCenterString();
	}
	else
	{
		if (crosshair.value)
			Draw_Character(scr_vrect.x + scr_vrect.width / 2, scr_vrect.y + scr_vrect.height / 2, '+');

		SCR_DrawRam();
		SCR_DrawNet();
		SCR_DrawTurtle();
		SCR_DrawPause();
		SCR_CheckDrawCenterString();
		Sbar_Draw();
		SCR_DrawConsole();
		M_Draw();
	}

	V_UpdatePalette();

	GL_EndRendering();
#else
	static float oldscr_viewsize;
	static float oldlcd_x;
	vrect_t vrect;

	if (scr_skipupdate || block_drawing)
		return;

	scr_copytop = 0;
	scr_copyeverything = 0;

	if (scr_disabled_for_loading)
	{
		if (realtime - scr_disabled_time > 60)
		{
			scr_disabled_for_loading = false;
			Con_Printf((char*)"load failed.\n");
		}
		else
			return;
	}

	if (cls.state == ca_dedicated)
		return; // stdout only

	if (!scr_initialized || !con_initialized)
		return; // not initialized yet

	if (scr_viewsize.value != oldscr_viewsize)
	{
		oldscr_viewsize = scr_viewsize.value;
		vid.recalc_refdef = 1;
	}

	//
	// check for vid changes
	//
	if (oldfov != scr_fov.value)
	{
		oldfov = scr_fov.value;
		vid.recalc_refdef = true;
	}

	if (oldlcd_x != lcd_x.value)
	{
		oldlcd_x = lcd_x.value;
		vid.recalc_refdef = true;
	}

	if (oldscreensize != scr_viewsize.value)
	{
		oldscreensize = scr_viewsize.value;
		vid.recalc_refdef = true;
	}

	if (vid.recalc_refdef)
	{
		// something changed, so reorder the screen
		SCR_CalcRefdef();
	}

	//
	// do 3D refresh drawing, and then update the screen
	//
	D_EnableBackBufferAccess(); // of all overlay stuff if drawing directly

	if (scr_fullupdate++ < vid.numpages)
	{
		// clear the entire screen
		scr_copyeverything = 1;
		Draw_TileClear(0, 0, vid.width, vid.height);
		Sbar_Changed();
	}

	vrect_t* pconupdate = NULL;


	SCR_SetUpToDrawConsole();
	//SCR_EraseCenterString();
	//void SCR_EraseCenterString(void)
	do {
		int y;

		if (scr_erase_center++ > vid.numpages)
		{
			scr_erase_lines = 0;
			break;
		}

		if (scr_center_lines <= 4)
			y = vid.height * 0.35;
		else
			y = 48;

		scr_copytop = 1;
		Draw_TileClear(0, y, vid.width, 8 * scr_erase_lines);
	} while (0);


	D_DisableBackBufferAccess(); // for adapters that can't stay mapped in
	//  for linear writes all the time

	VID_LockBuffer();

	V_RenderView();

	VID_UnlockBuffer();

	D_EnableBackBufferAccess(); // of all overlay stuff if drawing directly

	if (scr_drawdialog)
	{
		Sbar_Draw();
		Draw_FadeScreen();
		SCR_DrawNotifyString();
		scr_copyeverything = true;
	}
	else if (scr_drawloading)
	{
		SCR_DrawLoading();
		Sbar_Draw();
	}
	else if (cl.intermission == 1 && key_dest == key_game)
	{
		Sbar_IntermissionOverlay();
	}
	else if (cl.intermission == 2 && key_dest == key_game)
	{
		Sbar_FinaleOverlay();
		SCR_CheckDrawCenterString();
	}
	else if (cl.intermission == 3 && key_dest == key_game)
	{
		SCR_CheckDrawCenterString();
	}
	else
	{
		SCR_DrawRam();
		SCR_DrawNet();
		SCR_DrawTurtle();
		SCR_DrawPause();
		SCR_CheckDrawCenterString();
		Sbar_Draw();
		SCR_DrawConsole();
		M_Draw();
	}

	D_DisableBackBufferAccess(); // for adapters that can't stay mapped in
	//  for linear writes all the time
	if (pconupdate)
	{
		D_UpdateRects(pconupdate);
	}

	V_UpdatePalette();

	//
	// update one of three areas
	//

	if (scr_copyeverything)
	{
		vrect.x = 0;
		vrect.y = 0;
		vrect.width = vid.width;
		vrect.height = vid.height;
		vrect.pnext = 0;

		VID_Update(&vrect);
	}
	else if (scr_copytop)
	{
		vrect.x = 0;
		vrect.y = 0;
		vrect.width = vid.width;
		vrect.height = vid.height - sb_lines;
		vrect.pnext = 0;

		VID_Update(&vrect);
	}
	else
	{
		vrect.x = scr_vrect.x;
		vrect.y = scr_vrect.y;
		vrect.width = scr_vrect.width;
		vrect.height = scr_vrect.height;
		vrect.pnext = 0;

		VID_Update(&vrect);
	}
#endif
}


/*
===============
Brings the console down and fades the palettes back to normal
================
*/
void SCR_BringDownConsole(void)
{
	scr_centertime_off = 0;

	for (int i = 0; i < 20 && scr_conlines != scr_con_current; i++)
		SCR_UpdateScreen();

	cl.cshifts[0].percent = 0; // no area contents palette on next frame
	VID_SetPalette(host_basepal);
}


/*
==============
Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint(char* str)
{
	strncpy(scr_centerstring, str, sizeof(scr_centerstring) - 1);
	scr_centertime_off = scr_centertime.value;
	scr_centertime_start = cl.time;

	// count the number of lines for centering
	scr_center_lines = 1;
	while (*str)
	{
		if (*str == '\n')
			scr_center_lines++;
		str++;
	}
}


void SCR_BeginLoadingPlaque(void)
{
	S_StopAllSounds(true);

	if (cls.state != ca_connected)
		return;
	if (cls.signon != SIGNONS)
		return;

	// redraw with no console and the loading plaque
	Con_ClearNotify();
	scr_centertime_off = 0;
	scr_con_current = 0;

	scr_drawloading = true;
	scr_fullupdate = 0;
	Sbar_Changed();
	SCR_UpdateScreen();
	scr_drawloading = false;

	scr_disabled_for_loading = true;
	scr_disabled_time = realtime;
	scr_fullupdate = 0;
}


void SCR_EndLoadingPlaque(void)
{
	scr_disabled_for_loading = false;
	scr_fullupdate = 0;
	Con_ClearNotify();
}


int SCR_ModalMessage(char* text)
{
	if (cls.state == ca_dedicated)
		return true;

	scr_notifystring = text;

	// draw a fresh screen
	scr_fullupdate = 0;
	scr_drawdialog = true;
	SCR_UpdateScreen();
	scr_drawdialog = false;

	S_ClearBuffer(); // so dma doesn't loop current sound

	do
	{
		key_count = -1; // wait for a key down and up
		Sys_SendKeyEvents();
	} while (key_lastpress != 'y' && key_lastpress != 'n' && key_lastpress != K_ESCAPE);

	scr_fullupdate = 0;
	SCR_UpdateScreen();

	return key_lastpress == 'y';
}


void SCR_UpdateWholeScreen(void)
{
	scr_fullupdate = 0;
	SCR_UpdateScreen();
}

