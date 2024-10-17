
// sys_win.c -- Win32 system interface code

#include "quakedef.h"
#include "winquake.h"
#include "resource.h"
#include "conproc.h"

#include <errno.h>


#define MINIMUM_WIN_MEMORY 0x0880000
#define MAXIMUM_WIN_MEMORY 0x1000000

#define CONSOLE_ERROR_TIMEOUT 60.0 // # of seconds to wait on Sys_Error running
 // dedicated before exiting
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

static char * tracking_tag = "Clams & Mooses";

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
FILE * sys_handles[MAX_HANDLES];

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
int filelength(FILE * f)
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

int Sys_FileOpenRead(char * path, int * hndl)
{
    FILE * f;
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

int Sys_FileOpenWrite(char * path)
{
    FILE * f;
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

int Sys_FileRead(int handle, void * dest, int count)
{
    int t, x;

    t = VID_ForceUnlockedAndReturnState();
    x = fread(dest, 1, count, sys_handles[handle]);
    VID_ForceLockState(t);
    return x;
}

int Sys_FileWrite(int handle, void * data, int count)
{
    int t, x;

    t = VID_ForceUnlockedAndReturnState();
    x = fwrite(data, 1, count, sys_handles[handle]);
    VID_ForceLockState(t);
    return x;
}

int Sys_FileTime(char * path)
{
    FILE * f;
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

void Sys_mkdir(char * path)
{
    //TODO
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

void Sys_SetFPCW()
{
}

void Sys_PushFPCW_SetHigh()
{
}

void Sys_PopFPCW()
{
}

void MaskExceptions()
{
}


/*
================
Sys_Init
================
*/
void Sys_Init()
{
    LARGE_INTEGER PerformanceFreq;
    unsigned int lowpart, highpart;

    MaskExceptions();
    Sys_SetFPCW();

    if (!QueryPerformanceFrequency(&PerformanceFreq))
        Sys_Error("No hardware timer available");

    // get 32 out of the 64 time bits such that we have around
    // 1 microsecond resolution
    lowpart = (unsigned int)PerformanceFreq.LowPart;
    highpart = (unsigned int)PerformanceFreq.HighPart;
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


void Sys_Error(char * error, ...)
{
    va_list argptr;
    char text[1024], text2[1024];
    char * text3 = "Press Enter to exit\n";
    char * text4 = "***********************************\n";
    char * text5 = "\n";
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


        starttime = Sys_FloatTime();
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

void Sys_Printf(char * fmt, ...)
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


/*
================
Sys_FloatTime
================
*/
double Sys_FloatTime()
{
    static int sametimecount;
    static unsigned int oldtime;
    static int first = 1;
    LARGE_INTEGER PerformanceCount;
    unsigned int temp, t2;
    double time;

    Sys_PushFPCW_SetHigh();

    QueryPerformanceCounter(&PerformanceCount);

    temp = ((unsigned int)PerformanceCount.LowPart >> lowshift) |
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
            t2 = temp - oldtime;

            time = (double)t2 * pfreq;
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
void Sys_InitFloatTime()
{
    int j;

    Sys_FloatTime();

    j = COM_CheckParm("-starttime");

    if (j)
    {
        curtime = (double)(std::atof(com_argv[j + 1]));
    }
    else
    {
        curtime = 0.0;
    }

    lastcurtime = curtime;
}


char * Sys_ConsoleInput()
{
    static char text[256];
    static int len;
    INPUT_RECORD recs[1024];
    int count;
    int i;
    DWORD dummy;
    int ch;
    DWORD numread;
    DWORD numevents;

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
HINSTANCE global_hInstance;
int global_nCmdShow;
char * argv[MAX_NUM_ARGVS];
static char * empty_string = "";
HWND hwnd_dialog;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    quakeparms_t parms;
    double time, oldtime, newtime;
    static char cwd[1024];
    int t;
    RECT rect;

    /* previous instances do not exist in Win32 */
    if (hPrevInstance)
        return 0;

    global_hInstance = hInstance;
    global_nCmdShow = nCmdShow;

    MEMORYSTATUS lpBuffer;
    lpBuffer.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&lpBuffer);

    if (!GetCurrentDirectory(sizeof(cwd), cwd))
        Sys_Error("Couldn't determine current directory");

    if (cwd[std::strlen(cwd) - 1] == '/')
        cwd[std::strlen(cwd) - 1] = 0;

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

    isDedicated = (COM_CheckParm("-dedicated") != 0);

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
    size_t memsize = lpBuffer.dwAvailPhys;

    if (memsize < MINIMUM_WIN_MEMORY)
        memsize = MINIMUM_WIN_MEMORY;

    if (memsize < (lpBuffer.dwTotalPhys >> 1))
        memsize = lpBuffer.dwTotalPhys >> 1;

    if (memsize > MAXIMUM_WIN_MEMORY)
        memsize = MAXIMUM_WIN_MEMORY;

    if (COM_CheckParm("-heapsize"))
    {
        t = COM_CheckParm("-heapsize") + 1;

        if (t < com_argc)
            memsize = std::atoi(com_argv[t]) * 1024;
    }

    if (COM_CheckParm("-minmemory"))
        memsize = minimum_memory;

    parms.mem = std::vector<byte>(memsize, byte());

    tevent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!tevent)
        Sys_Error("Couldn't create event");

    if (isDedicated)
    {
        if (!AllocConsole())
        {
            Sys_Error("Couldn't create dedicated server console");
        }

        hinput = GetStdHandle(STD_INPUT_HANDLE);
        houtput = GetStdHandle(STD_OUTPUT_HANDLE);

        // give QHOST a chance to hook into the console
        if ((t = COM_CheckParm("-HFILE")) > 0)
        {
            if (t < com_argc)
                hFile = (HANDLE)std::atoi(com_argv[t + 1]);
        }

        if ((t = COM_CheckParm("-HPARENT")) > 0)
        {
            if (t < com_argc)
                heventParent = (HANDLE)std::atoi(com_argv[t + 1]);
        }

        if ((t = COM_CheckParm("-HCHILD")) > 0)
        {
            if (t < com_argc)
                heventChild = (HANDLE)std::atoi(com_argv[t + 1]);
        }

        InitConProc(hFile, heventParent, heventChild);
    }

    Sys_Init();

    // because sound is off until we become active
    S_BlockSound();

    Sys_Printf("Host_Init\n");
    Host_Init(parms);

    oldtime = Sys_FloatTime();

    /* main window message loop */
    while (1)
    {
        if (isDedicated)
        {
            newtime = Sys_FloatTime();
            time = newtime - oldtime;

            while (time < sys_ticrate.value)
            {
                Sys_Sleep();
                newtime = Sys_FloatTime();
                time = newtime - oldtime;
            }
        }
        else
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

            newtime = Sys_FloatTime();
            time = newtime - oldtime;
        }

        Host_Frame(time);
        oldtime = newtime;
    }

    /* return success of application */
    return TRUE;
}

