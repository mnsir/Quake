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
// winquake.h: Win32-specific Quake header file

#include <windows.h>
#define WM_MOUSEWHEEL 0x020A

#ifndef SERVERONLY
#include <ddraw.h>
#include <dsound.h>
#endif

extern int global_nCmdShow;

#ifndef SERVERONLY

extern LPDIRECTDRAW lpDD;
extern bool DDActive;
extern LPDIRECTDRAWSURFACE lpPrimary;
extern LPDIRECTDRAWSURFACE lpFrontBuffer;
extern LPDIRECTDRAWSURFACE lpBackBuffer;
extern LPDIRECTDRAWPALETTE lpDDPal;
extern LPDIRECTSOUND pDS;
extern LPDIRECTSOUNDBUFFER pDSBuf;

extern DWORD gSndBufSize;
//#define SNDBUFSIZE 65536

void VID_LockBuffer();
void VID_UnlockBuffer();

#endif

typedef enum { MS_WINDOWED, MS_FULLSCREEN, MS_FULLDIB, MS_UNINIT } modestate_t;

extern modestate_t modestate;

extern HWND mainwindow;
extern bool ActiveApp, Minimized;

extern bool WinNT;

int VID_ForceUnlockedAndReturnState();
void VID_ForceLockState(int lk);

void IN_ShowMouse();
void IN_DeactivateMouse();
void IN_HideMouse();
void IN_ActivateMouse();
void IN_RestoreOriginalMouseState();
void IN_SetQuakeMouseState();
void IN_MouseEvent(int mstate);

extern bool winsock_lib_initialized;

extern cvar_t _windowed_mouse;

extern int window_center_x, window_center_y;
extern RECT window_rect;

extern bool mouseinitialized;
extern HWND hwnd_dialog;

extern HANDLE hinput, houtput;

void IN_UpdateClipCursor();
void CenterWindow(HWND hWndCenter, int width, int height, BOOL lefttopjustify);

void S_BlockSound();
void S_UnblockSound();

void VID_SetDefaultMode();

int (PASCAL FAR * pWSAStartup)(WORD wVersionRequired, LPWSADATA lpWSAData);
int (PASCAL FAR * pWSACleanup)();
int (PASCAL FAR * pWSAGetLastError)();
SOCKET(PASCAL FAR * psocket)(int af, int type, int protocol);
int (PASCAL FAR * pioctlsocket)(SOCKET s, long cmd, u_long FAR * argp);
int (PASCAL FAR * psetsockopt)(SOCKET s, int level, int optname,
                               const char FAR * optval, int optlen);
int (PASCAL FAR * precvfrom)(SOCKET s, char FAR * buf, int len, int flags,
                             struct sockaddr FAR * from, int FAR * fromlen);
int (PASCAL FAR * psendto)(SOCKET s, const char FAR * buf, int len, int flags,
                           const struct sockaddr FAR * to, int tolen);
int (PASCAL FAR * pclosesocket)(SOCKET s);
int (PASCAL FAR * pgethostname)(char FAR * name, int namelen);
struct hostent FAR * (PASCAL FAR * pgethostbyname)(const char FAR * name);
struct hostent FAR * (PASCAL FAR * pgethostbyaddr)(const char FAR * addr,
    int len, int type);
int (PASCAL FAR * pgetsockname)(SOCKET s, struct sockaddr FAR * name,
                                int FAR * namelen);
