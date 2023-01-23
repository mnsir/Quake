#pragma once

#include "common.h"
#include "net_main.h"


typedef struct
{
	char* name;
	bool initialized;
	int controlSock;
	int (*Init)(void);
	void (*Shutdown)(void);
	void (*Listen)(bool state);
	int (*OpenSocket)(int port);
	int (*CloseSocket)(int socket);
	int (*Connect)(int socket, struct qsockaddr* addr);
	int (*CheckNewConnections)(void);
	int (*Read)(int socket, uint8_t* buf, int len, struct qsockaddr* addr);
	int (*Write)(int socket, uint8_t* buf, int len, struct qsockaddr* addr);
	int (*Broadcast)(int socket, uint8_t* buf, int len);
	char* (*AddrToString)(struct qsockaddr* addr);
	int (*StringToAddr)(char* string, struct qsockaddr* addr);
	int (*GetSocketAddr)(int socket, struct qsockaddr* addr);
	int (*GetNameFromAddr)(struct qsockaddr* addr, char* name);
	int (*GetAddrFromName)(char* name, struct qsockaddr* addr);
	int (*AddrCompare)(struct qsockaddr* addr1, struct qsockaddr* addr2);
	int (*GetSocketPort)(struct qsockaddr* addr);
	int (*SetSocketPort)(struct qsockaddr* addr, int port);
} net_landriver_t;

#define MAX_NET_DRIVERS 8
extern int net_numlandrivers;
extern net_landriver_t net_landrivers[MAX_NET_DRIVERS];

typedef struct
{
	char* name;
	bool initialized;
	int (*Init)(void);
	void (*Listen)(bool state);
	void (*SearchForHosts)(bool xmit);
	qsocket_t* (*Connect)(char* host);
	qsocket_t* (*CheckNewConnections)(void);
	int (*QGetMessage)(qsocket_t* sock);
	int (*QSendMessage)(qsocket_t* sock, sizebuf_t* data);
	int (*SendUnreliableMessage)(qsocket_t* sock, sizebuf_t* data);
	bool(*CanSendMessage)(qsocket_t* sock);
	bool(*CanSendUnreliableMessage)(qsocket_t* sock);
	void (*Close)(qsocket_t* sock);
	void (*Shutdown)(void);
	int controlSock;
} net_driver_t;

extern int net_numdrivers;
extern net_driver_t net_drivers[MAX_NET_DRIVERS];

