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
// net_wins.h
#pragma once

#include "common.h"


int WINS_Init();
void WINS_Shutdown();
void WINS_Listen(bool state);
int WINS_OpenSocket(int port);
int WINS_CloseSocket(int socket);
int WINS_Connect(int socket, struct qsockaddr* addr);
int WINS_CheckNewConnections();
int WINS_Read(int socket, uint8_t* buf, int len, struct qsockaddr* addr);
int WINS_Write(int socket, uint8_t* buf, int len, struct qsockaddr* addr);
int WINS_Broadcast(int socket, uint8_t* buf, int len);
char* WINS_AddrToString(struct qsockaddr* addr);
int WINS_StringToAddr(char* string, struct qsockaddr* addr);
int WINS_GetSocketAddr(int socket, struct qsockaddr* addr);
int WINS_GetNameFromAddr(struct qsockaddr* addr, char* name);
int WINS_GetAddrFromName(char* name, struct qsockaddr* addr);
int WINS_AddrCompare(struct qsockaddr* addr1, struct qsockaddr* addr2);
int WINS_GetSocketPort(struct qsockaddr* addr);
int WINS_SetSocketPort(struct qsockaddr* addr, int port);


extern bool winsock_lib_initialized;