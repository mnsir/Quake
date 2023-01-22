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
// winquake.h: Win32-specific Quake header file
#pragma once

#pragma warning( disable : 4229 )  // mgraph gets this_

#include <windows.h>
#define WM_MOUSEWHEEL                   0x020A

#include <ddraw.h>
#include <dsound.h>
#ifndef GLQUAKE
#define __HAS_BOOL__
#include <mgraph.h>
#endif


extern LPDIRECTDRAW lpDD;
extern LPDIRECTDRAWSURFACE lpPrimary;
extern LPDIRECTDRAWSURFACE lpFrontBuffer;
extern LPDIRECTDRAWSURFACE lpBackBuffer;
extern LPDIRECTDRAWPALETTE lpDDPal;

//#define SNDBUFSIZE 65536


using pWSAStartup_t = int (PASCAL FAR* )(WORD wVersionRequired, LPWSADATA lpWSAData);
using pWSACleanup_t = int (PASCAL FAR* )(void);
using pWSAGetLastError_t = int (PASCAL FAR* )(void);
using psocket_t = SOCKET (PASCAL FAR* )(int af, int type, int protocol);
using pioctlsocket_t = int (PASCAL FAR* )(SOCKET s, long cmd, u_long FAR* argp);
using psetsockopt_t = int (PASCAL FAR* )(SOCKET s, int level, int optname, const char FAR* optval, int optlen);
using precvfrom_t = int (PASCAL FAR* )(SOCKET s, char FAR* buf, int len, int flags, struct sockaddr FAR* from, int FAR* fromlen);
using psendto_t = int (PASCAL FAR* )(SOCKET s, const char FAR* buf, int len, int flags, const struct sockaddr FAR* to, int tolen);
using pclosesocket_t = int (PASCAL FAR* )(SOCKET s);
using pgethostname_t = int (PASCAL FAR* )(char FAR* name, int namelen);
using pgethostbyname_t = struct hostent FAR* (PASCAL FAR* )(const char FAR* name);
using pgethostbyaddr_t = struct hostent FAR* (PASCAL FAR* )(const char FAR* addr, int len, int type);
using pgetsockname_t = int (PASCAL FAR* )(SOCKET s, struct sockaddr FAR* name, int FAR* namelen);

extern pWSAStartup_t pWSAStartup;
extern pWSACleanup_t pWSACleanup;
extern pWSAGetLastError_t pWSAGetLastError;
extern psocket_t psocket;
extern pioctlsocket_t pioctlsocket;
extern psetsockopt_t psetsockopt;
extern precvfrom_t precvfrom;
extern psendto_t psendto;
extern pclosesocket_t pclosesocket;
extern pgethostname_t pgethostname;
extern pgethostbyname_t pgethostbyname;
extern pgethostbyaddr_t pgethostbyaddr;
extern pgetsockname_t pgetsockname;