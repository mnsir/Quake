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
// net_wins.c

#include "net_wins.h"

#include "cl_main.h"
#include "console.h"
#include "cvar.h"
#include "sys.h"
#include "winquake.h"

extern cvar_t hostname;

#define MAXHOSTNAMELEN 256

static int net_acceptsocket = -1; // socket for fielding new_ connections
static int net_controlsocket;
static int net_broadcastsocket = 0;
static struct qsockaddr broadcastaddr;

static unsigned long myAddr;

qboolean winsock_lib_initialized;


pWSAStartup_t pWSAStartup;
pWSACleanup_t pWSACleanup;
pWSAGetLastError_t pWSAGetLastError;
psocket_t psocket;
pioctlsocket_t pioctlsocket;
psetsockopt_t psetsockopt;
precvfrom_t precvfrom;
psendto_t psendto;
pclosesocket_t pclosesocket;
pgethostname_t pgethostname;
pgethostbyname_t pgethostbyname;
pgethostbyaddr_t pgethostbyaddr;
pgetsockname_t pgetsockname;

#include "net_wins.h"

int winsock_initialized = 0;
WSADATA winsockdata;

//=============================================================================

static double blocktime;

BOOL PASCAL FAR BlockingHook(void)
{
	MSG msg;
	BOOL ret;

	if ((Sys_FloatTime() - blocktime) > 2.0)
	{
		WSACancelBlockingCall();
		return FALSE;
	}

	/* get the next message, if any */
	ret = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);

	/* if we got one, process it */
	if (ret)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	/* TRUE if we got a message */
	return ret;
}


void WINS_GetLocalAddress()
{
	struct hostent* local = NULL;
	char buff[MAXHOSTNAMELEN];
	unsigned long addr;

	if (myAddr != INADDR_ANY)
		return;

	if (pgethostname(buff, MAXHOSTNAMELEN) == SOCKET_ERROR)
		return;

	blocktime = Sys_FloatTime();
	WSASetBlockingHook(BlockingHook);
	local = pgethostbyname(buff);
	WSAUnhookBlockingHook();
	if (local == NULL)
		return;

	myAddr = *(int*)local->h_addr_list[0];

	addr = ntohl(myAddr);
	sprintf(my_tcpip_address, (char*)"%d.%d.%d.%d", (addr >> 24) & 0xff, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff);
}


int WINS_Init(void)
{
	int i;
	char buff[MAXHOSTNAMELEN];
	char* p;
	int r;
	WORD wVersionRequested;
	HINSTANCE hInst;

	// initialize the Winsock function vectors (we do this_ instead of statically linking
	// so we can run on Win 3.1, where there isn't necessarily Winsock)
	hInst = LoadLibrary((char*)"wsock32.dll");

	if (hInst == NULL)
	{
		Con_SafePrintf((char*)"Failed to load winsock.dll\n");
		winsock_lib_initialized = false;
		return -1;
	}

	winsock_lib_initialized = true;

	pWSAStartup = decltype(pWSAStartup)(GetProcAddress(hInst, (char*)"WSAStartup"));
	pWSACleanup = (decltype(pWSACleanup))GetProcAddress(hInst, (char*)"WSACleanup");
	pWSAGetLastError = (decltype(pWSAGetLastError))GetProcAddress(hInst, (char*)"WSAGetLastError");
	psocket = decltype(psocket)(GetProcAddress(hInst, (char*)"socket"));
	pioctlsocket = (decltype(pioctlsocket))GetProcAddress(hInst, (char*)"ioctlsocket");
	psetsockopt = (decltype(psetsockopt))GetProcAddress(hInst, (char*)"setsockopt");
	precvfrom = (decltype(precvfrom))GetProcAddress(hInst, (char*)"recvfrom");
	psendto = (decltype(psendto))GetProcAddress(hInst, (char*)"sendto");
	pclosesocket = (decltype(pclosesocket))GetProcAddress(hInst, (char*)"closesocket");
	pgethostname = (decltype(pgethostname))GetProcAddress(hInst, (char*)"gethostname");
	pgethostbyname = (decltype(pgethostbyname))GetProcAddress(hInst, (char*)"gethostbyname");
	pgethostbyaddr = (decltype(pgethostbyaddr))GetProcAddress(hInst, (char*)"gethostbyaddr");
	pgetsockname = (decltype(pgetsockname))GetProcAddress(hInst, (char*)"getsockname");

	if (!pWSAStartup || !pWSACleanup || !pWSAGetLastError ||
		!psocket || !pioctlsocket || !psetsockopt ||
		!precvfrom || !psendto || !pclosesocket ||
		!pgethostname || !pgethostbyname || !pgethostbyaddr ||
		!pgetsockname)
	{
		Con_SafePrintf((char*)"Couldn't GetProcAddress from winsock.dll\n");
		return -1;
	}

	if (COM_CheckParm((char*)"-noudp"))
		return -1;

	if (winsock_initialized == 0)
	{
		wVersionRequested = MAKEWORD(1, 1);

		r = pWSAStartup(MAKEWORD(1, 1), &winsockdata);

		if (r)
		{
			Con_SafePrintf((char*)"Winsock initialization failed.\n");
			return -1;
		}
	}
	winsock_initialized++;

	// determine my name
	if (pgethostname(buff, MAXHOSTNAMELEN) == SOCKET_ERROR)
	{
		Con_DPrintf((char*)"Winsock TCP/IP Initialization failed.\n");
		if (--winsock_initialized == 0)
			pWSACleanup();
		return -1;
	}

	// if the quake hostname isn't set, set it to the machine name
	if (Q_strcmp(hostname.string, (char*)"UNNAMED") == 0)
	{
		// see if it's a text IP address (well, close enough)
		for (p = buff; *p; p++)
			if ((*p < '0' || *p > '9') && *p != '.')
				break;

		// if it is a real name, strip off the domain; we only want the host
		if (*p)
		{
			for (i = 0; i < 15; i++)
				if (buff[i] == '.')
					break;
			buff[i] = 0;
		}
		Cvar_Set((char*)"hostname", buff);
	}

	i = COM_CheckParm((char*)"-ip");
	if (i)
	{
		if (i < com_argc - 1)
		{
			myAddr = inet_addr(com_argv[i + 1]);
			if (myAddr == INADDR_NONE)
				Sys_Error((char*)"%s is not a valid IP address", com_argv[i + 1]);
			strcpy(my_tcpip_address, com_argv[i + 1]);
		}
		else
		{
			Sys_Error((char*)"NET_Init: you must specify an IP address after -ip");
		}
	}
	else
	{
		myAddr = INADDR_ANY;
		strcpy(my_tcpip_address, (char*)"INADDR_ANY");
	}

	if ((net_controlsocket = WINS_OpenSocket(0)) == -1)
	{
		Con_Printf((char*)"WINS_Init: Unable to open control socket\n");
		if (--winsock_initialized == 0)
			pWSACleanup();
		return -1;
	}

	((struct sockaddr_in*)&broadcastaddr)->sin_family = AF_INET;
	((struct sockaddr_in*)&broadcastaddr)->sin_addr.s_addr = INADDR_BROADCAST;
	((struct sockaddr_in*)&broadcastaddr)->sin_port = htons(net_hostport);

	Con_Printf((char*)"Winsock TCP/IP Initialized\n");
	tcpipAvailable = true;

	return net_controlsocket;
}

//=============================================================================

void WINS_Shutdown(void)
{
	WINS_Listen(false);
	WINS_CloseSocket(net_controlsocket);
	if (--winsock_initialized == 0)
		pWSACleanup();
}

//=============================================================================

void WINS_Listen(qboolean state)
{
	// enable listening
	if (state)
	{
		if (net_acceptsocket != -1)
			return;
		WINS_GetLocalAddress();
		if ((net_acceptsocket = WINS_OpenSocket(net_hostport)) == -1)
			Sys_Error((char*)"WINS_Listen: Unable to open accept socket\n");
		return;
	}

	// disable listening
	if (net_acceptsocket == -1)
		return;
	WINS_CloseSocket(net_acceptsocket);
	net_acceptsocket = -1;
}

//=============================================================================

int WINS_OpenSocket(int port)
{
	int new_socket;
	struct sockaddr_in address;
	u_long _true = 1;

	if ((new_socket = psocket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		return -1;

	if (pioctlsocket(new_socket, FIONBIO, &_true) == -1)
		goto ErrorReturn;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = myAddr;
	address.sin_port = htons(port);
	if (bind(new_socket, (sockaddr*)&address, sizeof(address)) == 0)
		return new_socket;

	Sys_Error((char*)"Unable to bind to %s", WINS_AddrToString((struct qsockaddr*)&address));
ErrorReturn:
	pclosesocket(new_socket);
	return -1;
}

//=============================================================================

int WINS_CloseSocket(int socket)
{
	if (socket == net_broadcastsocket)
		net_broadcastsocket = 0;
	return pclosesocket(socket);
}


//=============================================================================
/*
============
PartialIPAddress

this_ lets you type only as much of the net address as required, using
the local network components to fill in the rest
============
*/
static int PartialIPAddress(char* in, struct qsockaddr* hostaddr)
{
	char buff[256];
	char* b;
	int addr;
	int num;
	int mask;
	int run;
	int port;

	buff[0] = '.';
	b = buff;
	strcpy(buff + 1, in);
	if (buff[1] == '.')
		b++;

	addr = 0;
	mask = -1;
	while (*b == '.')
	{
		b++;
		num = 0;
		run = 0;
		while (!(*b < '0' || *b > '9'))
		{
			num = num * 10 + *b++ - '0';
			if (++run > 3)
				return -1;
		}
		if ((*b < '0' || *b > '9') && *b != '.' && *b != ':' && *b != 0)
			return -1;
		if (num < 0 || num > 255)
			return -1;
		mask <<= 8;
		addr = (addr << 8) + num;
	}

	if (*b++ == ':')
		port = Q_atoi(b);
	else
		port = net_hostport;

	hostaddr->sa_family = AF_INET;
	((struct sockaddr_in*)hostaddr)->sin_port = htons(port);
	((struct sockaddr_in*)hostaddr)->sin_addr.s_addr = (myAddr & htonl(mask)) | htonl(addr);

	return 0;
}

//=============================================================================

int WINS_Connect(int socket, struct qsockaddr* addr)
{
	return 0;
}

//=============================================================================

int WINS_CheckNewConnections(void)
{
	char buf[4096];

	if (net_acceptsocket == -1)
		return -1;

	if (precvfrom(net_acceptsocket, buf, sizeof(buf), MSG_PEEK, NULL, NULL) > 0)
	{
		return net_acceptsocket;
	}
	return -1;
}

//=============================================================================

int WINS_Read(int socket, byte* buf, int len, struct qsockaddr* addr)
{
	int addrlen = sizeof(struct qsockaddr);
	int ret;

	ret = precvfrom(socket, (char*)buf, len, 0, (struct sockaddr*)addr, &addrlen);
	if (ret == -1)
	{
		int err = pWSAGetLastError();

		if (err == WSAEWOULDBLOCK || err == WSAECONNREFUSED)
			return 0;
	}
	return ret;
}

//=============================================================================

int WINS_MakeSocketBroadcastCapable(int socket)
{
	int i = 1;

	// make this_ socket broadcast capable
	if (psetsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char*)&i, sizeof(i)) < 0)
		return -1;
	net_broadcastsocket = socket;

	return 0;
}

//=============================================================================

int WINS_Broadcast(int socket, byte* buf, int len)
{
	int ret;

	if (socket != net_broadcastsocket)
	{
		if (net_broadcastsocket != 0)
			Sys_Error((char*)"Attempted to use multiple broadcasts sockets\n");
		WINS_GetLocalAddress();
		ret = WINS_MakeSocketBroadcastCapable(socket);
		if (ret == -1)
		{
			Con_Printf((char*)"Unable to make socket broadcast capable\n");
			return ret;
		}
	}

	return WINS_Write(socket, buf, len, &broadcastaddr);
}

//=============================================================================

int WINS_Write(int socket, byte* buf, int len, struct qsockaddr* addr)
{
	int ret;

	ret = psendto(socket, (char*)buf, len, 0, (struct sockaddr*)addr, sizeof(struct qsockaddr));
	if (ret == -1)
		if (pWSAGetLastError() == WSAEWOULDBLOCK)
			return 0;

	return ret;
}

//=============================================================================

char* WINS_AddrToString(struct qsockaddr* addr)
{
	static char buffer[22];
	int haddr;

	haddr = ntohl(((struct sockaddr_in*)addr)->sin_addr.s_addr);
	sprintf(buffer, (char*)"%d.%d.%d.%d:%d", (haddr >> 24) & 0xff, (haddr >> 16) & 0xff, (haddr >> 8) & 0xff, haddr & 0xff,
	        ntohs(((struct sockaddr_in*)addr)->sin_port));
	return buffer;
}

//=============================================================================

int WINS_StringToAddr(char* string, struct qsockaddr* addr)
{
	int ha1, ha2, ha3, ha4, hp;
	int ipaddr;

	sscanf(string, (char*)"%d.%d.%d.%d:%d", &ha1, &ha2, &ha3, &ha4, &hp);
	ipaddr = (ha1 << 24) | (ha2 << 16) | (ha3 << 8) | ha4;

	addr->sa_family = AF_INET;
	((struct sockaddr_in*)addr)->sin_addr.s_addr = htonl(ipaddr);
	((struct sockaddr_in*)addr)->sin_port = htons(hp);
	return 0;
}

//=============================================================================

int WINS_GetSocketAddr(int socket, struct qsockaddr* addr)
{
	int addrlen = sizeof(struct qsockaddr);
	unsigned int a;

	Q_memset(addr, 0, sizeof(struct qsockaddr));
	pgetsockname(socket, (struct sockaddr*)addr, &addrlen);
	a = ((struct sockaddr_in*)addr)->sin_addr.s_addr;
	if (a == 0 || a == inet_addr((char*)"127.0.0.1"))
		((struct sockaddr_in*)addr)->sin_addr.s_addr = myAddr;

	return 0;
}

//=============================================================================

int WINS_GetNameFromAddr(struct qsockaddr* addr, char* name)
{
	struct hostent* hostentry;

	hostentry = pgethostbyaddr((char*)&((struct sockaddr_in*)addr)->sin_addr, sizeof(struct in_addr), AF_INET);
	if (hostentry)
	{
		Q_strncpy(name, hostentry->h_name, NET_NAMELEN - 1);
		return 0;
	}

	Q_strcpy(name, WINS_AddrToString(addr));
	return 0;
}

//=============================================================================

int WINS_GetAddrFromName(char* name, struct qsockaddr* addr)
{
	struct hostent* hostentry;

	if (name[0] >= '0' && name[0] <= '9')
		return PartialIPAddress(name, addr);

	hostentry = pgethostbyname(name);
	if (!hostentry)
		return -1;

	addr->sa_family = AF_INET;
	((struct sockaddr_in*)addr)->sin_port = htons(net_hostport);
	((struct sockaddr_in*)addr)->sin_addr.s_addr = *(int*)hostentry->h_addr_list[0];

	return 0;
}

//=============================================================================

int WINS_AddrCompare(struct qsockaddr* addr1, struct qsockaddr* addr2)
{
	if (addr1->sa_family != addr2->sa_family)
		return -1;

	if (((struct sockaddr_in*)addr1)->sin_addr.s_addr != ((struct sockaddr_in*)addr2)->sin_addr.s_addr)
		return -1;

	if (((struct sockaddr_in*)addr1)->sin_port != ((struct sockaddr_in*)addr2)->sin_port)
		return 1;

	return 0;
}

//=============================================================================

int WINS_GetSocketPort(struct qsockaddr* addr)
{
	return ntohs(((struct sockaddr_in*)addr)->sin_port);
}


int WINS_SetSocketPort(struct qsockaddr* addr, int port)
{
	((struct sockaddr_in*)addr)->sin_port = htons(port);
	return 0;
}

//=============================================================================
