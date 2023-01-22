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
// console.c

#include "console.h"

#include "client.h"
#include "cl_main.h"
#include "cmd.h"
#include "cvar.h"
#include "draw.h"
#include "host.h"
#include "keys.h"
#include "screen.h"
#include "snd_dma.h"
#include "sys.h"


int con_linew_idth;

float con_cursorspeed = 4;

#define CON_TEXTSIZE 16384

bool con_forcedup; // because no entities to refresh

int con_totallines; // total lines in console scrollback
int con_backscroll; // lines up from bottom to display
int con_current; // where next message will be printed
int con_x; // offset in current line for next print
char* con_text = 0;

cvar_t con_notifytime = {(char*)"con_notifytime", (char*)"3"}; //seconds

#define NUM_CON_TIMES 4
float con_times[NUM_CON_TIMES]; // realtime time the line was generated
// for transparent notify lines

int con_vislines;

bool con_debuglog;

#define MAXCMDLINE 256
extern char key_lines[32][MAXCMDLINE];
extern int edit_line;
extern int key_linepos;


bool con_initialized;

int con_notifylines; // scan lines to clear for notify lines

extern void M_Menu_Main_f(void);

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f(void)
{
	if (key_dest == key_console)
	{
		if (cls.state == ca_connected)
		{
			key_dest = key_game;
			key_lines[edit_line][1] = 0; // clear any typing
			key_linepos = 1;
		}
		else
		{
			M_Menu_Main_f();
		}
	}
	else
		key_dest = key_console;

	SCR_EndLoadingPlaque();
	memset(con_times, 0, sizeof(con_times));
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f(void)
{
	if (con_text)
		Q_memset(con_text, ' ', CON_TEXTSIZE);
}


/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify(void)
{
	for (int i = 0; i < NUM_CON_TIMES; i++)
		con_times[i] = 0;
}


/*
================
Con_MessageMode_f
================
*/
extern bool team_message;

void Con_MessageMode_f(void)
{
	key_dest = key_message;
	team_message = false;
}


/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f(void)
{
	key_dest = key_message;
	team_message = true;
}


/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize(void)
{
	char tbuf[CON_TEXTSIZE];

	int width = (vid.width >> 3) - 2;

	if (width == con_linew_idth)
		return;

	if (width < 1) // video hasn't been initialized yet
	{
		width = 38;
		con_linew_idth = width;
		con_totallines = CON_TEXTSIZE / con_linew_idth;
		Q_memset(con_text, ' ', CON_TEXTSIZE);
	}
	else
	{
		int oldwidth = con_linew_idth;
		con_linew_idth = width;
		int oldtotallines = con_totallines;
		con_totallines = CON_TEXTSIZE / con_linew_idth;
		int numlines = oldtotallines;

		if (con_totallines < numlines)
			numlines = con_totallines;

		int numchars = oldwidth;

		if (con_linew_idth < numchars)
			numchars = con_linew_idth;

		Q_memcpy(tbuf, con_text, CON_TEXTSIZE);
		Q_memset(con_text, ' ', CON_TEXTSIZE);

		for (int i = 0; i < numlines; i++)
		{
			for (int j = 0; j < numchars; j++)
			{
				con_text[(con_totallines - 1 - i) * con_linew_idth + j] =
					tbuf[((con_current - i + oldtotallines) %
						oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify();
	}

	con_backscroll = 0;
	con_current = con_totallines - 1;
}


/*
================
Con_Init
================
*/
void Con_Init(void)
{
#define MAXGAMEDIRLEN 1000
	char temp[MAXGAMEDIRLEN + 1];
	char* t2 = (char*)"/qconsole.log";

	con_debuglog = COM_CheckParm((char*)"-condebug");

	if (con_debuglog)
	{
		if (strlen(com_gamedir) < (MAXGAMEDIRLEN - strlen(t2)))
		{
			sprintf(temp, (char*)"%s%s", com_gamedir, t2);
			_unlink(temp);
		}
	}

	con_text = static_cast<char*>(Hunk_AllocName(CON_TEXTSIZE, (char*)"context"));
	Q_memset(con_text, ' ', CON_TEXTSIZE);
	con_linew_idth = -1;
	Con_CheckResize();

	Con_Printf((char*)"Console initialized.\n");

	//
	// register our commands
	//
	Cvar_RegisterVariable(&con_notifytime);

	Cmd_AddCommand((char*)"toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand((char*)"messagemode", Con_MessageMode_f);
	Cmd_AddCommand((char*)"messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand((char*)"clear", Con_Clear_f);
	con_initialized = true;
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed(void)
{
	con_x = 0;
	con_current++;
	Q_memset(&con_text[(con_current % con_totallines) * con_linew_idth]
	         , ' ', con_linew_idth);
}

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this_ in order to be logged to disk
If no console is visible, the notify window will pop up.
================
*/
void Con_Print(char* txt)
{
	int c, l;
	static int cr;
	int mask;

	con_backscroll = 0;

	if (txt[0] == 1)
	{
		mask = 128; // go to colored text
		S_LocalSound((char*)"misc/talk.wav");
		// play talk wav
		txt++;
	}
	else if (txt[0] == 2)
	{
		mask = 128; // go to colored text
		txt++;
	}
	else
		mask = 0;


	while ((c = *txt))
	{
		// count word length
		for (l = 0; l < con_linew_idth; l++)
			if (txt[l] <= ' ')
				break;

		// word wrap
		if (l != con_linew_idth && (con_x + l > con_linew_idth))
			con_x = 0;

		txt++;

		if (cr)
		{
			con_current--;
			cr = false;
		}


		if (!con_x)
		{
			Con_Linefeed();
			// mark time for transparent overlay
			if (con_current >= 0)
				con_times[con_current % NUM_CON_TIMES] = realtime;
		}

		switch (c)
		{
		case '\n':
			con_x = 0;
			break;

		case '\r':
			con_x = 0;
			cr = 1;
			break;

		default: // display character and advance
			int y = con_current % con_totallines;
			con_text[y * con_linew_idth + con_x] = c | mask;
			con_x++;
			if (con_x >= con_linew_idth)
				con_x = 0;
			break;
		}
	}
}


/*
================
Con_DebugLog
================
*/
void Con_DebugLog(char* file, char* fmt, ...)
{
	va_list argptr;
	static char data[1024];

	va_start(argptr, fmt);
	vsnprintf(data, 1024, fmt, argptr);
	va_end(argptr);
	FILE* fd = fopen(file, "a");
	fwrite(data, sizeof(char), strlen(data), fd);
	fclose(fd);
}


/*
================
Con_Printf

Handles cursor positioning, line wrapping, etc
================
*/
#define MAXPRINTMSG 4096
// FIXME: make a buffer size safe vsprintf?
void Con_Printf(char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];
	static bool inupdate;

	va_start(argptr, fmt);
	vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

	// also echo to debugging console
	Sys_Printf((char*)"%s", msg); // also echo to debugging console

	// log all messages to file
	if (con_debuglog)
		Con_DebugLog(va((char*)"%s/qconsole.log", com_gamedir), (char*)"%s", msg);

	if (!con_initialized)
		return;

	if (cls.state == ca_dedicated)
		return; // no graphics mode

	// write it to the scrollable buffer
	Con_Print(msg);

	// update the screen if the console is displayed
	if (cls.signon != SIGNONS && !scr_disabled_for_loading)
	{
		// protect against infinite loop if something in SCR_UpdateScreen calls
		// Con_Printd
		if (!inupdate)
		{
			inupdate = true;
			SCR_UpdateScreen();
			inupdate = false;
		}
	}
}

/*
================
Con_DPrintf

A Con_Printf that only shows up if the "developer" cvar is set
================
*/
void Con_DPrintf(char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];

	if (!developer.value)
		return; // don't confuse non-developers with techie stuff...

	va_start(argptr, fmt);
	vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

	Con_Printf((char*)"%s", msg);
}


/*
==================
Con_SafePrintf

Okay to call even when the screen can't be updated
==================
*/
void Con_SafePrintf(char* fmt, ...)
{
	va_list argptr;
	char msg[1024];

	va_start(argptr, fmt);
	vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

	int temp = scr_disabled_for_loading;
	scr_disabled_for_loading = true;
	Con_Printf((char*)"%s", msg);
	scr_disabled_for_loading = temp;
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
================
*/
void Con_DrawInput(void)
{
	int i;

	if (key_dest != key_console && !con_forcedup)
		return; // don't draw anything

	char* text = key_lines[edit_line];

	// add the cursor frame
	text[key_linepos] = 10 + ((int)(realtime * con_cursorspeed) & 1);

	// fill out remainder with spaces
	for (i = key_linepos + 1; i < con_linew_idth; i++)
		text[i] = ' ';

	// prestep if horizontally scrolling
	if (key_linepos >= con_linew_idth)
		text += 1 + key_linepos - con_linew_idth;

	// draw it
	int y = con_vislines - 16;

	for (i = 0; i < con_linew_idth; i++)
		Draw_Character((i + 1) << 3, con_vislines - 16, text[i]);

	// remove cursor
	key_lines[edit_line][key_linepos] = 0;
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify(void)
{
	int x;
	extern char chat_buffer[];

	int v = 0;
	for (int i = con_current - NUM_CON_TIMES + 1; i <= con_current; i++)
	{
		if (i < 0)
			continue;
		float time = con_times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = realtime - time;
		if (time > con_notifytime.value)
			continue;
		char* text = con_text + (i % con_totallines) * con_linew_idth;

		clearnotify = 0;
		scr_copytop = 1;

		for (x = 0; x < con_linew_idth; x++)
			Draw_Character((x + 1) << 3, v, text[x]);

		v += 8;
	}


	if (key_dest == key_message)
	{
		clearnotify = 0;
		scr_copytop = 1;

		x = 0;

		Draw_String(8, v, (char*)"say:");
		while (chat_buffer[x])
		{
			Draw_Character((x + 5) << 3, v, chat_buffer[x]);
			x++;
		}
		Draw_Character((x + 5) << 3, v, 10 + ((int)(realtime * con_cursorspeed) & 1));
		v += 8;
	}

	if (v > con_notifylines)
		con_notifylines = v;
}

/*
================
Con_DrawConsole

Draws the console with the solid background
The typing input line at the bottom should only be drawn if typing is allowed
================
*/
void Con_DrawConsole(int lines, bool drawinput)
{
	if (lines <= 0)
		return;

	// draw the background
	Draw_ConsoleBackground(lines);

	// draw the text
	con_vislines = lines;

	int rows = (lines - 16) >> 3; // rows of text to draw
	int y = lines - 16 - (rows << 3); // may start slightly negative

	for (int i = con_current - rows + 1; i <= con_current; i++, y += 8)
	{
		int j = i - con_backscroll;
		if (j < 0)
			j = 0;
		char* text = con_text + (j % con_totallines) * con_linew_idth;

		for (int x = 0; x < con_linew_idth; x++)
			Draw_Character((x + 1) << 3, y, text[x]);
	}

	// draw the input prompt, user text, and cursor if desired
	if (drawinput)
		Con_DrawInput();
}


/*
==================
Con_NotifyBox
==================
*/
void Con_NotifyBox(char* text)
{
	// during startup for sound / cd warnings
	Con_Printf(
		(char*)"\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

	Con_Printf(text);

	Con_Printf((char*)"Press a key.\n");
	Con_Printf(
		(char*)"\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

	key_count = -2; // wait for a key down and up
	key_dest = key_console;

	do
	{
		double t1 = Sys_FloatTime();
		SCR_UpdateScreen();
		Sys_SendKeyEvents();
		double t2 = Sys_FloatTime();
		realtime += t2 - t1; // make the cursor blink
	}
	while (key_count < 0);

	Con_Printf((char*)"\n");
	key_dest = key_game;
	realtime = 0; // put the cursor back to invisible
}
