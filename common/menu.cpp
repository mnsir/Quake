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
#include "menu.h"

#include "cl_main.h"
#include "cmd.h"
#include "common.h"
#include "console.h"
#include "draw.h"
#include "host.h"
#include "host_cmd.h"
#include "keys.h"
#include "render.h"
#include "screen.h"
#include "snd_dma.h"
#include "sv_main.h"
#include "vid.h"
#include "view.h"


void (*vid_menudrawfn)(void);
void (*vid_menukeyfn)(int key);

MState m_state;

void M_Menu_Main_f(void);
void M_Menu_SinglePlayer_f(void);
void M_Menu_Load_f(void);
void M_Menu_Save_f(void);
void M_Menu_MultiPlayer_f(void);
void M_Menu_Setup_f(void);
void M_Menu_Net_f(void);
void M_Menu_Options_f(void);
void M_Menu_Keys_f(void);
void M_Menu_Video_f(void);
void M_Menu_Help_f(void);
void M_Menu_Quit_f(void);
void M_Menu_SerialConfig_f(void);
void M_Menu_ModemConfig_f(void);
void M_Menu_LanConfig_f(void);
void M_Menu_GameOptions_f(void);
void M_Menu_Search_f(void);
void M_Menu_ServerList_f(void);

void M_Main_Draw(void);
void M_SinglePlayer_Draw(void);
void M_Load_Draw(void);
void M_Save_Draw(void);
void M_MultiPlayer_Draw(void);
void M_Setup_Draw(void);
void M_Net_Draw(void);
void M_Options_Draw(void);
void M_Keys_Draw(void);
void M_Video_Draw(void);
void M_Help_Draw(void);
void M_Quit_Draw(void);
void M_SerialConfig_Draw(void);
void M_ModemConfig_Draw(void);
void M_LanConfig_Draw(void);
void M_GameOptions_Draw(void);
void M_Search_Draw(void);
void M_ServerList_Draw(void);

void M_Main_Key(int key);
void M_SinglePlayer_Key(int key);
void M_Load_Key(int key);
void M_Save_Key(int key);
void M_MultiPlayer_Key(int key);
void M_Setup_Key(int key);
void M_Net_Key(int key);
void M_Options_Key(int key);
void M_Keys_Key(int key);
void M_Video_Key(int key);
void M_Help_Key(int key);
void M_Quit_Key(int key);
void M_SerialConfig_Key(int key);
void M_ModemConfig_Key(int key);
void M_LanConfig_Key(int key);
void M_GameOptions_Key(int key);
void M_Search_Key(int key);
void M_ServerList_Key(int key);

qboolean m_entersound; // play after drawing a frame, so caching
// won't disrupt the sound
qboolean m_recursiveDraw;

int m_return_state;
qboolean m_return_onerror;
char m_return_reason[32];

#define StartingGame (m_multiplayer_cursor == 1)
#define JoiningGame (m_multiplayer_cursor == 0)
#define SerialConfig (m_net_cursor == 0)
#define DirectConfig (m_net_cursor == 1)
#define IPXConfig (m_net_cursor == 2)
#define TCPIPConfig (m_net_cursor == 3)

void M_ConfigureNetSubsystem(void);

/*
================
M_DrawCharacter

Draws one solid graphics character
================
*/
void M_DrawCharacter(int cx, int line, int num)
{
	Draw_Character(cx + ((vid.width - 320) >> 1), line, num);
}

void M_Print(int cx, int cy, char* str)
{
	while (*str)
	{
		M_DrawCharacter(cx, cy, (*str) + 128);
		str++;
		cx += 8;
	}
}

void M_PrintWhite(int cx, int cy, char* str)
{
	while (*str)
	{
		M_DrawCharacter(cx, cy, *str);
		str++;
		cx += 8;
	}
}

void M_DrawTransPic(int x, int y, qpic_t* pic)
{
	Draw_TransPic(x + ((vid.width - 320) >> 1), y, pic);
}

void M_DrawPic(int x, int y, qpic_t* pic)
{
	Draw_Pic(x + ((vid.width - 320) >> 1), y, pic);
}

byte identityTable[256];
byte translationTable[256];

void M_BuildTranslationTable(int top, int bottom)
{
	int j;

	for (j = 0; j < 256; j++)
		identityTable[j] = j;
	byte* dest = translationTable;
	byte* source = identityTable;
	memcpy(dest, source, 256);

	if (top < 128) // the artists made some backwards ranges.  sigh.
		memcpy(dest + TOP_RANGE, source + top, 16);
	else
		for (j = 0; j < 16; j++)
			dest[TOP_RANGE + j] = source[top + 15 - j];

	if (bottom < 128)
		memcpy(dest + BOTTOM_RANGE, source + bottom, 16);
	else
		for (j = 0; j < 16; j++)
			dest[BOTTOM_RANGE + j] = source[bottom + 15 - j];
}


void M_DrawTransPicTranslate(int x, int y, qpic_t* pic)
{
	Draw_TransPicTranslate(x + ((vid.width - 320) >> 1), y, pic, translationTable);
}


void M_DrawTextBox(int x, int y, int width, int lines)
{
	int n;

	// draw left side
	int cx = x;
	int cy = y;
	qpic_t* p = Draw_CachePic((char*)"gfx/box_tl.lmp");
	M_DrawTransPic(cx, cy, p);
	p = Draw_CachePic((char*)"gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic(cx, cy, p);
	}
	p = Draw_CachePic((char*)"gfx/box_bl.lmp");
	M_DrawTransPic(cx, cy + 8, p);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		p = Draw_CachePic((char*)"gfx/box_tm.lmp");
		M_DrawTransPic(cx, cy, p);
		p = Draw_CachePic((char*)"gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = Draw_CachePic((char*)"gfx/box_mm2.lmp");
			M_DrawTransPic(cx, cy, p);
		}
		p = Draw_CachePic((char*)"gfx/box_bm.lmp");
		M_DrawTransPic(cx, cy + 8, p);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic((char*)"gfx/box_tr.lmp");
	M_DrawTransPic(cx, cy, p);
	p = Draw_CachePic((char*)"gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic(cx, cy, p);
	}
	p = Draw_CachePic((char*)"gfx/box_br.lmp");
	M_DrawTransPic(cx, cy + 8, p);
}

//=============================================================================

int m_save_demonum;

/*
================
M_ToggleMenu_f
================
*/
void M_ToggleMenu_f(void)
{
	m_entersound = true;

	if (key_dest == key_menu)
	{
		if (m_state != m_main)
		{
			M_Menu_Main_f();
			return;
		}
		key_dest = key_game;
		m_state = m_none;
		return;
	}
	if (key_dest == key_console)
	{
		Con_ToggleConsole_f();
	}
	else
	{
		M_Menu_Main_f();
	}
}


//=============================================================================
/* MAIN MENU */

int m_main_cursor;
#define MAIN_ITEMS 5


void M_Menu_Main_f(void)
{
	if (key_dest != key_menu)
	{
		m_save_demonum = cls.demonum;
		cls.demonum = -1;
	}
	key_dest = key_menu;
	m_state = m_main;
	m_entersound = true;
}


void M_Main_Draw(void)
{
	M_DrawTransPic(16, 4, Draw_CachePic((char*)"gfx/qplaque.lmp"));
	qpic_t* p = Draw_CachePic((char*)"gfx/ttl_main.lmp");
	M_DrawPic((320 - p->width) / 2, 4, p);
	M_DrawTransPic(72, 32, Draw_CachePic((char*)"gfx/mainmenu.lmp"));

	int f = (int)(host_time * 10) % 6;

	M_DrawTransPic(54, 32 + m_main_cursor * 20, Draw_CachePic(va((char*)"gfx/menudot%i.lmp", f + 1)));
}


void M_Main_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
		key_dest = key_game;
		m_state = m_none;
		cls.demonum = m_save_demonum;
		if (cls.demonum != -1 && !cls.demoplayback && cls.state != ca_connected)
			CL_NextDemo();
		break;

	case K_DOWNARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_main_cursor)
		{
		case 0:
			M_Menu_SinglePlayer_f();
			break;

		case 1:
			M_Menu_MultiPlayer_f();
			break;

		case 2:
			M_Menu_Options_f();
			break;

		case 3:
			M_Menu_Help_f();
			break;

		case 4:
			M_Menu_Quit_f();
			break;
		}
	}
}

//=============================================================================
/* SINGLE PLAYER MENU */

int m_singleplayer_cursor;
#define SINGLEPLAYER_ITEMS 3


void M_Menu_SinglePlayer_f(void)
{
	key_dest = key_menu;
	m_state = m_singleplayer;
	m_entersound = true;
}


void M_SinglePlayer_Draw(void)
{
	M_DrawTransPic(16, 4, Draw_CachePic((char*)"gfx/qplaque.lmp"));
	qpic_t* p = Draw_CachePic((char*)"gfx/ttl_sgl.lmp");
	M_DrawPic((320 - p->width) / 2, 4, p);
	M_DrawTransPic(72, 32, Draw_CachePic((char*)"gfx/sp_menu.lmp"));

	int f = (int)(host_time * 10) % 6;

	M_DrawTransPic(54, 32 + m_singleplayer_cursor * 20, Draw_CachePic(va((char*)"gfx/menudot%i.lmp", f + 1)));
}


void M_SinglePlayer_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f();
		break;

	case K_DOWNARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		if (++m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
			m_singleplayer_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		if (--m_singleplayer_cursor < 0)
			m_singleplayer_cursor = SINGLEPLAYER_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_singleplayer_cursor)
		{
		case 0:
			if (sv.active)
				if (!SCR_ModalMessage((char*)"Are you sure you want to\nstart a new_ game?\n"))
					break;
			key_dest = key_game;
			if (sv.active)
				Cbuf_AddText((char*)"disconnect\n");
			Cbuf_AddText((char*)"maxplayers 1\n");
			Cbuf_AddText((char*)"map start\n");
			break;

		case 1:
			M_Menu_Load_f();
			break;

		case 2:
			M_Menu_Save_f();
			break;
		}
	}
}

//=============================================================================
/* LOAD/SAVE MENU */

int load_cursor; // 0 < load_cursor < MAX_SAVEGAMES

#define MAX_SAVEGAMES 12
char m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH + 1];
int loadable[MAX_SAVEGAMES];

void M_ScanSaves(void)
{
	char name[MAX_OSPATH];
	int version;

	for (int i = 0; i < MAX_SAVEGAMES; i++)
	{
		strcpy(m_filenames[i], "--- UNUSED SLOT ---");
		loadable[i] = false;
		sprintf(name, (char*)"%s/s%i.sav", com_gamedir, i);
		FILE* f = fopen(name, "r");
		if (!f)
			continue;
		fscanf(f, (char*)"%i\n", &version);
		fscanf(f, (char*)"%79s\n", name);
		strncpy(m_filenames[i], name, sizeof(m_filenames[i]) - 1);

		// change _ back to space
		for (int j = 0; j < SAVEGAME_COMMENT_LENGTH; j++)
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';
		loadable[i] = true;
		fclose(f);
	}
}

void M_Menu_Load_f(void)
{
	m_entersound = true;
	m_state = m_load;
	key_dest = key_menu;
	M_ScanSaves();
}


void M_Menu_Save_f(void)
{
	if (!sv.active)
		return;
	if (cl.intermission)
		return;
	if (svs.maxclients != 1)
		return;
	m_entersound = true;
	m_state = m_save;
	key_dest = key_menu;
	M_ScanSaves();
}


void M_Load_Draw(void)
{
	qpic_t* p = Draw_CachePic((char*)"gfx/p_load.lmp");
	M_DrawPic((320 - p->width) / 2, 4, p);

	for (int i = 0; i < MAX_SAVEGAMES; i++)
		M_Print(16, 32 + 8 * i, m_filenames[i]);

	// line cursor
	M_DrawCharacter(8, 32 + load_cursor * 8, 12 + ((int)(realtime * 4) & 1));
}


void M_Save_Draw(void)
{
	qpic_t* p = Draw_CachePic((char*)"gfx/p_save.lmp");
	M_DrawPic((320 - p->width) / 2, 4, p);

	for (int i = 0; i < MAX_SAVEGAMES; i++)
		M_Print(16, 32 + 8 * i, m_filenames[i]);

	// line cursor
	M_DrawCharacter(8, 32 + load_cursor * 8, 12 + ((int)(realtime * 4) & 1));
}


void M_Load_Key(int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f();
		break;

	case K_ENTER:
		S_LocalSound((char*)"misc/menu2.wav");
		if (!loadable[load_cursor])
			return;
		m_state = m_none;
		key_dest = key_game;

	// Host_Loadgame_f can't bring up the loading plaque because too much
	// stack space has been used, so do it now
		SCR_BeginLoadingPlaque();

	// issue the load command
		Cbuf_AddText(va((char*)"load s%i\n", load_cursor));
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES - 1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}


void M_Save_Key(int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f();
		break;

	case K_ENTER:
		m_state = m_none;
		key_dest = key_game;
		Cbuf_AddText(va((char*)"save s%i\n", load_cursor));
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES - 1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}

//=============================================================================
/* MULTIPLAYER MENU */

int m_multiplayer_cursor;
#define MULTIPLAYER_ITEMS 3


void M_Menu_MultiPlayer_f(void)
{
	key_dest = key_menu;
	m_state = m_multiplayer;
	m_entersound = true;
}


void M_MultiPlayer_Draw(void)
{
	M_DrawTransPic(16, 4, Draw_CachePic((char*)"gfx/qplaque.lmp"));
	qpic_t* p = Draw_CachePic((char*)"gfx/p_multi.lmp");
	M_DrawPic((320 - p->width) / 2, 4, p);
	M_DrawTransPic(72, 32, Draw_CachePic((char*)"gfx/mp_menu.lmp"));

	int f = (int)(host_time * 10) % 6;

	M_DrawTransPic(54, 32 + m_multiplayer_cursor * 20, Draw_CachePic(va((char*)"gfx/menudot%i.lmp", f + 1)));

	if (serialAvailable || ipxAvailable || tcpipAvailable)
		return;
	M_PrintWhite((320 / 2) - ((27 * 8) / 2), 148, (char*)"No Communications Available");
}


void M_MultiPlayer_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f();
		break;

	case K_DOWNARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		if (++m_multiplayer_cursor >= MULTIPLAYER_ITEMS)
			m_multiplayer_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		if (--m_multiplayer_cursor < 0)
			m_multiplayer_cursor = MULTIPLAYER_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;
		switch (m_multiplayer_cursor)
		{
		case 0:
			if (serialAvailable || ipxAvailable || tcpipAvailable)
				M_Menu_Net_f();
			break;

		case 1:
			if (serialAvailable || ipxAvailable || tcpipAvailable)
				M_Menu_Net_f();
			break;

		case 2:
			M_Menu_Setup_f();
			break;
		}
	}
}

//=============================================================================
/* SETUP MENU */

int setup_cursor = 4;
int setup_cursor_table[] = {40, 56, 80, 104, 140};

char setup_hostname[16];
char setup_myname[16];
int setup_oldtop;
int setup_oldbottom;
int setup_top;
int setup_bottom;

#define NUM_SETUP_CMDS 5

void M_Menu_Setup_f(void)
{
	key_dest = key_menu;
	m_state = m_setup;
	m_entersound = true;
	Q_strcpy(setup_myname, cl_name.string);
	Q_strcpy(setup_hostname, hostname.string);
	setup_top = setup_oldtop = ((int)cl_color.value) >> 4;
	setup_bottom = setup_oldbottom = ((int)cl_color.value) & 15;
}


void M_Setup_Draw(void)
{
	M_DrawTransPic(16, 4, Draw_CachePic((char*)"gfx/qplaque.lmp"));
	qpic_t* p = Draw_CachePic((char*)"gfx/p_multi.lmp");
	M_DrawPic((320 - p->width) / 2, 4, p);

	M_Print(64, 40, (char*)"Hostname");
	M_DrawTextBox(160, 32, 16, 1);
	M_Print(168, 40, setup_hostname);

	M_Print(64, 56, (char*)"Your name");
	M_DrawTextBox(160, 48, 16, 1);
	M_Print(168, 56, setup_myname);

	M_Print(64, 80, (char*)"Shirt color");
	M_Print(64, 104, (char*)"Pants color");

	M_DrawTextBox(64, 140 - 8, 14, 1);
	M_Print(72, 140, (char*)"Accept Changes");

	p = Draw_CachePic((char*)"gfx/bigbox.lmp");
	M_DrawTransPic(160, 64, p);
	p = Draw_CachePic((char*)"gfx/menuplyr.lmp");
	M_BuildTranslationTable(setup_top * 16, setup_bottom * 16);
	M_DrawTransPicTranslate(172, 72, p);

	M_DrawCharacter(56, setup_cursor_table[setup_cursor], 12 + ((int)(realtime * 4) & 1));

	if (setup_cursor == 0)
		M_DrawCharacter(168 + 8 * strlen(setup_hostname), setup_cursor_table[setup_cursor],
		                10 + ((int)(realtime * 4) & 1));

	if (setup_cursor == 1)
		M_DrawCharacter(168 + 8 * strlen(setup_myname), setup_cursor_table[setup_cursor],
		                10 + ((int)(realtime * 4) & 1));
}


void M_Setup_Key(int k)
{
	int l;

	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f();
		break;

	case K_UPARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		setup_cursor--;
		if (setup_cursor < 0)
			setup_cursor = NUM_SETUP_CMDS - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		setup_cursor++;
		if (setup_cursor >= NUM_SETUP_CMDS)
			setup_cursor = 0;
		break;

	case K_LEFTARROW:
		if (setup_cursor < 2)
			return;
		S_LocalSound((char*)"misc/menu3.wav");
		if (setup_cursor == 2)
			setup_top = setup_top - 1;
		if (setup_cursor == 3)
			setup_bottom = setup_bottom - 1;
		break;
	case K_RIGHTARROW:
		if (setup_cursor < 2)
			return;
	forward:
		S_LocalSound((char*)"misc/menu3.wav");
		if (setup_cursor == 2)
			setup_top = setup_top + 1;
		if (setup_cursor == 3)
			setup_bottom = setup_bottom + 1;
		break;

	case K_ENTER:
		if (setup_cursor == 0 || setup_cursor == 1)
			return;

		if (setup_cursor == 2 || setup_cursor == 3)
			goto forward;

	// setup_cursor == 4 (OK)
		if (Q_strcmp(cl_name.string, setup_myname) != 0)
			Cbuf_AddText(va((char*)"name \"%s\"\n", setup_myname));
		if (Q_strcmp(hostname.string, setup_hostname) != 0)
			Cvar_Set((char*)"hostname", setup_hostname);
		if (setup_top != setup_oldtop || setup_bottom != setup_oldbottom)
			Cbuf_AddText(va((char*)"color %i %i\n", setup_top, setup_bottom));
		m_entersound = true;
		M_Menu_MultiPlayer_f();
		break;

	case K_BACKSPACE:
		if (setup_cursor == 0)
		{
			if (strlen(setup_hostname))
				setup_hostname[strlen(setup_hostname) - 1] = 0;
		}

		if (setup_cursor == 1)
		{
			if (strlen(setup_myname))
				setup_myname[strlen(setup_myname) - 1] = 0;
		}
		break;

	default:
		if (k < 32 || k > 127)
			break;
		if (setup_cursor == 0)
		{
			l = strlen(setup_hostname);
			if (l < 15)
			{
				setup_hostname[l + 1] = 0;
				setup_hostname[l] = k;
			}
		}
		if (setup_cursor == 1)
		{
			l = strlen(setup_myname);
			if (l < 15)
			{
				setup_myname[l + 1] = 0;
				setup_myname[l] = k;
			}
		}
	}

	if (setup_top > 13)
		setup_top = 0;
	if (setup_top < 0)
		setup_top = 13;
	if (setup_bottom > 13)
		setup_bottom = 0;
	if (setup_bottom < 0)
		setup_bottom = 13;
}

//=============================================================================
/* NET MENU */

int m_net_cursor;
int m_net_items;
int m_net_saveHeight;

char* net_helpMessage[] =
{
	/* .........1.........2.... */
	(char*)"                        ",
	(char*)" Two computers connected",
	(char*)"   through two modems.  ",
	(char*)"                        ",

	(char*)"                        ",
	(char*)" Two computers connected",
	(char*)" by a null-modem cable. ",
	(char*)"                        ",

	(char*)" Novell network LANs    ",
	(char*)" or Windows 95 DOS-box. ",
	(char*)"                        ",
	(char*)"(LAN=Local Area Network)",

	(char*)" Commonly used to play  ",
	(char*)" over the Internet, but ",
	(char*)" also used on a Local   ",
	(char*)" Area Network.          "
};

void M_Menu_Net_f(void)
{
	key_dest = key_menu;
	m_state = m_net;
	m_entersound = true;
	m_net_items = 4;

	if (m_net_cursor >= m_net_items)
		m_net_cursor = 0;
	m_net_cursor--;
	M_Net_Key(K_DOWNARROW);
}


void M_Net_Draw(void)
{
	M_DrawTransPic(16, 4, Draw_CachePic((char*)"gfx/qplaque.lmp"));
	qpic_t* p = Draw_CachePic((char*)"gfx/p_multi.lmp");
	M_DrawPic((320 - p->width) / 2, 4, p);

	int f = 32;

	if (serialAvailable)
	{
		p = Draw_CachePic((char*)"gfx/netmen1.lmp");
	}
	else
	{
		p = NULL;
	}

	if (p)
		M_DrawTransPic(72, f, p);

	f += 19;

	if (serialAvailable)
	{
		p = Draw_CachePic((char*)"gfx/netmen2.lmp");
	}
	else
	{
		p = NULL;
	}

	if (p)
		M_DrawTransPic(72, f, p);

	f += 19;
	if (ipxAvailable)
		p = Draw_CachePic((char*)"gfx/netmen3.lmp");
	else
		p = Draw_CachePic((char*)"gfx/dim_ipx.lmp");
	M_DrawTransPic(72, f, p);

	f += 19;
	if (tcpipAvailable)
		p = Draw_CachePic((char*)"gfx/netmen4.lmp");
	else
		p = Draw_CachePic((char*)"gfx/dim_tcp.lmp");
	M_DrawTransPic(72, f, p);

	if (m_net_items == 5) // JDC, could just be removed
	{
		f += 19;
		p = Draw_CachePic((char*)"gfx/netmen5.lmp");
		M_DrawTransPic(72, f, p);
	}

	f = (320 - 26 * 8) / 2;
	M_DrawTextBox(f, 134, 24, 4);
	f += 8;
	M_Print(f, 142, net_helpMessage[m_net_cursor * 4 + 0]);
	M_Print(f, 150, net_helpMessage[m_net_cursor * 4 + 1]);
	M_Print(f, 158, net_helpMessage[m_net_cursor * 4 + 2]);
	M_Print(f, 166, net_helpMessage[m_net_cursor * 4 + 3]);

	f = (int)(host_time * 10) % 6;
	M_DrawTransPic(54, 32 + m_net_cursor * 20, Draw_CachePic(va((char*)"gfx/menudot%i.lmp", f + 1)));
}


void M_Net_Key(int k)
{
again:
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f();
		break;

	case K_DOWNARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		if (++m_net_cursor >= m_net_items)
			m_net_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		if (--m_net_cursor < 0)
			m_net_cursor = m_net_items - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_net_cursor)
		{
		case 0:
			M_Menu_SerialConfig_f();
			break;

		case 1:
			M_Menu_SerialConfig_f();
			break;

		case 2:
			M_Menu_LanConfig_f();
			break;

		case 3:
			M_Menu_LanConfig_f();
			break;

		case 4:
			// multiprotocol
			break;
		}
	}

	if (m_net_cursor == 0 && !serialAvailable)
		goto again;
	if (m_net_cursor == 1 && !serialAvailable)
		goto again;
	if (m_net_cursor == 2 && !ipxAvailable)
		goto again;
	if (m_net_cursor == 3 && !tcpipAvailable)
		goto again;
}

//=============================================================================
/* OPTIONS MENU */

#define OPTIONS_ITEMS 14

#define SLIDER_RANGE 10

int options_cursor;

void M_Menu_Options_f(void)
{
	key_dest = key_menu;
	m_state = m_options;
	m_entersound = true;
	
	if ((options_cursor == 13) && (modestate != MS_WINDOWED))
	{
		options_cursor = 0;
	}
}


void M_AdjustSliders(int dir)
{
	S_LocalSound((char*)"misc/menu3.wav");

	switch (options_cursor)
	{
	case 3: // screen size
		scr_viewsize.value += dir * 10;
		if (scr_viewsize.value < 30)
			scr_viewsize.value = 30;
		if (scr_viewsize.value > 120)
			scr_viewsize.value = 120;
		Cvar_SetValue((char*)"viewsize", scr_viewsize.value);
		break;
	case 4: // gamma
		v_gamma.value -= dir * 0.05;
		if (v_gamma.value < 0.5)
			v_gamma.value = 0.5;
		if (v_gamma.value > 1)
			v_gamma.value = 1;
		Cvar_SetValue((char*)"gamma", v_gamma.value);
		break;
	case 5: // mouse speed
		sensitivity.value += dir * 0.5;
		if (sensitivity.value < 1)
			sensitivity.value = 1;
		if (sensitivity.value > 11)
			sensitivity.value = 11;
		Cvar_SetValue((char*)"sensitivity", sensitivity.value);
		break;
	case 6: // music volume
		bgmvolume.value += dir * 1.0;
		if (bgmvolume.value < 0)
			bgmvolume.value = 0;
		if (bgmvolume.value > 1)
			bgmvolume.value = 1;
		Cvar_SetValue((char*)"bgmvolume", bgmvolume.value);
		break;
	case 7: // sfx volume
		volume.value += dir * 0.1;
		if (volume.value < 0)
			volume.value = 0;
		if (volume.value > 1)
			volume.value = 1;
		Cvar_SetValue((char*)"volume", volume.value);
		break;

	case 8: // allways run
		if (cl_forwardspeed.value > 200)
		{
			Cvar_SetValue((char*)"cl_forwardspeed", 200);
			Cvar_SetValue((char*)"cl_backspeed", 200);
		}
		else
		{
			Cvar_SetValue((char*)"cl_forwardspeed", 400);
			Cvar_SetValue((char*)"cl_backspeed", 400);
		}
		break;

	case 9: // invert mouse
		Cvar_SetValue((char*)"m_pitch", -m_pitch.value);
		break;

	case 10: // lookspring
		Cvar_SetValue((char*)"lookspring", !lookspring.value);
		break;

	case 11: // lookstrafe
		Cvar_SetValue((char*)"lookstrafe", !lookstrafe.value);
		break;
		
	case 13: // _windowed_mouse
		Cvar_SetValue((char*)"_windowed_mouse", !_windowed_mouse.value);
		break;
	}
}


void M_DrawSlider(int x, int y, float range)
{
	int i;

	if (range < 0)
		range = 0;
	if (range > 1)
		range = 1;
	M_DrawCharacter(x - 8, y, 128);
	for (i = 0; i < SLIDER_RANGE; i++)
		M_DrawCharacter(x + i * 8, y, 129);
	M_DrawCharacter(x + i * 8, y, 130);
	M_DrawCharacter(x + (SLIDER_RANGE - 1) * 8 * range, y, 131);
}

void M_DrawCheckbox(int x, int y, int on)
{
	if (on)
		M_Print(x, y, (char*)"on");
	else
		M_Print(x, y, (char*)"off");
}

void M_Options_Draw(void)
{
	M_DrawTransPic(16, 4, Draw_CachePic((char*)"gfx/qplaque.lmp"));
	qpic_t* p = Draw_CachePic((char*)"gfx/p_option.lmp");
	M_DrawPic((320 - p->width) / 2, 4, p);

	M_Print(16, 32, (char*)"    Customize controls");
	M_Print(16, 40, (char*)"         Go to console");
	M_Print(16, 48, (char*)"     Reset to defaults");

	M_Print(16, 56, (char*)"           Screen size");
	float r = (scr_viewsize.value - 30) / (120 - 30);
	M_DrawSlider(220, 56, r);

	M_Print(16, 64, (char*)"            Brightness");
	r = (1.0 - v_gamma.value) / 0.5;
	M_DrawSlider(220, 64, r);

	M_Print(16, 72, (char*)"           Mouse Speed");
	r = (sensitivity.value - 1) / 10;
	M_DrawSlider(220, 72, r);

	M_Print(16, 80, (char*)"       CD Music Volume");
	r = bgmvolume.value;
	M_DrawSlider(220, 80, r);

	M_Print(16, 88, (char*)"          Sound Volume");
	r = volume.value;
	M_DrawSlider(220, 88, r);

	M_Print(16, 96, (char*)"            Always Run");
	M_DrawCheckbox(220, 96, cl_forwardspeed.value > 200);

	M_Print(16, 104, (char*)"          Invert Mouse");
	M_DrawCheckbox(220, 104, m_pitch.value < 0);

	M_Print(16, 112, (char*)"            Lookspring");
	M_DrawCheckbox(220, 112, lookspring.value);

	M_Print(16, 120, (char*)"            Lookstrafe");
	M_DrawCheckbox(220, 120, lookstrafe.value);

	if (vid_menudrawfn)
		M_Print(16, 128, (char*)"         Video Options");
	
	if (modestate == MS_WINDOWED)
	{
		M_Print(16, 136, (char*)"             Use Mouse");
		M_DrawCheckbox(220, 136, _windowed_mouse.value);
	}

	// cursor
	M_DrawCharacter(200, 32 + options_cursor * 8, 12 + ((int)(realtime * 4) & 1));
}


void M_Options_Key(int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Main_f();
		break;

	case K_ENTER:
		m_entersound = true;
		switch (options_cursor)
		{
		case 0:
			M_Menu_Keys_f();
			break;
		case 1:
			m_state = m_none;
			Con_ToggleConsole_f();
			break;
		case 2:
			Cbuf_AddText((char*)"exec default.cfg\n");
			break;
		case 12:
			M_Menu_Video_f();
			break;
		default:
			M_AdjustSliders(1);
			break;
		}
		return;

	case K_UPARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		options_cursor--;
		if (options_cursor < 0)
			options_cursor = OPTIONS_ITEMS - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		options_cursor++;
		if (options_cursor >= OPTIONS_ITEMS)
			options_cursor = 0;
		break;

	case K_LEFTARROW:
		M_AdjustSliders(-1);
		break;

	case K_RIGHTARROW:
		M_AdjustSliders(1);
		break;
	}

	if (options_cursor == 12 && vid_menudrawfn == NULL)
	{
		if (k == K_UPARROW)
			options_cursor = 11;
		else
			options_cursor = 0;
	}
	
	if ((options_cursor == 13) && (modestate != MS_WINDOWED))
	{
		if (k == K_UPARROW)
			options_cursor = 12;
		else
			options_cursor = 0;
	}
}

//=============================================================================
/* KEYS MENU */

char* bindnames[][2] =
{
	{(char*)"+attack", (char*)"attack"},
	{(char*)"impulse 10", (char*)"change weapon"},
	{(char*)"+jump", (char*)"jump / swim up"},
	{(char*)"+forward", (char*)"walk forward"},
	{(char*)"+back", (char*)"backpedal"},
	{(char*)"+left", (char*)"turn left"},
	{(char*)"+right", (char*)"turn right"},
	{(char*)"+speed", (char*)"run"},
	{(char*)"+moveleft", (char*)"step left"},
	{(char*)"+moveright", (char*)"step right"},
	{(char*)"+strafe", (char*)"sidestep"},
	{(char*)"+lookup", (char*)"look up"},
	{(char*)"+lookdown", (char*)"look down"},
	{(char*)"centerview", (char*)"center view"},
	{(char*)"+mlook", (char*)"mouse look"},
	{(char*)"+klook", (char*)"keyboard look"},
	{(char*)"+moveup", (char*)"swim up"},
	{(char*)"+movedown", (char*)"swim down"}
};

#define NUMCOMMANDS (sizeof(bindnames)/sizeof(bindnames[0]))

int keys_cursor;
int bind_grab;

void M_Menu_Keys_f(void)
{
	key_dest = key_menu;
	m_state = m_keys;
	m_entersound = true;
}


void M_FindKeysForCommand(char* command, int* twokeys)
{
	twokeys[0] = twokeys[1] = -1;
	int l = strlen(command);
	int count = 0;

	for (int j = 0; j < 256; j++)
	{
		char* b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp(b, command, l))
		{
			twokeys[count] = j;
			count++;
			if (count == 2)
				break;
		}
	}
}

void M_UnbindCommand(char* command)
{
	int l = strlen(command);

	for (int j = 0; j < 256; j++)
	{
		char* b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp(b, command, l))
			Key_SetBinding(j, (char*)"");
	}
}


void M_Keys_Draw(void)
{
	int keys[2];

	qpic_t* p = Draw_CachePic((char*)"gfx/ttl_cstm.lmp");
	M_DrawPic((320 - p->width) / 2, 4, p);

	if (bind_grab)
		M_Print(12, 32, (char*)"Press a key or button for this_ action");
	else
		M_Print(18, 32, (char*)"Enter to change, backspace to clear");

	// search for known bindings
	for (int i = 0; i < NUMCOMMANDS; i++)
	{
		int y = 48 + 8 * i;

		M_Print(16, y, bindnames[i][1]);

		int l = strlen(bindnames[i][0]);

		M_FindKeysForCommand(bindnames[i][0], keys);

		if (keys[0] == -1)
		{
			M_Print(140, y, (char*)"???");
		}
		else
		{
			char* name = Key_KeynumToString(keys[0]);
			M_Print(140, y, name);
			int x = strlen(name) * 8;
			if (keys[1] != -1)
			{
				M_Print(140 + x + 8, y, (char*)"or");
				M_Print(140 + x + 32, y, Key_KeynumToString(keys[1]));
			}
		}
	}

	if (bind_grab)
		M_DrawCharacter(130, 48 + keys_cursor * 8, '=');
	else
		M_DrawCharacter(130, 48 + keys_cursor * 8, 12 + ((int)(realtime * 4) & 1));
}


void M_Keys_Key(int k)
{
	char cmd[80];
	int keys[2];

	if (bind_grab)
	{
		// defining a key
		S_LocalSound((char*)"misc/menu1.wav");
		if (k == K_ESCAPE)
		{
			bind_grab = false;
		}
		else if (k != '`')
		{
			sprintf(cmd, (char*)"bind \"%s\" \"%s\"\n", Key_KeynumToString(k), bindnames[keys_cursor][0]);
			Cbuf_InsertText(cmd);
		}

		bind_grab = false;
		return;
	}

	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Options_f();
		break;

	case K_LEFTARROW:
	case K_UPARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		keys_cursor--;
		if (keys_cursor < 0)
			keys_cursor = NUMCOMMANDS - 1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		keys_cursor++;
		if (keys_cursor >= NUMCOMMANDS)
			keys_cursor = 0;
		break;

	case K_ENTER: // go into bind mode
		M_FindKeysForCommand(bindnames[keys_cursor][0], keys);
		S_LocalSound((char*)"misc/menu2.wav");
		if (keys[1] != -1)
			M_UnbindCommand(bindnames[keys_cursor][0]);
		bind_grab = true;
		break;

	case K_BACKSPACE: // delete bindings
	case K_DEL: // delete bindings
		S_LocalSound((char*)"misc/menu2.wav");
		M_UnbindCommand(bindnames[keys_cursor][0]);
		break;
	}
}

//=============================================================================
/* VIDEO MENU */

void M_Menu_Video_f(void)
{
	key_dest = key_menu;
	m_state = m_video;
	m_entersound = true;
}


void M_Video_Draw(void)
{
	(*vid_menudrawfn)();
}


void M_Video_Key(int key)
{
	(*vid_menukeyfn)(key);
}

//=============================================================================
/* HELP MENU */

int help_page;
#define NUM_HELP_PAGES 6


void M_Menu_Help_f(void)
{
	key_dest = key_menu;
	m_state = m_help;
	m_entersound = true;
	help_page = 0;
}


void M_Help_Draw(void)
{
	M_DrawPic(0, 0, Draw_CachePic(va((char*)"gfx/help%i.lmp", help_page)));
}


void M_Help_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f();
		break;

	case K_UPARROW:
	case K_RIGHTARROW:
		m_entersound = true;
		if (++help_page >= NUM_HELP_PAGES)
			help_page = 0;
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
		m_entersound = true;
		if (--help_page < 0)
			help_page = NUM_HELP_PAGES - 1;
		break;
	}
}

//=============================================================================
/* QUIT MENU */

int msgNumber;
int m_quit_prevstate;
qboolean wasInMenus;


void M_Menu_Quit_f(void)
{
	if (m_state == m_quit)
		return;
	wasInMenus = (key_dest == key_menu);
	key_dest = key_menu;
	m_quit_prevstate = m_state;
	m_state = m_quit;
	m_entersound = true;
	msgNumber = rand() & 7;
}


void M_Quit_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case 'n':
	case 'N':
		if (wasInMenus)
		{
			m_state = static_cast<MState>(m_quit_prevstate);
			m_entersound = true;
		}
		else
		{
			key_dest = key_game;
			m_state = m_none;
		}
		break;

	case 'Y':
	case 'y':
		key_dest = key_console;
		Host_Quit_f();
		break;

	default:
		break;
	}
}


void M_Quit_Draw(void)
{
	if (wasInMenus)
	{
		m_state = static_cast<MState>(m_quit_prevstate);
		m_recursiveDraw = true;
		M_Draw();
		m_state = m_quit;
	}
	
	M_DrawTextBox(0, 0, 38, 23);
	M_PrintWhite(16, 12, (char*)"  Quake version 1.09 by id Software\n\n");
	M_PrintWhite(16, 28, (char*)"Programming        Art \n");
	M_Print(16, 36, (char*)" John Carmack       Adrian Carmack\n");
	M_Print(16, 44, (char*)" Michael Abrash     Kevin Cloud\n");
	M_Print(16, 52, (char*)" John Cash          Paul Steed\n");
	M_Print(16, 60, (char*)" Dave 'Zoid' Kirsch\n");
	M_PrintWhite(16, 68, (char*)"Design             Biz\n");
	M_Print(16, 76, (char*)" John Romero        Jay Wilbur\n");
	M_Print(16, 84, (char*)" Sandy Petersen     Mike Wilson\n");
	M_Print(16, 92, (char*)" American McGee     Donna Jackson\n");
	M_Print(16, 100, (char*)" Tim Willits        Todd Hollenshead\n");
	M_PrintWhite(16, 108, (char*)"Support            Projects\n");
	M_Print(16, 116, (char*)" Barrett Alexander  Shawn Green\n");
	M_PrintWhite(16, 124, (char*)"Sound Effects\n");
	M_Print(16, 132, (char*)" Trent Reznor and Nine Inch Nails\n\n");
	M_PrintWhite(16, 140, (char*)"Quake is a trademark of Id Software,\n");
	M_PrintWhite(16, 148, (char*)"inc., (c)1996 Id Software, inc. All\n");
	M_PrintWhite(16, 156, (char*)"rights reserved. NIN logo is a\n");
	M_PrintWhite(16, 164, (char*)"registered trademark licensed to\n");
	M_PrintWhite(16, 172, (char*)"Nothing Interactive, Inc. All rights\n");
	M_PrintWhite(16, 180, (char*)"reserved. Press y to exit\n");
}

//=============================================================================

/* SERIAL CONFIG MENU */

int serialConfig_cursor;
int serialConfig_cursor_table[] = {48, 64, 80, 96, 112, 132};
#define NUM_SERIALCONFIG_CMDS 6

static int ISA_uarts[] = {0x3f8, 0x2f8, 0x3e8, 0x2e8};
static int ISA_IRQs[] = {4, 3, 4, 3};
int serialConfig_baudrate[] = {9600, 14400, 19200, 28800, 38400, 57600};

int serialConfig_comport;
int serialConfig_irq;
int serialConfig_baud;
char serialConfig_phone[16];

void M_Menu_SerialConfig_f(void)
{
	int n;
	int port;
	int baudrate;
	qboolean useModem;

	key_dest = key_menu;
	m_state = m_serialconfig;
	m_entersound = true;
	if (JoiningGame && SerialConfig)
		serialConfig_cursor = 4;
	else
		serialConfig_cursor = 5;

	(*GetComPortConfig)(0, &port, &serialConfig_irq, &baudrate, &useModem);

	// map uart's port to COMx
	for (n = 0; n < 4; n++)
		if (ISA_uarts[n] == port)
			break;
	if (n == 4)
	{
		n = 0;
		serialConfig_irq = 4;
	}
	serialConfig_comport = n + 1;

	// map baudrate to index
	for (n = 0; n < 6; n++)
		if (serialConfig_baudrate[n] == baudrate)
			break;
	if (n == 6)
		n = 5;
	serialConfig_baud = n;

	m_return_onerror = false;
	m_return_reason[0] = 0;
}


void M_SerialConfig_Draw(void)
{
	char* startJoin;
	char* directModem;

	M_DrawTransPic(16, 4, Draw_CachePic((char*)"gfx/qplaque.lmp"));
	qpic_t* p = Draw_CachePic((char*)"gfx/p_multi.lmp");
	int basex = (320 - p->width) / 2;
	M_DrawPic(basex, 4, p);

	if (StartingGame)
		startJoin = (char*)"New Game";
	else
		startJoin = (char*)"Join Game";
	if (SerialConfig)
		directModem = (char*)"Modem";
	else
		directModem = (char*)"Direct Connect";
	M_Print(basex, 32, va((char*)"%s - %s", startJoin, directModem));
	basex += 8;

	M_Print(basex, serialConfig_cursor_table[0], (char*)"Port");
	M_DrawTextBox(160, 40, 4, 1);
	M_Print(168, serialConfig_cursor_table[0], va((char*)"COM%u", serialConfig_comport));

	M_Print(basex, serialConfig_cursor_table[1], (char*)"IRQ");
	M_DrawTextBox(160, serialConfig_cursor_table[1] - 8, 1, 1);
	M_Print(168, serialConfig_cursor_table[1], va((char*)"%u", serialConfig_irq));

	M_Print(basex, serialConfig_cursor_table[2], (char*)"Baud");
	M_DrawTextBox(160, serialConfig_cursor_table[2] - 8, 5, 1);
	M_Print(168, serialConfig_cursor_table[2], va((char*)"%u", serialConfig_baudrate[serialConfig_baud]));

	if (SerialConfig)
	{
		M_Print(basex, serialConfig_cursor_table[3], (char*)"Modem Setup...");
		if (JoiningGame)
		{
			M_Print(basex, serialConfig_cursor_table[4], (char*)"Phone number");
			M_DrawTextBox(160, serialConfig_cursor_table[4] - 8, 16, 1);
			M_Print(168, serialConfig_cursor_table[4], serialConfig_phone);
		}
	}

	if (JoiningGame)
	{
		M_DrawTextBox(basex, serialConfig_cursor_table[5] - 8, 7, 1);
		M_Print(basex + 8, serialConfig_cursor_table[5], (char*)"Connect");
	}
	else
	{
		M_DrawTextBox(basex, serialConfig_cursor_table[5] - 8, 2, 1);
		M_Print(basex + 8, serialConfig_cursor_table[5], (char*)"OK");
	}

	M_DrawCharacter(basex - 8, serialConfig_cursor_table[serialConfig_cursor], 12 + ((int)(realtime * 4) & 1));

	if (serialConfig_cursor == 4)
		M_DrawCharacter(168 + 8 * strlen(serialConfig_phone), serialConfig_cursor_table[serialConfig_cursor],
		                10 + ((int)(realtime * 4) & 1));

	if (*m_return_reason)
		M_PrintWhite(basex, 148, m_return_reason);
}


void M_SerialConfig_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Net_f();
		break;

	case K_UPARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		serialConfig_cursor--;
		if (serialConfig_cursor < 0)
			serialConfig_cursor = NUM_SERIALCONFIG_CMDS - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		serialConfig_cursor++;
		if (serialConfig_cursor >= NUM_SERIALCONFIG_CMDS)
			serialConfig_cursor = 0;
		break;

	case K_LEFTARROW:
		if (serialConfig_cursor > 2)
			break;
		S_LocalSound((char*)"misc/menu3.wav");

		if (serialConfig_cursor == 0)
		{
			serialConfig_comport--;
			if (serialConfig_comport == 0)
				serialConfig_comport = 4;
			serialConfig_irq = ISA_IRQs[serialConfig_comport - 1];
		}

		if (serialConfig_cursor == 1)
		{
			serialConfig_irq--;
			if (serialConfig_irq == 6)
				serialConfig_irq = 5;
			if (serialConfig_irq == 1)
				serialConfig_irq = 7;
		}

		if (serialConfig_cursor == 2)
		{
			serialConfig_baud--;
			if (serialConfig_baud < 0)
				serialConfig_baud = 5;
		}

		break;

	case K_RIGHTARROW:
		if (serialConfig_cursor > 2)
			break;
	forward:
		S_LocalSound((char*)"misc/menu3.wav");

		if (serialConfig_cursor == 0)
		{
			serialConfig_comport++;
			if (serialConfig_comport > 4)
				serialConfig_comport = 1;
			serialConfig_irq = ISA_IRQs[serialConfig_comport - 1];
		}

		if (serialConfig_cursor == 1)
		{
			serialConfig_irq++;
			if (serialConfig_irq == 6)
				serialConfig_irq = 7;
			if (serialConfig_irq == 8)
				serialConfig_irq = 2;
		}

		if (serialConfig_cursor == 2)
		{
			serialConfig_baud++;
			if (serialConfig_baud > 5)
				serialConfig_baud = 0;
		}

		break;

	case K_ENTER:
		if (serialConfig_cursor < 3)
			goto forward;

		m_entersound = true;

		if (serialConfig_cursor == 3)
		{
			(*SetComPortConfig)(0, ISA_uarts[serialConfig_comport - 1], serialConfig_irq,
			                    serialConfig_baudrate[serialConfig_baud], SerialConfig);

			M_Menu_ModemConfig_f();
			break;
		}

		if (serialConfig_cursor == 4)
		{
			serialConfig_cursor = 5;
			break;
		}

	// serialConfig_cursor == 5 (OK/CONNECT)
		(*SetComPortConfig)(0, ISA_uarts[serialConfig_comport - 1], serialConfig_irq,
		                    serialConfig_baudrate[serialConfig_baud], SerialConfig);

		M_ConfigureNetSubsystem();

		if (StartingGame)
		{
			M_Menu_GameOptions_f();
			break;
		}

		m_return_state = m_state;
		m_return_onerror = true;
		key_dest = key_game;
		m_state = m_none;

		if (SerialConfig)
			Cbuf_AddText(va((char*)"connect \"%s\"\n", serialConfig_phone));
		else
			Cbuf_AddText((char*)"connect\n");
		break;

	case K_BACKSPACE:
		if (serialConfig_cursor == 4)
		{
			if (strlen(serialConfig_phone))
				serialConfig_phone[strlen(serialConfig_phone) - 1] = 0;
		}
		break;

	default:
		if (key < 32 || key > 127)
			break;
		if (serialConfig_cursor == 4)
		{
			int l = strlen(serialConfig_phone);
			if (l < 15)
			{
				serialConfig_phone[l + 1] = 0;
				serialConfig_phone[l] = key;
			}
		}
	}

	if (DirectConfig && (serialConfig_cursor == 3 || serialConfig_cursor == 4))
		if (key == K_UPARROW)
			serialConfig_cursor = 2;
		else
			serialConfig_cursor = 5;

	if (SerialConfig && StartingGame && serialConfig_cursor == 4)
		if (key == K_UPARROW)
			serialConfig_cursor = 3;
		else
			serialConfig_cursor = 5;
}

//=============================================================================
/* MODEM CONFIG MENU */

int modemConfig_cursor;
int modemConfig_cursor_table[] = {40, 56, 88, 120, 156};
#define NUM_MODEMCONFIG_CMDS 5

char modemConfig_dialing;
char modemConfig_clear[16];
char modemConfig_init[32];
char modemConfig_hangup[16];

void M_Menu_ModemConfig_f(void)
{
	key_dest = key_menu;
	m_state = m_modemconfig;
	m_entersound = true;
	(*GetModemConfig)(0, &modemConfig_dialing, modemConfig_clear, modemConfig_init, modemConfig_hangup);
}


void M_ModemConfig_Draw(void)
{
	M_DrawTransPic(16, 4, Draw_CachePic((char*)"gfx/qplaque.lmp"));
	qpic_t* p = Draw_CachePic((char*)"gfx/p_multi.lmp");
	int basex = (320 - p->width) / 2;
	M_DrawPic(basex, 4, p);
	basex += 8;

	if (modemConfig_dialing == 'P')
		M_Print(basex, modemConfig_cursor_table[0], (char*)"Pulse Dialing");
	else
		M_Print(basex, modemConfig_cursor_table[0], (char*)"Touch Tone Dialing");

	M_Print(basex, modemConfig_cursor_table[1], (char*)"Clear");
	M_DrawTextBox(basex, modemConfig_cursor_table[1] + 4, 16, 1);
	M_Print(basex + 8, modemConfig_cursor_table[1] + 12, modemConfig_clear);
	if (modemConfig_cursor == 1)
		M_DrawCharacter(basex + 8 + 8 * strlen(modemConfig_clear), modemConfig_cursor_table[1] + 12,
		                10 + ((int)(realtime * 4) & 1));

	M_Print(basex, modemConfig_cursor_table[2], (char*)"Init");
	M_DrawTextBox(basex, modemConfig_cursor_table[2] + 4, 30, 1);
	M_Print(basex + 8, modemConfig_cursor_table[2] + 12, modemConfig_init);
	if (modemConfig_cursor == 2)
		M_DrawCharacter(basex + 8 + 8 * strlen(modemConfig_init), modemConfig_cursor_table[2] + 12,
		                10 + ((int)(realtime * 4) & 1));

	M_Print(basex, modemConfig_cursor_table[3], (char*)"Hangup");
	M_DrawTextBox(basex, modemConfig_cursor_table[3] + 4, 16, 1);
	M_Print(basex + 8, modemConfig_cursor_table[3] + 12, modemConfig_hangup);
	if (modemConfig_cursor == 3)
		M_DrawCharacter(basex + 8 + 8 * strlen(modemConfig_hangup), modemConfig_cursor_table[3] + 12,
		                10 + ((int)(realtime * 4) & 1));

	M_DrawTextBox(basex, modemConfig_cursor_table[4] - 8, 2, 1);
	M_Print(basex + 8, modemConfig_cursor_table[4], (char*)"OK");

	M_DrawCharacter(basex - 8, modemConfig_cursor_table[modemConfig_cursor], 12 + ((int)(realtime * 4) & 1));
}


void M_ModemConfig_Key(int key)
{
	int l;

	switch (key)
	{
	case K_ESCAPE:
		M_Menu_SerialConfig_f();
		break;

	case K_UPARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		modemConfig_cursor--;
		if (modemConfig_cursor < 0)
			modemConfig_cursor = NUM_MODEMCONFIG_CMDS - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		modemConfig_cursor++;
		if (modemConfig_cursor >= NUM_MODEMCONFIG_CMDS)
			modemConfig_cursor = 0;
		break;

	case K_LEFTARROW:
	case K_RIGHTARROW:
		if (modemConfig_cursor == 0)
		{
			if (modemConfig_dialing == 'P')
				modemConfig_dialing = 'T';
			else
				modemConfig_dialing = 'P';
			S_LocalSound((char*)"misc/menu1.wav");
		}
		break;

	case K_ENTER:
		if (modemConfig_cursor == 0)
		{
			if (modemConfig_dialing == 'P')
				modemConfig_dialing = 'T';
			else
				modemConfig_dialing = 'P';
			m_entersound = true;
		}

		if (modemConfig_cursor == 4)
		{
			(*SetModemConfig)(0, va((char*)"%c", modemConfig_dialing), modemConfig_clear, modemConfig_init,
			                  modemConfig_hangup);
			m_entersound = true;
			M_Menu_SerialConfig_f();
		}
		break;

	case K_BACKSPACE:
		if (modemConfig_cursor == 1)
		{
			if (strlen(modemConfig_clear))
				modemConfig_clear[strlen(modemConfig_clear) - 1] = 0;
		}

		if (modemConfig_cursor == 2)
		{
			if (strlen(modemConfig_init))
				modemConfig_init[strlen(modemConfig_init) - 1] = 0;
		}

		if (modemConfig_cursor == 3)
		{
			if (strlen(modemConfig_hangup))
				modemConfig_hangup[strlen(modemConfig_hangup) - 1] = 0;
		}
		break;

	default:
		if (key < 32 || key > 127)
			break;

		if (modemConfig_cursor == 1)
		{
			l = strlen(modemConfig_clear);
			if (l < 15)
			{
				modemConfig_clear[l + 1] = 0;
				modemConfig_clear[l] = key;
			}
		}

		if (modemConfig_cursor == 2)
		{
			l = strlen(modemConfig_init);
			if (l < 29)
			{
				modemConfig_init[l + 1] = 0;
				modemConfig_init[l] = key;
			}
		}

		if (modemConfig_cursor == 3)
		{
			l = strlen(modemConfig_hangup);
			if (l < 15)
			{
				modemConfig_hangup[l + 1] = 0;
				modemConfig_hangup[l] = key;
			}
		}
	}
}

//=============================================================================
/* LAN CONFIG MENU */

int lanConfig_cursor = -1;
int lanConfig_cursor_table[] = {72, 92, 124};
#define NUM_LANCONFIG_CMDS 3

int lanConfig_port;
char lanConfig_portname[6];
char lanConfig_joinname[22];

void M_Menu_LanConfig_f(void)
{
	key_dest = key_menu;
	m_state = m_lanconfig;
	m_entersound = true;
	if (lanConfig_cursor == -1)
	{
		if (JoiningGame && TCPIPConfig)
			lanConfig_cursor = 2;
		else
			lanConfig_cursor = 1;
	}
	if (StartingGame && lanConfig_cursor == 2)
		lanConfig_cursor = 1;
	lanConfig_port = DEFAULTnet_hostport;
	sprintf(lanConfig_portname, (char*)"%u", lanConfig_port);

	m_return_onerror = false;
	m_return_reason[0] = 0;
}


void M_LanConfig_Draw(void)
{
	char* startJoin;
	char* protocol;

	M_DrawTransPic(16, 4, Draw_CachePic((char*)"gfx/qplaque.lmp"));
	qpic_t* p = Draw_CachePic((char*)"gfx/p_multi.lmp");
	int basex = (320 - p->width) / 2;
	M_DrawPic(basex, 4, p);

	if (StartingGame)
		startJoin = (char*)"New Game";
	else
		startJoin = (char*)"Join Game";
	if (IPXConfig)
		protocol = (char*)"IPX";
	else
		protocol = (char*)"TCP/IP";
	M_Print(basex, 32, va((char*)"%s - %s", startJoin, protocol));
	basex += 8;

	M_Print(basex, 52, (char*)"Address:");
	if (IPXConfig)
		M_Print(basex + 9 * 8, 52, my_ipx_address);
	else
		M_Print(basex + 9 * 8, 52, my_tcpip_address);

	M_Print(basex, lanConfig_cursor_table[0], (char*)"Port");
	M_DrawTextBox(basex + 8 * 8, lanConfig_cursor_table[0] - 8, 6, 1);
	M_Print(basex + 9 * 8, lanConfig_cursor_table[0], lanConfig_portname);

	if (JoiningGame)
	{
		M_Print(basex, lanConfig_cursor_table[1], (char*)"Search for local games...");
		M_Print(basex, 108, (char*)"Join game at:");
		M_DrawTextBox(basex + 8, lanConfig_cursor_table[2] - 8, 22, 1);
		M_Print(basex + 16, lanConfig_cursor_table[2], lanConfig_joinname);
	}
	else
	{
		M_DrawTextBox(basex, lanConfig_cursor_table[1] - 8, 2, 1);
		M_Print(basex + 8, lanConfig_cursor_table[1], (char*)"OK");
	}

	M_DrawCharacter(basex - 8, lanConfig_cursor_table[lanConfig_cursor], 12 + ((int)(realtime * 4) & 1));

	if (lanConfig_cursor == 0)
		M_DrawCharacter(basex + 9 * 8 + 8 * strlen(lanConfig_portname), lanConfig_cursor_table[0],
		                10 + ((int)(realtime * 4) & 1));

	if (lanConfig_cursor == 2)
		M_DrawCharacter(basex + 16 + 8 * strlen(lanConfig_joinname), lanConfig_cursor_table[2],
		                10 + ((int)(realtime * 4) & 1));

	if (*m_return_reason)
		M_PrintWhite(basex, 148, m_return_reason);
}


void M_LanConfig_Key(int key)
{
	int l;

	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Net_f();
		break;

	case K_UPARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		lanConfig_cursor--;
		if (lanConfig_cursor < 0)
			lanConfig_cursor = NUM_LANCONFIG_CMDS - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		lanConfig_cursor++;
		if (lanConfig_cursor >= NUM_LANCONFIG_CMDS)
			lanConfig_cursor = 0;
		break;

	case K_ENTER:
		if (lanConfig_cursor == 0)
			break;

		m_entersound = true;

		M_ConfigureNetSubsystem();

		if (lanConfig_cursor == 1)
		{
			if (StartingGame)
			{
				M_Menu_GameOptions_f();
				break;
			}
			M_Menu_Search_f();
			break;
		}

		if (lanConfig_cursor == 2)
		{
			m_return_state = m_state;
			m_return_onerror = true;
			key_dest = key_game;
			m_state = m_none;
			Cbuf_AddText(va((char*)"connect \"%s\"\n", lanConfig_joinname));
			break;
		}

		break;

	case K_BACKSPACE:
		if (lanConfig_cursor == 0)
		{
			if (strlen(lanConfig_portname))
				lanConfig_portname[strlen(lanConfig_portname) - 1] = 0;
		}

		if (lanConfig_cursor == 2)
		{
			if (strlen(lanConfig_joinname))
				lanConfig_joinname[strlen(lanConfig_joinname) - 1] = 0;
		}
		break;

	default:
		if (key < 32 || key > 127)
			break;

		if (lanConfig_cursor == 2)
		{
			l = strlen(lanConfig_joinname);
			if (l < 21)
			{
				lanConfig_joinname[l + 1] = 0;
				lanConfig_joinname[l] = key;
			}
		}

		if (key < '0' || key > '9')
			break;
		if (lanConfig_cursor == 0)
		{
			l = strlen(lanConfig_portname);
			if (l < 5)
			{
				lanConfig_portname[l + 1] = 0;
				lanConfig_portname[l] = key;
			}
		}
	}

	if (StartingGame && lanConfig_cursor == 2)
		if (key == K_UPARROW)
			lanConfig_cursor = 1;
		else
			lanConfig_cursor = 0;

	l = Q_atoi(lanConfig_portname);
	if (l > 65535)
		l = lanConfig_port;
	else
		lanConfig_port = l;
	sprintf(lanConfig_portname, (char*)"%u", lanConfig_port);
}

//=============================================================================
/* GAME OPTIONS MENU */

typedef struct
{
	char* name;
	char* description;
} level_t;

level_t levels[] =
{
	{(char*)"start", (char*)"Entrance"}, // 0

	{(char*)"e1m1", (char*)"Slipgate Complex"}, // 1
	{(char*)"e1m2", (char*)"Castle of the Damned"},
	{(char*)"e1m3", (char*)"The Necropolis"},
	{(char*)"e1m4", (char*)"The Grisly Grotto"},
	{(char*)"e1m5", (char*)"Gloom Keep"},
	{(char*)"e1m6", (char*)"The Door To Chthon"},
	{(char*)"e1m7", (char*)"The House of Chthon"},
	{(char*)"e1m8", (char*)"Ziggurat Vertigo"},

	{(char*)"e2m1", (char*)"The Installation"}, // 9
	{(char*)"e2m2", (char*)"Ogre Citadel"},
	{(char*)"e2m3", (char*)"Crypt of Decay"},
	{(char*)"e2m4", (char*)"The Ebon Fortress"},
	{(char*)"e2m5", (char*)"The Wizard's Manse"},
	{(char*)"e2m6", (char*)"The Dismal Oubliette"},
	{(char*)"e2m7", (char*)"Underearth"},

	{(char*)"e3m1", (char*)"Termination Central"}, // 16
	{(char*)"e3m2", (char*)"The Vaults of Zin"},
	{(char*)"e3m3", (char*)"The Tomb of Terror"},
	{(char*)"e3m4", (char*)"Satan's Dark Delight"},
	{(char*)"e3m5", (char*)"Wind Tunnels"},
	{(char*)"e3m6", (char*)"Chambers of Torment"},
	{(char*)"e3m7", (char*)"The Haunted Halls"},

	{(char*)"e4m1", (char*)"The Sewage System"}, // 23
	{(char*)"e4m2", (char*)"The Tower of Despair"},
	{(char*)"e4m3", (char*)"The Elder God Shrine"},
	{(char*)"e4m4", (char*)"The Palace of Hate"},
	{(char*)"e4m5", (char*)"Hell's Atrium"},
	{(char*)"e4m6", (char*)"The Pain Maze"},
	{(char*)"e4m7", (char*)"Azure Agony"},
	{(char*)"e4m8", (char*)"The Nameless City"},

	{(char*)"end", (char*)"Shub-Niggurath's Pit"}, // 31

	{(char*)"dm1", (char*)"Place of Two Deaths"}, // 32
	{(char*)"dm2", (char*)"Claustrophobopolis"},
	{(char*)"dm3", (char*)"The Abandoned Base"},
	{(char*)"dm4", (char*)"The Bad Place"},
	{(char*)"dm5", (char*)"The Cistern"},
	{(char*)"dm6", (char*)"The Dark Zone"}
};

//MED 01/06/97 added hipnotic levels
level_t hipnoticlevels[] =
{
	{(char*)"start", (char*)"Command HQ"}, // 0

	{(char*)"hip1m1", (char*)"The Pumping Station"}, // 1
	{(char*)"hip1m2", (char*)"Storage Facility"},
	{(char*)"hip1m3", (char*)"The Lost Mine"},
	{(char*)"hip1m4", (char*)"Research Facility"},
	{(char*)"hip1m5", (char*)"Military Complex"},

	{(char*)"hip2m1", (char*)"Ancient Realms"}, // 6
	{(char*)"hip2m2", (char*)"The Black Cathedral"},
	{(char*)"hip2m3", (char*)"The Catacombs"},
	{(char*)"hip2m4", (char*)"The Crypt"},
	{(char*)"hip2m5", (char*)"Mortum's Keep"},
	{(char*)"hip2m6", (char*)"The Gremlin's Domain"},

	{(char*)"hip3m1", (char*)"Tur Torment"}, // 12
	{(char*)"hip3m2", (char*)"Pandemonium"},
	{(char*)"hip3m3", (char*)"Limbo"},
	{(char*)"hip3m4", (char*)"The Gauntlet"},

	{(char*)"hipend", (char*)"Armagon's Lair"}, // 16

	{(char*)"hipdm1", (char*)"The Edge of Oblivion"} // 17
};

//PGM 01/07/97 added rogue levels
//PGM 03/02/97 added dmatch level
level_t roguelevels[] =
{
	{(char*)"start", (char*)"Split Decision"},
	{(char*)"r1m1", (char*)"Deviant's Domain"},
	{(char*)"r1m2", (char*)"Dread Portal"},
	{(char*)"r1m3", (char*)"Judgement Call"},
	{(char*)"r1m4", (char*)"Cave of Death"},
	{(char*)"r1m5", (char*)"Towers of Wrath"},
	{(char*)"r1m6", (char*)"Temple of Pain"},
	{(char*)"r1m7", (char*)"Tomb of the Overlord"},
	{(char*)"r2m1", (char*)"Tempus Fugit"},
	{(char*)"r2m2", (char*)"Elemental Fury I"},
	{(char*)"r2m3", (char*)"Elemental Fury II"},
	{(char*)"r2m4", (char*)"Curse of Osiris"},
	{(char*)"r2m5", (char*)"Wizard's Keep"},
	{(char*)"r2m6", (char*)"Blood Sacrifice"},
	{(char*)"r2m7", (char*)"Last Bastion"},
	{(char*)"r2m8", (char*)"Source of Evil"},
	{(char*)"ctf1", (char*)"Division of Change"}
};

typedef struct
{
	char* description;
	int firstLevel;
	int levels;
} episode_t;

episode_t episodes[] =
{
	{(char*)"Welcome to Quake", 0, 1},
	{(char*)"Doomed Dimension", 1, 8},
	{(char*)"Realm of Black Magic", 9, 7},
	{(char*)"Netherworld", 16, 7},
	{(char*)"The Elder World", 23, 8},
	{(char*)"Final Level", 31, 1},
	{(char*)"Deathmatch Arena", 32, 6}
};

//MED 01/06/97  added hipnotic episodes
episode_t hipnoticepisodes[] =
{
	{(char*)"Scourge of Armagon", 0, 1},
	{(char*)"Fortress of the Dead", 1, 5},
	{(char*)"Dominion of Darkness", 6, 6},
	{(char*)"The Rift", 12, 4},
	{(char*)"Final Level", 16, 1},
	{(char*)"Deathmatch Arena", 17, 1}
};

//PGM 01/07/97 added rogue episodes
//PGM 03/02/97 added dmatch episode
episode_t rogueepisodes[] =
{
	{(char*)"Introduction", 0, 1},
	{(char*)"Hell's Fortress", 1, 7},
	{(char*)"Corridors of Time", 8, 8},
	{(char*)"Deathmatch Arena", 16, 1}
};

int startepisode;
int startlevel;
int maxplayers;
qboolean m_serverInfoMessage = false;
double m_serverInfoMessageTime;

void M_Menu_GameOptions_f(void)
{
	key_dest = key_menu;
	m_state = m_gameoptions;
	m_entersound = true;
	if (maxplayers == 0)
		maxplayers = svs.maxclients;
	if (maxplayers < 2)
		maxplayers = svs.maxclientslimit;
}


int gameoptions_cursor_table[] = {40, 56, 64, 72, 80, 88, 96, 112, 120};
#define NUM_GAMEOPTIONS 9
int gameoptions_cursor;

void M_GameOptions_Draw(void)
{
	M_DrawTransPic(16, 4, Draw_CachePic((char*)"gfx/qplaque.lmp"));
	qpic_t* p = Draw_CachePic((char*)"gfx/p_multi.lmp");
	M_DrawPic((320 - p->width) / 2, 4, p);

	M_DrawTextBox(152, 32, 10, 1);
	M_Print(160, 40, (char*)"begin game");

	M_Print(0, 56, (char*)"      Max players");
	M_Print(160, 56, va((char*)"%i", maxplayers));

	M_Print(0, 64, (char*)"        Game Type");
	if (coop.value)
		M_Print(160, 64, (char*)"Cooperative");
	else
		M_Print(160, 64, (char*)"Deathmatch");

	M_Print(0, 72, (char*)"        Teamplay");
	if (rogue)
	{
		char* msg;

		switch ((int)teamplay.value)
		{
		case 1: msg = (char*)"No Friendly Fire";
			break;
		case 2: msg = (char*)"Friendly Fire";
			break;
		case 3: msg = (char*)"Tag";
			break;
		case 4: msg = (char*)"Capture the Flag";
			break;
		case 5: msg = (char*)"One Flag CTF";
			break;
		case 6: msg = (char*)"Three Team CTF";
			break;
		default: msg = (char*)"Off";
			break;
		}
		M_Print(160, 72, msg);
	}
	else
	{
		char* msg;

		switch ((int)teamplay.value)
		{
		case 1: msg = (char*)"No Friendly Fire";
			break;
		case 2: msg = (char*)"Friendly Fire";
			break;
		default: msg = (char*)"Off";
			break;
		}
		M_Print(160, 72, msg);
	}

	M_Print(0, 80, (char*)"            Skill");
	if (skill.value == 0)
		M_Print(160, 80, (char*)"Easy difficulty");
	else if (skill.value == 1)
		M_Print(160, 80, (char*)"Normal difficulty");
	else if (skill.value == 2)
		M_Print(160, 80, (char*)"Hard difficulty");
	else
		M_Print(160, 80, (char*)"Nightmare difficulty");

	M_Print(0, 88, (char*)"       Frag Limit");
	if (fraglimit.value == 0)
		M_Print(160, 88, (char*)"none");
	else
		M_Print(160, 88, va((char*)"%i frags", (int)fraglimit.value));

	M_Print(0, 96, (char*)"       Time Limit");
	if (timelimit.value == 0)
		M_Print(160, 96, (char*)"none");
	else
		M_Print(160, 96, va((char*)"%i minutes", (int)timelimit.value));

	M_Print(0, 112, (char*)"         Episode");
	//MED 01/06/97 added hipnotic episodes
	if (hipnotic)
		M_Print(160, 112, hipnoticepisodes[startepisode].description);
		//PGM 01/07/97 added rogue episodes
	else if (rogue)
		M_Print(160, 112, rogueepisodes[startepisode].description);
	else
		M_Print(160, 112, episodes[startepisode].description);

	M_Print(0, 120, (char*)"           Level");
	//MED 01/06/97 added hipnotic episodes
	if (hipnotic)
	{
		M_Print(160, 120, hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].description);
		M_Print(160, 128, hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].name);
	}
	//PGM 01/07/97 added rogue episodes
	else if (rogue)
	{
		M_Print(160, 120, roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].description);
		M_Print(160, 128, roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].name);
	}
	else
	{
		M_Print(160, 120, levels[episodes[startepisode].firstLevel + startlevel].description);
		M_Print(160, 128, levels[episodes[startepisode].firstLevel + startlevel].name);
	}

	// line cursor
	M_DrawCharacter(144, gameoptions_cursor_table[gameoptions_cursor], 12 + ((int)(realtime * 4) & 1));

	if (m_serverInfoMessage)
	{
		if ((realtime - m_serverInfoMessageTime) < 5.0)
		{
			int x = (320 - 26 * 8) / 2;
			M_DrawTextBox(x, 138, 24, 4);
			x += 8;
			M_Print(x, 146, (char*)"  More than 4 players   ");
			M_Print(x, 154, (char*)" requires using command ");
			M_Print(x, 162, (char*)"line parameters; please ");
			M_Print(x, 170, (char*)"   see techinfo.txt.    ");
		}
		else
		{
			m_serverInfoMessage = false;
		}
	}
}


void M_NetStart_Change(int dir)
{
	int count;

	switch (gameoptions_cursor)
	{
	case 1:
		maxplayers += dir;
		if (maxplayers > svs.maxclientslimit)
		{
			maxplayers = svs.maxclientslimit;
			m_serverInfoMessage = true;
			m_serverInfoMessageTime = realtime;
		}
		if (maxplayers < 2)
			maxplayers = 2;
		break;

	case 2:
		Cvar_SetValue((char*)"coop", coop.value ? 0 : 1);
		break;

	case 3:
		if (rogue)
			count = 6;
		else
			count = 2;

		Cvar_SetValue((char*)"teamplay", teamplay.value + dir);
		if (teamplay.value > count)
			Cvar_SetValue((char*)"teamplay", 0);
		else if (teamplay.value < 0)
			Cvar_SetValue((char*)"teamplay", count);
		break;

	case 4:
		Cvar_SetValue((char*)"skill", skill.value + dir);
		if (skill.value > 3)
			Cvar_SetValue((char*)"skill", 0);
		if (skill.value < 0)
			Cvar_SetValue((char*)"skill", 3);
		break;

	case 5:
		Cvar_SetValue((char*)"fraglimit", fraglimit.value + dir * 10);
		if (fraglimit.value > 100)
			Cvar_SetValue((char*)"fraglimit", 0);
		if (fraglimit.value < 0)
			Cvar_SetValue((char*)"fraglimit", 100);
		break;

	case 6:
		Cvar_SetValue((char*)"timelimit", timelimit.value + dir * 5);
		if (timelimit.value > 60)
			Cvar_SetValue((char*)"timelimit", 0);
		if (timelimit.value < 0)
			Cvar_SetValue((char*)"timelimit", 60);
		break;

	case 7:
		startepisode += dir;
	//MED 01/06/97 added hipnotic count
		if (hipnotic)
			count = 6;
			//PGM 01/07/97 added rogue count
			//PGM 03/02/97 added 1 for dmatch episode
		else if (rogue)
			count = 4;
		else if (registered.value)
			count = 7;
		else
			count = 2;

		if (startepisode < 0)
			startepisode = count - 1;

		if (startepisode >= count)
			startepisode = 0;

		startlevel = 0;
		break;

	case 8:
		startlevel += dir;
	//MED 01/06/97 added hipnotic episodes
		if (hipnotic)
			count = hipnoticepisodes[startepisode].levels;
			//PGM 01/06/97 added hipnotic episodes
		else if (rogue)
			count = rogueepisodes[startepisode].levels;
		else
			count = episodes[startepisode].levels;

		if (startlevel < 0)
			startlevel = count - 1;

		if (startlevel >= count)
			startlevel = 0;
		break;
	}
}

void M_GameOptions_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Net_f();
		break;

	case K_UPARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		gameoptions_cursor--;
		if (gameoptions_cursor < 0)
			gameoptions_cursor = NUM_GAMEOPTIONS - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		gameoptions_cursor++;
		if (gameoptions_cursor >= NUM_GAMEOPTIONS)
			gameoptions_cursor = 0;
		break;

	case K_LEFTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound((char*)"misc/menu3.wav");
		M_NetStart_Change(-1);
		break;

	case K_RIGHTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound((char*)"misc/menu3.wav");
		M_NetStart_Change(1);
		break;

	case K_ENTER:
		S_LocalSound((char*)"misc/menu2.wav");
		if (gameoptions_cursor == 0)
		{
			if (sv.active)
				Cbuf_AddText((char*)"disconnect\n");
			Cbuf_AddText((char*)"listen 0\n"); // so host_netport will be re-examined
			Cbuf_AddText(va((char*)"maxplayers %u\n", maxplayers));
			SCR_BeginLoadingPlaque();

			if (hipnotic)
				Cbuf_AddText(
					va((char*)"map %s\n", hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].name));
			else if (rogue)
				Cbuf_AddText(va((char*)"map %s\n", roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].name));
			else
				Cbuf_AddText(va((char*)"map %s\n", levels[episodes[startepisode].firstLevel + startlevel].name));

			return;
		}

		M_NetStart_Change(1);
		break;
	}
}

//=============================================================================
/* SEARCH MENU */

qboolean searchComplete = false;
double searchCompleteTime;

void M_Menu_Search_f(void)
{
	key_dest = key_menu;
	m_state = m_search;
	m_entersound = false;
	slistSilent = true;
	slistLocal = false;
	searchComplete = false;
	NET_Slist_f();
}


void M_Search_Draw(void)
{
	qpic_t* p = Draw_CachePic((char*)"gfx/p_multi.lmp");
	M_DrawPic((320 - p->width) / 2, 4, p);
	int x = (320 / 2) - ((12 * 8) / 2) + 4;
	M_DrawTextBox(x - 8, 32, 12, 1);
	M_Print(x, 40, (char*)"Searching...");

	if (slistInProgress)
	{
		NET_Poll();
		return;
	}

	if (!searchComplete)
	{
		searchComplete = true;
		searchCompleteTime = realtime;
	}

	if (hostCacheCount)
	{
		M_Menu_ServerList_f();
		return;
	}

	M_PrintWhite((320 / 2) - ((22 * 8) / 2), 64, (char*)"No Quake servers found");
	if ((realtime - searchCompleteTime) < 3.0)
		return;

	M_Menu_LanConfig_f();
}


void M_Search_Key(int key)
{
}

//=============================================================================
/* SLIST MENU */

int slist_cursor;
qboolean slist_sorted;

void M_Menu_ServerList_f(void)
{
	key_dest = key_menu;
	m_state = m_slist;
	m_entersound = true;
	slist_cursor = 0;
	m_return_onerror = false;
	m_return_reason[0] = 0;
	slist_sorted = false;
}


void M_ServerList_Draw(void)
{
	char string[64];

	if (!slist_sorted)
	{
		if (hostCacheCount > 1)
		{
			hostcache_t temp;
			for (int i = 0; i < hostCacheCount; i++)
				for (int j = i + 1; j < hostCacheCount; j++)
					if (strcmp(hostcache[j].name, hostcache[i].name) < 0)
					{
						Q_memcpy(&temp, &hostcache[j], sizeof(hostcache_t));
						Q_memcpy(&hostcache[j], &hostcache[i], sizeof(hostcache_t));
						Q_memcpy(&hostcache[i], &temp, sizeof(hostcache_t));
					}
		}
		slist_sorted = true;
	}

	qpic_t* p = Draw_CachePic((char*)"gfx/p_multi.lmp");
	M_DrawPic((320 - p->width) / 2, 4, p);
	for (int n = 0; n < hostCacheCount; n++)
	{
		if (hostcache[n].maxusers)
			sprintf(string, (char*)"%-15.15s %-15.15s %2u/%2u\n", hostcache[n].name, hostcache[n].map, hostcache[n].users,
			        hostcache[n].maxusers);
		else
			sprintf(string, (char*)"%-15.15s %-15.15s\n", hostcache[n].name, hostcache[n].map);
		M_Print(16, 32 + 8 * n, string);
	}
	M_DrawCharacter(0, 32 + slist_cursor * 8, 12 + ((int)(realtime * 4) & 1));

	if (*m_return_reason)
		M_PrintWhite(16, 148, m_return_reason);
}


void M_ServerList_Key(int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_LanConfig_f();
		break;

	case K_SPACE:
		M_Menu_Search_f();
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		slist_cursor--;
		if (slist_cursor < 0)
			slist_cursor = hostCacheCount - 1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound((char*)"misc/menu1.wav");
		slist_cursor++;
		if (slist_cursor >= hostCacheCount)
			slist_cursor = 0;
		break;

	case K_ENTER:
		S_LocalSound((char*)"misc/menu2.wav");
		m_return_state = m_state;
		m_return_onerror = true;
		slist_sorted = false;
		key_dest = key_game;
		m_state = m_none;
		Cbuf_AddText(va((char*)"connect \"%s\"\n", hostcache[slist_cursor].cname));
		break;

	default:
		break;
	}
}

//=============================================================================
/* Menu Subsystem */


void M_Init(void)
{
	Cmd_AddCommand((char*)"togglemenu", M_ToggleMenu_f);

	Cmd_AddCommand((char*)"menu_main", M_Menu_Main_f);
	Cmd_AddCommand((char*)"menu_singleplayer", M_Menu_SinglePlayer_f);
	Cmd_AddCommand((char*)"menu_load", M_Menu_Load_f);
	Cmd_AddCommand((char*)"menu_save", M_Menu_Save_f);
	Cmd_AddCommand((char*)"menu_multiplayer", M_Menu_MultiPlayer_f);
	Cmd_AddCommand((char*)"menu_setup", M_Menu_Setup_f);
	Cmd_AddCommand((char*)"menu_options", M_Menu_Options_f);
	Cmd_AddCommand((char*)"menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand((char*)"menu_video", M_Menu_Video_f);
	Cmd_AddCommand((char*)"help", M_Menu_Help_f);
	Cmd_AddCommand((char*)"menu_quit", M_Menu_Quit_f);
}


void M_Draw(void)
{
	if (m_state == m_none || key_dest != key_menu)
		return;

	if (!m_recursiveDraw)
	{
		scr_copyeverything = 1;

		if (scr_con_current)
		{
			Draw_ConsoleBackground(vid.height);
			VID_UnlockBuffer();
			S_ExtraUpdate();
			VID_LockBuffer();
		}
		else
			Draw_FadeScreen();

		scr_fullupdate = 0;
	}
	else
	{
		m_recursiveDraw = false;
	}

	switch (m_state)
	{
	case m_none:
		break;

	case m_main:
		M_Main_Draw();
		break;

	case m_singleplayer:
		M_SinglePlayer_Draw();
		break;

	case m_load:
		M_Load_Draw();
		break;

	case m_save:
		M_Save_Draw();
		break;

	case m_multiplayer:
		M_MultiPlayer_Draw();
		break;

	case m_setup:
		M_Setup_Draw();
		break;

	case m_net:
		M_Net_Draw();
		break;

	case m_options:
		M_Options_Draw();
		break;

	case m_keys:
		M_Keys_Draw();
		break;

	case m_video:
		M_Video_Draw();
		break;

	case m_help:
		M_Help_Draw();
		break;

	case m_quit:
		M_Quit_Draw();
		break;

	case m_serialconfig:
		M_SerialConfig_Draw();
		break;

	case m_modemconfig:
		M_ModemConfig_Draw();
		break;

	case m_lanconfig:
		M_LanConfig_Draw();
		break;

	case m_gameoptions:
		M_GameOptions_Draw();
		break;

	case m_search:
		M_Search_Draw();
		break;

	case m_slist:
		M_ServerList_Draw();
		break;
	}

	if (m_entersound)
	{
		S_LocalSound((char*)"misc/menu2.wav");
		m_entersound = false;
	}

	VID_UnlockBuffer();
	S_ExtraUpdate();
	VID_LockBuffer();
}


void M_Keydown(int key)
{
	switch (m_state)
	{
	case m_none:
		return;

	case m_main:
		M_Main_Key(key);
		return;

	case m_singleplayer:
		M_SinglePlayer_Key(key);
		return;

	case m_load:
		M_Load_Key(key);
		return;

	case m_save:
		M_Save_Key(key);
		return;

	case m_multiplayer:
		M_MultiPlayer_Key(key);
		return;

	case m_setup:
		M_Setup_Key(key);
		return;

	case m_net:
		M_Net_Key(key);
		return;

	case m_options:
		M_Options_Key(key);
		return;

	case m_keys:
		M_Keys_Key(key);
		return;

	case m_video:
		M_Video_Key(key);
		return;

	case m_help:
		M_Help_Key(key);
		return;

	case m_quit:
		M_Quit_Key(key);
		return;

	case m_serialconfig:
		M_SerialConfig_Key(key);
		return;

	case m_modemconfig:
		M_ModemConfig_Key(key);
		return;

	case m_lanconfig:
		M_LanConfig_Key(key);
		return;

	case m_gameoptions:
		M_GameOptions_Key(key);
		return;

	case m_search:
		M_Search_Key(key);
		break;

	case m_slist:
		M_ServerList_Key(key);
	}
}


void M_ConfigureNetSubsystem(void)
{
	// enable/disable net systems to match desired config

	Cbuf_AddText((char*)"stopdemo\n");
	if (SerialConfig || DirectConfig)
	{
		Cbuf_AddText((char*)"com1 enable\n");
	}

	if (IPXConfig || TCPIPConfig)
		net_hostport = lanConfig_port;
}
