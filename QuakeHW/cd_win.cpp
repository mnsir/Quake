
// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.

#include <windows.h>
#include "quakedef.h"

extern HWND mainwindow;
extern cvar_t bgmvolume;

static bool cdValid = false;
static bool playing = false;
static bool wasPlaying = false;
static bool initialized = false;
static bool enabled = false;
static bool playLooping = false;
static float cdvolume;
static byte remap[100];
static byte cdrom;
static byte playTrack;
static byte maxTrack;

UINT wDeviceID;


static void CDAudio_Eject()
{
    DWORD dwReturn;

    if (dwReturn = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_DOOR_OPEN, (DWORD)NULL))
        Con_DPrintf((char*)"MCI_SET_DOOR_OPEN failed (%i)\n", dwReturn);
}


static void CDAudio_CloseDoor()
{
    DWORD dwReturn;

    if (dwReturn = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_DOOR_CLOSED, (DWORD)NULL))
        Con_DPrintf((char*)"MCI_SET_DOOR_CLOSED failed (%i)\n", dwReturn);
}


static int CDAudio_GetAudioDiskInfo()
{
    DWORD dwReturn;
    MCI_STATUS_PARMS mciStatusParms;


    cdValid = false;

    mciStatusParms.dwItem = MCI_STATUS_READY;
    dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD)(LPVOID)&mciStatusParms);
    if (dwReturn)
    {
        Con_DPrintf((char*)"CDAudio: drive ready test - get status failed\n");
        return -1;
    }
    if (!mciStatusParms.dwReturn)
    {
        Con_DPrintf((char*)"CDAudio: drive not ready\n");
        return -1;
    }

    mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD)(LPVOID)&mciStatusParms);
    if (dwReturn)
    {
        Con_DPrintf((char*)"CDAudio: get tracks - status failed\n");
        return -1;
    }
    if (mciStatusParms.dwReturn < 1)
    {
        Con_DPrintf((char*)"CDAudio: no music tracks\n");
        return -1;
    }

    cdValid = true;
    maxTrack = mciStatusParms.dwReturn;

    return 0;
}


void CDAudio_Play(byte track, bool looping)
{
    DWORD dwReturn;
    MCI_PLAY_PARMS mciPlayParms;
    MCI_STATUS_PARMS mciStatusParms;

    if (!enabled)
        return;

    if (!cdValid)
    {
        CDAudio_GetAudioDiskInfo();
        if (!cdValid)
            return;
    }

    track = remap[track];

    if (track < 1 || track > maxTrack)
    {
        Con_DPrintf((char*)"CDAudio: Bad track number %u.\n", track);
        return;
    }

    // don't try to play a non-audio track
    mciStatusParms.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
    mciStatusParms.dwTrack = track;
    dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD)(LPVOID)&mciStatusParms);
    if (dwReturn)
    {
        Con_DPrintf((char*)"MCI_STATUS failed (%i)\n", dwReturn);
        return;
    }
    if (mciStatusParms.dwReturn != MCI_CDA_TRACK_AUDIO)
    {
        Con_Printf((char*)"CDAudio: track %i is not audio\n", track);
        return;
    }

    // get the length of the track to be played
    mciStatusParms.dwItem = MCI_STATUS_LENGTH;
    mciStatusParms.dwTrack = track;
    dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD)(LPVOID)&mciStatusParms);
    if (dwReturn)
    {
        Con_DPrintf((char*)"MCI_STATUS failed (%i)\n", dwReturn);
        return;
    }

    if (playing)
    {
        if (playTrack == track)
            return;
        CDAudio_Stop();
    }

    mciPlayParms.dwFrom = MCI_MAKE_TMSF(track, 0, 0, 0);
    mciPlayParms.dwTo = (mciStatusParms.dwReturn << 8) | track;
    mciPlayParms.dwCallback = (DWORD)mainwindow;
    dwReturn = mciSendCommand(wDeviceID, MCI_PLAY, MCI_NOTIFY | MCI_FROM | MCI_TO, (DWORD)(LPVOID)&mciPlayParms);
    if (dwReturn)
    {
        Con_DPrintf((char*)"CDAudio: MCI_PLAY failed (%i)\n", dwReturn);
        return;
    }

    playLooping = looping;
    playTrack = track;
    playing = true;

    if (cdvolume == 0.0)
        CDAudio_Pause();
}


void CDAudio_Stop()
{
    DWORD dwReturn;

    if (!enabled)
        return;

    if (!playing)
        return;

    if (dwReturn = mciSendCommand(wDeviceID, MCI_STOP, 0, (DWORD)NULL))
        Con_DPrintf((char*)"MCI_STOP failed (%i)", dwReturn);

    wasPlaying = false;
    playing = false;
}


void CDAudio_Pause()
{
    DWORD dwReturn;
    MCI_GENERIC_PARMS mciGenericParms;

    if (!enabled)
        return;

    if (!playing)
        return;

    mciGenericParms.dwCallback = (DWORD)mainwindow;
    if (dwReturn = mciSendCommand(wDeviceID, MCI_PAUSE, 0, (DWORD)(LPVOID)&mciGenericParms))
        Con_DPrintf((char*)"MCI_PAUSE failed (%i)", dwReturn);

    wasPlaying = playing;
    playing = false;
}


void CDAudio_Resume()
{
    DWORD dwReturn;
    MCI_PLAY_PARMS mciPlayParms;

    if (!enabled)
        return;

    if (!cdValid)
        return;

    if (!wasPlaying)
        return;

    mciPlayParms.dwFrom = MCI_MAKE_TMSF(playTrack, 0, 0, 0);
    mciPlayParms.dwTo = MCI_MAKE_TMSF(playTrack + 1, 0, 0, 0);
    mciPlayParms.dwCallback = (DWORD)mainwindow;
    dwReturn = mciSendCommand(wDeviceID, MCI_PLAY, MCI_TO | MCI_NOTIFY, (DWORD)(LPVOID)&mciPlayParms);
    if (dwReturn)
    {
        Con_DPrintf((char*)"CDAudio: MCI_PLAY failed (%i)\n", dwReturn);
        return;
    }
    playing = true;
}


static void CD_f()
{
    char * command;
    int ret;
    int n;
    int startAddress;

    if (Cmd_Argc() < 2)
        return;

    command = Cmd_Argv(1);

    if (Q_strcasecmp(command, (char*)"on") == 0)
    {
        enabled = true;
        return;
    }

    if (Q_strcasecmp(command, (char*)"off") == 0)
    {
        if (playing)
            CDAudio_Stop();
        enabled = false;
        return;
    }

    if (Q_strcasecmp(command, (char*)"reset") == 0)
    {
        enabled = true;
        if (playing)
            CDAudio_Stop();
        for (n = 0; n < 100; n++)
            remap[n] = n;
        CDAudio_GetAudioDiskInfo();
        return;
    }

    if (Q_strcasecmp(command, (char*)"remap") == 0)
    {
        ret = Cmd_Argc() - 2;
        if (ret <= 0)
        {
            for (n = 1; n < 100; n++)
                if (remap[n] != n)
                    Con_Printf((char*)" %u -> %u\n", n, remap[n]);
            return;
        }
        for (n = 1; n <= ret; n++)
            remap[n] = std::atoi(Cmd_Argv(n + 1));
        return;
    }

    if (Q_strcasecmp(command, (char*)"close") == 0)
    {
        CDAudio_CloseDoor();
        return;
    }

    if (!cdValid)
    {
        CDAudio_GetAudioDiskInfo();
        if (!cdValid)
        {
            Con_Printf((char*)"No CD in player.\n");
            return;
        }
    }

    if (Q_strcasecmp(command, (char*)"play") == 0)
    {
        CDAudio_Play((byte)std::atoi(Cmd_Argv(2)), false);
        return;
    }

    if (Q_strcasecmp(command, (char*)"loop") == 0)
    {
        CDAudio_Play((byte)std::atoi(Cmd_Argv(2)), true);
        return;
    }

    if (Q_strcasecmp(command, (char*)"stop") == 0)
    {
        CDAudio_Stop();
        return;
    }

    if (Q_strcasecmp(command, (char*)"pause") == 0)
    {
        CDAudio_Pause();
        return;
    }

    if (Q_strcasecmp(command, (char*)"resume") == 0)
    {
        CDAudio_Resume();
        return;
    }

    if (Q_strcasecmp(command, (char*)"eject") == 0)
    {
        if (playing)
            CDAudio_Stop();
        CDAudio_Eject();
        cdValid = false;
        return;
    }

    if (Q_strcasecmp(command, (char*)"info") == 0)
    {
        Con_Printf((char*)"%u tracks\n", maxTrack);
        if (playing)
            Con_Printf((char*)"Currently %s track %u\n", playLooping ? "looping" : "playing", playTrack);
        else if (wasPlaying)
            Con_Printf((char*)"Paused %s track %u\n", playLooping ? "looping" : "playing", playTrack);
        Con_Printf((char*)"Volume is %f\n", cdvolume);
        return;
    }
}


LONG CDAudio_MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (lParam != wDeviceID)
        return 1;

    switch (wParam)
    {
    case MCI_NOTIFY_SUCCESSFUL:
        if (playing)
        {
            playing = false;
            if (playLooping)
                CDAudio_Play(playTrack, true);
        }
        break;

    case MCI_NOTIFY_ABORTED:
    case MCI_NOTIFY_SUPERSEDED:
        break;

    case MCI_NOTIFY_FAILURE:
        Con_DPrintf((char*)"MCI_NOTIFY_FAILURE\n");
        CDAudio_Stop();
        cdValid = false;
        break;

    default:
        Con_DPrintf((char*)"Unexpected MM_MCINOTIFY type (%i)\n", wParam);
        return 1;
    }

    return 0;
}


void CDAudio_Update()
{
    if (!enabled)
        return;

    if (bgmvolume.value != cdvolume)
    {
        if (cdvolume)
        {
            Cvar_SetValue((char*)"bgmvolume", 0.0);
            cdvolume = bgmvolume.value;
            CDAudio_Pause();
        }
        else
        {
            Cvar_SetValue((char*)"bgmvolume", 1.0);
            cdvolume = bgmvolume.value;
            CDAudio_Resume();
        }
    }
}


int CDAudio_Init()
{
    DWORD dwReturn;
    MCI_OPEN_PARMS mciOpenParms;
    MCI_SET_PARMS mciSetParms;
    int n;

    if (cls.state == ca_dedicated)
        return -1;

    if (COM_CheckParm((char*)"-nocdaudio"))
        return -1;

    mciOpenParms.lpstrDeviceType = "cdaudio";
    if (dwReturn = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE, (DWORD)(LPVOID)&mciOpenParms))
    {
        Con_Printf((char*)"CDAudio_Init: MCI_OPEN failed (%i)\n", dwReturn);
        return -1;
    }
    wDeviceID = mciOpenParms.wDeviceID;

    // Set the time format to track/minute/second/frame (TMSF).
    mciSetParms.dwTimeFormat = MCI_FORMAT_TMSF;
    if (dwReturn = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)(LPVOID)&mciSetParms))
    {
        Con_Printf((char*)"MCI_SET_TIME_FORMAT failed (%i)\n", dwReturn);
        mciSendCommand(wDeviceID, MCI_CLOSE, 0, (DWORD)NULL);
        return -1;
    }

    for (n = 0; n < 100; n++)
        remap[n] = n;
    initialized = true;
    enabled = true;

    if (CDAudio_GetAudioDiskInfo())
    {
        Con_Printf((char*)"CDAudio_Init: No CD in player.\n");
        cdValid = false;
    }

    Cmd_AddCommand((char*)"cd", CD_f);

    Con_Printf((char*)"CD Audio Initialized\n");

    return 0;
}


void CDAudio_Shutdown()
{
    if (!initialized)
        return;
    CDAudio_Stop();
    if (mciSendCommand(wDeviceID, MCI_CLOSE, MCI_WAIT, (DWORD)NULL))
        Con_DPrintf((char*)"CDAudio_Shutdown: MCI_CLOSE failed\n");
}
