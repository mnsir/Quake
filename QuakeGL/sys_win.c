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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/
// sys_win.c -- Win32 system interface code

#include "quakedef.h"
#include "winquake.h"
#include "resource.h"
#include "conproc.h"

#include <errno.h>
#include <appapi.h>


#define CONSOLE_ERROR_TIMEOUT 60.0 // # of seconds to wait on Sys_Error running
 // dedicated before exiting
#define PAUSE_SLEEP 50 // sleep time on pause or minimization
#define NOT_FOCUS_SLEEP 20 // sleep time when not focus

AppAPI* g_pAppApi = NULL;

int starttime;
bool ActiveApp, Minimized;
bool WinNT = true;

bool isDedicated;
static bool sc_return_on_enter = false;
HANDLE hinput, houtput;

static char* tracking_tag = "Clams & Mooses";

static HANDLE tevent;
static HANDLE hFile;
static HANDLE heventParent;
static HANDLE heventChild;

void MaskExceptions();
void Sys_InitFloatTime();
void Sys_PushFPCW_SetHigh();
void Sys_PopFPCW();


/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES 10
FILE* sys_handles[MAX_HANDLES];

int findhandle()
{
	int i;

	for (i = 1; i < MAX_HANDLES; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error("out of handles");
	return -1;
}

/*
================
filelength
================
*/
int filelength(FILE* f)
{
	int pos;
	int end;
	int t;

	t = VID_ForceUnlockedAndReturnState();

	pos = ftell(f);
	fseek(f, 0, SEEK_END);
	end = ftell(f);
	fseek(f, pos, SEEK_SET);

	VID_ForceLockState(t);

	return end;
}

int Sys_FileOpenRead(char* path, int* hndl)
{
	FILE* f;
	int i, retval;
	int t;

	t = VID_ForceUnlockedAndReturnState();

	i = findhandle();

	f = fopen(path, "rb");

	if (!f)
	{
		*hndl = -1;
		retval = -1;
	}
	else
	{
		sys_handles[i] = f;
		*hndl = i;
		retval = filelength(f);
	}

	VID_ForceLockState(t);

	return retval;
}

int Sys_FileOpenWrite(char* path)
{
	FILE* f;
	int i;
	int t;

	t = VID_ForceUnlockedAndReturnState();

	i = findhandle();

	f = fopen(path, "wb");
	if (!f)
		Sys_Error("Error opening %s: %s", path, strerror(errno));
	sys_handles[i] = f;

	VID_ForceLockState(t);

	return i;
}

void Sys_FileClose(int handle)
{
	int t;

	t = VID_ForceUnlockedAndReturnState();
	fclose(sys_handles[handle]);
	sys_handles[handle] = NULL;
	VID_ForceLockState(t);
}

void Sys_FileSeek(int handle, int position)
{
	int t;

	t = VID_ForceUnlockedAndReturnState();
	fseek(sys_handles[handle], position, SEEK_SET);
	VID_ForceLockState(t);
}

int Sys_FileRead(int handle, void* dest, int count)
{
	int t, x;

	t = VID_ForceUnlockedAndReturnState();
	x = fread(dest, 1, count, sys_handles[handle]);
	VID_ForceLockState(t);
	return x;
}

int Sys_FileWrite(int handle, void* data, int count)
{
	int t, x;

	t = VID_ForceUnlockedAndReturnState();
	x = fwrite(data, 1, count, sys_handles[handle]);
	VID_ForceLockState(t);
	return x;
}

int Sys_FileTime(char* path)
{
	FILE* f;
	int t, retval;

	t = VID_ForceUnlockedAndReturnState();

	f = fopen(path, "rb");

	if (f)
	{
		fclose(f);
		retval = 1;
	}
	else
	{
		retval = -1;
	}

	VID_ForceLockState(t);
	return retval;
}

void Sys_mkdir(char* path)
{
	_mkdir(path);
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

/*
================
Sys_MakeCodeWriteable
================
*/
void Sys_MakeCodeWriteable(unsigned long startaddr, unsigned long length)
{
	DWORD flOldProtect;

	if (!VirtualProtect((LPVOID)startaddr, length, PAGE_READWRITE, &flOldProtect))
		Sys_Error("Protection change failed\n");
}


void Sys_LowFPPrecision() {}
void Sys_HighFPPrecision() {}



void Sys_Error(char* error, ...)
{
	va_list argptr;
	char text[1024], text2[1024];
	char* text3 = "Press Enter to exit\n";
	char* text4 = "***********************************\n";
	char* text5 = "\n";
	DWORD dummy;
	double starttime;
	static int in_sys_error0 = 0;
	static int in_sys_error1 = 0;
	static int in_sys_error2 = 0;
	static int in_sys_error3 = 0;

	if (!in_sys_error3)
	{
		in_sys_error3 = 1;
		VID_ForceUnlockedAndReturnState();
	}

	va_start(argptr, error);
	vsprintf(text, error, argptr);
	va_end(argptr);

	if (isDedicated)
	{
		va_start(argptr, error);
		vsprintf(text, error, argptr);
		va_end(argptr);

		sprintf(text2, "ERROR: %s\n", text);
		WriteFile(houtput, text5, strlen(text5), &dummy, NULL);
		WriteFile(houtput, text4, strlen(text4), &dummy, NULL);
		WriteFile(houtput, text2, strlen(text2), &dummy, NULL);
		WriteFile(houtput, text3, strlen(text3), &dummy, NULL);
		WriteFile(houtput, text4, strlen(text4), &dummy, NULL);


		starttime = g_pAppApi->Sys_FloatTime();
		sc_return_on_enter = true; // so Enter will get us out of here

		while (!Sys_ConsoleInput() &&
			((g_pAppApi->Sys_FloatTime() - starttime) < CONSOLE_ERROR_TIMEOUT))
		{
		}
	}
	else
	{
		// switch to windowed so the message box is visible, unless we already
		// tried that and failed
		if (!in_sys_error0)
		{
			in_sys_error0 = 1;
			VID_SetDefaultMode();
			MessageBox(NULL, text, "Quake Error",
				MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
		}
		else
		{
			MessageBox(NULL, text, "Double Quake Error",
				MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
		}
	}

	if (!in_sys_error1)
	{
		in_sys_error1 = 1;
		Host_Shutdown();
	}

	// shut down QHOST hooks if necessary
	if (!in_sys_error2)
	{
		in_sys_error2 = 1;
		DeinitConProc();
	}

	exit(1);
}

void Sys_Printf(char* fmt, ...)
{
	va_list argptr;
	char text[1024];
	DWORD dummy;

	if (isDedicated)
	{
		va_start(argptr, fmt);
		vsprintf(text, fmt, argptr);
		va_end(argptr);

		WriteFile(houtput, text, strlen(text), &dummy, NULL);
	}
}

void Sys_Quit()
{

	VID_ForceUnlockedAndReturnState();

	Host_Shutdown();

	if (tevent)
		CloseHandle(tevent);

	if (isDedicated)
		FreeConsole();

	// shut down QHOST hooks if necessary
	DeinitConProc();

	exit(0);
}


char* Sys_ConsoleInput()
{
	static char text[256];
	static int len;
	INPUT_RECORD recs[1024];
	int dummy;
	int ch, numread, numevents;

	if (!isDedicated)
		return NULL;


	for (;; )
	{
		if (!GetNumberOfConsoleInputEvents(hinput, &numevents))
			Sys_Error("Error getting # of console events");

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(hinput, recs, 1, &numread))
			Sys_Error("Error reading console input");

		if (numread != 1)
			Sys_Error("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT)
		{
			if (!recs[0].Event.KeyEvent.bKeyDown)
			{
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
				case '\r':
					WriteFile(houtput, "\r\n", 2, &dummy, NULL);

					if (len)
					{
						text[len] = 0;
						len = 0;
						return text;
					}
					else if (sc_return_on_enter)
					{
						// special case to allow exiting from the error handler on Enter
						text[0] = '\r';
						len = 0;
						return text;
					}

					break;

				case '\b':
					WriteFile(houtput, "\b \b", 3, &dummy, NULL);
					if (len)
					{
						len--;
					}
					break;

				default:
					if (ch >= ' ')
					{
						WriteFile(houtput, &ch, 1, &dummy, NULL);
						text[len] = ch;
						len = (len + 1) & 0xff;
					}

					break;

				}
			}
		}
	}

	return NULL;
}

void Sys_Sleep()
{
	Sleep(1);
}


void Sys_SendKeyEvents()
{
	MSG msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		// we always update if there are any event, even if we're paused
		scr_skipupdate = 0;

		if (!GetMessage(&msg, NULL, 0, 0))
			Sys_Quit();

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}


/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/


/*
==================
WinMain
==================
*/
void SleepUntilInput(int time)
{

	MsgWaitForMultipleObjects(1, &tevent, FALSE, time, QS_ALLINPUT);
}


/*
==================
WinMain
==================
*/
HWND hwnd_dialog;


__declspec(dllexport) void CALLBACK Setup()
{
	isDedicated = false;
	hwnd_dialog = CreateDialog(g_pAppApi->GetAppInstance(), MAKEINTRESOURCE(IDD_DIALOG1), NULL, NULL);

	if (hwnd_dialog)
	{
		RECT rect;
		if (GetWindowRect(hwnd_dialog, &rect))
		{
			if (rect.left > (rect.top * 2))
			{
				SetWindowPos(hwnd_dialog, 0,
					(rect.left / 2) - ((rect.right - rect.left) / 2),
					rect.top, 0, 0,
					SWP_NOZORDER | SWP_NOSIZE);
			}
		}

		ShowWindow(hwnd_dialog, SW_SHOWDEFAULT);
		UpdateWindow(hwnd_dialog);
		SetForegroundWindow(hwnd_dialog);
	}

	tevent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (!tevent)
		Sys_Error("Couldn't create event");

	// because sound is off until we become active
	S_BlockSound();

	Sys_Printf("Host_Init\n");
	Host_Init();
}


__declspec(dllexport) double CALLBACK Loop(double old_time)
{
	// yield the CPU for a little while when paused, minimized, or not the focus
	if ((cl.paused && (!ActiveApp && !DDActive)) || Minimized || block_drawing)
	{
		SleepUntilInput(PAUSE_SLEEP);
		scr_skipupdate = 1; // no point in bothering to draw
	}
	else if (!ActiveApp && !DDActive)
	{
		SleepUntilInput(NOT_FOCUS_SLEEP);
	}

	double newtime = g_pAppApi->Sys_FloatTime();
	double time = newtime - old_time;

	Host_Frame(time);
	return newtime;
}


__declspec(dllexport) void CALLBACK Setup_Dedicated()
{
	isDedicated = true;
	tevent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (!tevent)
		Sys_Error("Couldn't create event");

	if (!AllocConsole())
	{
		Sys_Error("Couldn't create dedicated server console");
	}

	hinput = GetStdHandle(STD_INPUT_HANDLE);
	houtput = GetStdHandle(STD_OUTPUT_HANDLE);

	int t;
	// give QHOST a chance to hook into the console
	if ((t = g_pAppApi->Args_GetIndex("-HFILE")) > 0)
	{
		if (t < g_pAppApi->Args_GetCount())
			hFile = (HANDLE)Q_atoi(g_pAppApi->Args_GetByIndex(t + 1));
	}

	if ((t = g_pAppApi->Args_GetIndex("-HPARENT")) > 0)
	{
		if (t < g_pAppApi->Args_GetCount())
			heventParent = (HANDLE)Q_atoi(g_pAppApi->Args_GetByIndex(t + 1));
	}

	if ((t = g_pAppApi->Args_GetIndex("-HCHILD")) > 0)
	{
		if (t < g_pAppApi->Args_GetCount())
			heventChild = (HANDLE)Q_atoi(g_pAppApi->Args_GetByIndex(t + 1));
	}

	InitConProc(hFile, heventParent, heventChild);

	// because sound is off until we become active
	S_BlockSound();

	Sys_Printf("Host_Init\n");
	Host_Init();
}


__declspec(dllexport) double CALLBACK Loop_Dedicated(double old_time)
{
	double newtime = g_pAppApi->Sys_FloatTime();
	double time = newtime - old_time;

	while (time < sys_ticrate.value)
	{
		Sys_Sleep();
		newtime = g_pAppApi->Sys_FloatTime();
		time = newtime - old_time;
	}

	Host_Frame(time);
	return newtime;
}


__declspec(dllexport) void CALLBACK Initialize(AppAPI* pAppApi)
{
	g_pAppApi = pAppApi;
}


__declspec(dllexport) void CALLBACK Lib_M_ToggleMenu_f()
{
	M_ToggleMenu_f();
}


__declspec(dllexport) void CALLBACK Lib_Con_Printf(const char* str)
{
	Con_Printf(str);
}
