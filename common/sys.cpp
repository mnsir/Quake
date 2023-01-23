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
// sys_win.c -- Win32 system interface code

#include "sys.h"

#include <direct.h>
#include <format>
#include <string_view>

#include "cl_main.h"
#include "common.h"
#include "conproc.h"
#include "host.h"
#include "resource.h"
#include "screen.h"
#include "snd_win.h"
#include "vid.h"


#define MINIMUM_WIN_MEMORY 0x0880000
#define MAXIMUM_WIN_MEMORY 0x1000000

#define CONSOLE_ERROR_TIMEOUT 60.0 // # of seconds to wait on Sys_Error running
//  dedicated before exiting
#define PAUSE_SLEEP 50 // sleep time on pause or minimization
#define NOT_FOCUS_SLEEP 20 // sleep time when not focus

int starttime;
bool ActiveApp, Minimized;
bool WinNT;

static double pfreq;
static double curtime = 0.0;
static double lastcurtime = 0.0;
static int lowshift;
bool isDedicated;
static bool sc_return_on_enter = false;
HANDLE hinput, houtput;

static HANDLE tevent;
static HANDLE hFile;
static HANDLE heventParent;
static HANDLE heventChild;

void MaskExceptions(void);
void Sys_InitFloatTime(void);
void Sys_PushFPCW_SetHigh(void);
void Sys_PopFPCW(void);

volatile int sys_checksum;


/*
================
Sys_PageIn
================
*/
void Sys_PageIn(void* ptr, int size)
{
	byte* x;
	int j;

	// touch all the memory to make sure it's there. The 16-page skip is to
	// keep Win 95 from thinking we're trying to page ourselves in (we are
	// doing that, of course, but there's no reason we shouldn't)
	x = (uint8_t*)ptr;

	for (int n = 0; n < 4; n++)
	{
		for (int m = 0; m < (size - 16 * 0x1000); m += 4)
		{
			sys_checksum += *(int*)&x[m];
			sys_checksum += *(int*)&x[m + 16 * 0x1000];
		}
	}
}


/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES 10
FILE* sys_handles[MAX_HANDLES];

int findhandle(void)
{
	using namespace std::string_view_literals;
	for (int i = 1; i < MAX_HANDLES; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error("out of handles"sv);
	return -1;
}

/*
================
filelength
================
*/
int filelength(FILE* f)
{
	int t = VID_ForceUnlockedAndReturnState();

	int pos = ftell(f);
	fseek(f, 0, SEEK_END);
	int end = ftell(f);
	fseek(f, pos, SEEK_SET);

	VID_ForceLockState(t);

	return end;
}

int Sys_FileOpenRead(char* path, int* hndl)
{
	int retval;

	int t = VID_ForceUnlockedAndReturnState();

	int i = findhandle();

	FILE* f = fopen(path, "rb");

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
	using namespace std::string_view_literals;
	int t = VID_ForceUnlockedAndReturnState();

	int i = findhandle();

	FILE* f = fopen(path, "wb");
	if (!f)
		Sys_Error(std::format("Error opening {}: {}"sv, path, strerror(errno)));
	sys_handles[i] = f;

	VID_ForceLockState(t);

	return i;
}

void Sys_FileClose(int handle)
{
	int t = VID_ForceUnlockedAndReturnState();
	fclose(sys_handles[handle]);
	sys_handles[handle] = NULL;
	VID_ForceLockState(t);
}

void Sys_FileSeek(int handle, int position)
{
	int t = VID_ForceUnlockedAndReturnState();
	fseek(sys_handles[handle], position, SEEK_SET);
	VID_ForceLockState(t);
}

int Sys_FileRead(int handle, void* dest, int count)
{
	int t = VID_ForceUnlockedAndReturnState();
	int x = fread(dest, 1, count, sys_handles[handle]);
	VID_ForceLockState(t);
	return x;
}

int Sys_FileWrite(int handle, void* data, int count)
{
	int t = VID_ForceUnlockedAndReturnState();
	int x = fwrite(data, 1, count, sys_handles[handle]);
	VID_ForceLockState(t);
	return x;
}

int Sys_FileTime(char* path)
{
	int retval;

	int t = VID_ForceUnlockedAndReturnState();

	FILE* f = fopen(path, "rb");

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
	using namespace std::string_view_literals;
	DWORD flOldProtect;

	if (!VirtualProtect((LPVOID)startaddr, length, PAGE_READWRITE, &flOldProtect))
		Sys_Error("Protection change failed\n"sv);
}


void Sys_LowFPPrecision(void)
{
}

void Sys_HighFPPrecision(void)
{
}

void Sys_SetFPCW(void)
{
}

void Sys_PushFPCW_SetHigh(void)
{
}

void Sys_PopFPCW(void)
{
}

void MaskExceptions(void)
{
}


/*
================
Sys_Init
================
*/
void Sys_Init(void)
{
	using namespace std::string_view_literals;
	LARGE_INTEGER PerformanceFreq;

	MaskExceptions();
	Sys_SetFPCW();

	if (!QueryPerformanceFrequency(&PerformanceFreq))
		Sys_Error("No hardware timer available"sv);

	// get 32 out of the 64 time bits such that we have around
	// 1 microsecond resolution
	unsigned int lowpart = (unsigned int)PerformanceFreq.LowPart;
	unsigned int highpart = (unsigned int)PerformanceFreq.HighPart;
	lowshift = 0;

	while (highpart || (lowpart > 2000000.0))
	{
		lowshift++;
		lowpart >>= 1;
		lowpart |= (highpart & 1) << 31;
		highpart >>= 1;
	}

	pfreq = 1.0 / (double)lowpart;

	Sys_InitFloatTime();

	WinNT = true;
}


void Sys_Error(std::string_view err)
{
	using namespace std::string_view_literals;
	static bool in_sys_error3 = false;
	if (!in_sys_error3)
	{
		in_sys_error3 = true;
		VID_ForceUnlockedAndReturnState();
	}
	
	if (isDedicated)
	{
		DWORD dummy;
		constexpr auto text5 = "\n"sv;
		WriteFile(houtput, text5.data(), text5.size(), &dummy, NULL);

		constexpr auto text4 = "***********************************\n"sv;
		WriteFile(houtput, text4.data(), text4.size(), &dummy, NULL);

		auto && text2 = std::format("ERROR: {}\n"sv, err);
		WriteFile(houtput, text2.c_str(), text2.size(), &dummy, NULL);

		constexpr auto text3 = "Press Enter to exit\n"sv;
		WriteFile(houtput, text3.data(), text3.size(), &dummy, NULL);

		WriteFile(houtput, text4.data(), text4.size(), &dummy, NULL);
		
		double starttime = Sys_FloatTime();
		sc_return_on_enter = true; // so Enter will get us out of here

		while (!Sys_ConsoleInput() &&
			((Sys_FloatTime() - starttime) < CONSOLE_ERROR_TIMEOUT))
		{
		}
	}
	else
	{
		// switch to windowed so the message box is visible, unless we already
		// tried that and failed
		static bool in_sys_error0 = false;
		if (!in_sys_error0)
		{
			in_sys_error0 = true;
			VID_SetDefaultMode();
			MessageBox(NULL, err.data(), "Quake Error", MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
		}
		else
		{
			MessageBox(NULL, err.data(), "Double Quake Error", MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
		}
	}

	static bool in_sys_error1 = false;
	if (!in_sys_error1)
	{
		in_sys_error1 = true;
		Host_Shutdown();
	}

	// shut down QHOST hooks if necessary
	static bool in_sys_error2 = false;
	if (!in_sys_error2)
	{
		in_sys_error2 = true;
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

void Sys_Quit(void)
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


/*
================
Sys_FloatTime
================
*/
double Sys_FloatTime(void)
{
	static int sametimecount;
	static unsigned int oldtime;
	static int first = 1;
	LARGE_INTEGER PerformanceCount;

	Sys_PushFPCW_SetHigh();

	QueryPerformanceCounter(&PerformanceCount);

	unsigned int temp = ((unsigned int)PerformanceCount.LowPart >> lowshift) |
		((unsigned int)PerformanceCount.HighPart << (32 - lowshift));

	if (first)
	{
		oldtime = temp;
		first = 0;
	}
	else
	{
		// check for turnover or backward time
		if ((temp <= oldtime) && ((oldtime - temp) < 0x10000000))
		{
			oldtime = temp; // so we can't get stuck
		}
		else
		{
			unsigned int t2 = temp - oldtime;

			double time = (double)t2 * pfreq;
			oldtime = temp;

			curtime += time;

			if (curtime == lastcurtime)
			{
				sametimecount++;

				if (sametimecount > 100000)
				{
					curtime += 1.0;
					sametimecount = 0;
				}
			}
			else
			{
				sametimecount = 0;
			}

			lastcurtime = curtime;
		}
	}

	Sys_PopFPCW();

	return curtime;
}


/*
================
Sys_InitFloatTime
================
*/
void Sys_InitFloatTime(void)
{
	Sys_FloatTime();

	int j = COM_CheckParm((char*)"-starttime");

	if (j)
	{
		curtime = (double)(Q_atof(com_argv[j + 1]));
	}
	else
	{
		curtime = 0.0;
	}

	lastcurtime = curtime;
}


char* Sys_ConsoleInput(void)
{
	using namespace std::string_view_literals;
	static char text[256];
	static int len;
	INPUT_RECORD recs[1024];
	int count;
	int i, dummy;
	int ch, numread, numevents;

	if (!isDedicated)
		return NULL;


	for (;;)
	{
		if (!GetNumberOfConsoleInputEvents(hinput, (LPDWORD)&numevents))
			Sys_Error("Error getting # of console events"sv);

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(hinput, recs, 1, (LPDWORD)&numread))
			Sys_Error("Error reading console input"sv);

		if (numread != 1)
			Sys_Error("Couldn't read console input"sv);

		if (recs[0].EventType == KEY_EVENT)
		{
			if (!recs[0].Event.KeyEvent.bKeyDown)
			{
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
				case '\r':
					WriteFile(houtput, "\r\n", 2, (LPDWORD)&dummy, NULL);

					if (len)
					{
						text[len] = 0;
						len = 0;
						return text;
					}
					if (sc_return_on_enter)
					{
						// special case to allow exiting from the error handler on Enter
						text[0] = '\r';
						len = 0;
						return text;
					}

					break;

				case '\b':
					WriteFile(houtput, "\b \b", 3, (LPDWORD)&dummy, NULL);
					if (len)
					{
						len--;
					}
					break;

				default:
					if (ch >= ' ')
					{
						WriteFile(houtput, &ch, 1, (LPDWORD)&dummy, NULL);
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

void Sys_Sleep(void)
{
	Sleep(1);
}


void Sys_SendKeyEvents(void)
{
	MSG msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		// we always update if there are any event, even if we're paused
		scr_skipupdate = false;

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
HINSTANCE global_hInstance;
int global_nCmdShow;
char* argv[MAX_NUM_ARGVS];
static char* empty_string = (char*)"";
HWND hwnd_dialog;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	using namespace std::string_view_literals;
	MSG msg;
	quakeparms_t parms;
	double time, new_time;
	MEMORYSTATUS lpBuffer;
	static char cwd[1024];
	int t;
	RECT rect;

	/* previous instances do not exist in Win32 */
	if (hPrevInstance)
		return 0;

	global_hInstance = hInstance;
	global_nCmdShow = nCmdShow;

	lpBuffer.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&lpBuffer);

	if (!GetCurrentDirectory(sizeof(cwd), cwd))
		Sys_Error("Couldn't determine current directory"sv);

	if (cwd[Q_strlen(cwd) - 1] == '/')
		cwd[Q_strlen(cwd) - 1] = 0;

	parms.basedir = cwd;
	parms.cachedir = NULL;

	parms.argc = 1;
	argv[0] = empty_string;

	while (*lpCmdLine && (parms.argc < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			argv[parms.argc] = lpCmdLine;
			parms.argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}
		}
	}

	parms.argv = argv;

	COM_InitArgv(parms.argc, parms.argv);

	parms.argc = com_argc;
	parms.argv = com_argv;

	isDedicated = (COM_CheckParm((char*)"-dedicated") != 0);

	if (!isDedicated)
	{
		hwnd_dialog = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, NULL);

		if (hwnd_dialog)
		{
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
	}

	// take the greater of all the available memory or half the total memory,
	// but at least 8 Mb and no more than 16 Mb, unless they explicitly
	// request otherwise
	parms.memsize = lpBuffer.dwAvailPhys;

	if (parms.memsize < MINIMUM_WIN_MEMORY)
		parms.memsize = MINIMUM_WIN_MEMORY;

	if (parms.memsize < (lpBuffer.dwTotalPhys >> 1))
		parms.memsize = lpBuffer.dwTotalPhys >> 1;

	if (parms.memsize > MAXIMUM_WIN_MEMORY)
		parms.memsize = MAXIMUM_WIN_MEMORY;

	if (COM_CheckParm((char*)"-heapsize"))
	{
		t = COM_CheckParm((char*)"-heapsize") + 1;

		if (t < com_argc)
			parms.memsize = Q_atoi(com_argv[t]) * 1024;
	}

	parms.membase = malloc(parms.memsize);

	if (!parms.membase)
		Sys_Error("Not enough memory free; check disk space\n"sv);

	Sys_PageIn(parms.membase, parms.memsize);

	tevent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (!tevent)
		Sys_Error("Couldn't create event"sv);

	if (isDedicated)
	{
		if (!AllocConsole())
		{
			Sys_Error("Couldn't create dedicated server console"sv);
		}

		hinput = GetStdHandle(STD_INPUT_HANDLE);
		houtput = GetStdHandle(STD_OUTPUT_HANDLE);

		// give QHOST a chance to hook into the console
		if ((t = COM_CheckParm((char*)"-HFILE")) > 0)
		{
			if (t < com_argc)
				hFile = (HANDLE)Q_atoi(com_argv[t + 1]);
		}

		if ((t = COM_CheckParm((char*)"-HPARENT")) > 0)
		{
			if (t < com_argc)
				heventParent = (HANDLE)Q_atoi(com_argv[t + 1]);
		}

		if ((t = COM_CheckParm((char*)"-HCHILD")) > 0)
		{
			if (t < com_argc)
				heventChild = (HANDLE)Q_atoi(com_argv[t + 1]);
		}

		InitConProc(hFile, heventParent, heventChild);
	}

	Sys_Init();

	// because sound is off until we become active
	S_BlockSound();

	Sys_Printf((char*)"Host_Init\n");
	Host_Init(&parms);

	double oldtime = Sys_FloatTime();

	/* main window message loop */
	while (true)
	{
		if (isDedicated)
		{
			new_time = Sys_FloatTime();
			time = new_time - oldtime;

			while (time < sys_ticrate.value)
			{
				Sys_Sleep();
				new_time = Sys_FloatTime();
				time = new_time - oldtime;
			}
		}
		else
		{
			// yield the CPU for a little while when paused, minimized, or not the focus
			if ((cl.paused && (!ActiveApp && !DDActive)) || Minimized || block_drawing)
			{
				SleepUntilInput(PAUSE_SLEEP);
				scr_skipupdate = true; // no point in bothering to draw
			}
			else if (!ActiveApp && !DDActive)
			{
				SleepUntilInput(NOT_FOCUS_SLEEP);
			}

			new_time = Sys_FloatTime();
			time = new_time - oldtime;
		}

		Host_Frame(time);
		oldtime = new_time;
	}

	/* return success of application */
	return TRUE;
}
