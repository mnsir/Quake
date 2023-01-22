#pragma once

#include "cvar.h"

struct qsockaddr
{
	short sa_family;
	unsigned char sa_data[14];
};

#define NET_NAMELEN 64

#define NET_MAXMESSAGE 8192

typedef struct qsocket_s
{
	struct qsocket_s* next;
	double connecttime;
	double lastMessageTime;
	double lastSendTime;

	bool disconnected;
	bool canSend;
	bool sendNext;

	int driver;
	int landriver;
	int socket;
	void* driverdata;

	unsigned int ackSequence;
	unsigned int sendSequence;
	unsigned int unreliableSendSequence;
	int sendMessageLength;
	byte sendMessage[NET_MAXMESSAGE];

	unsigned int receiveSequence;
	unsigned int unreliableReceiveSequence;
	int receiveMessageLength;
	byte receiveMessage[NET_MAXMESSAGE];

	struct qsockaddr addr;
	char address[NET_NAMELEN];
} qsocket_t;

extern qsocket_t* net_activeSockets;
extern qsocket_t* net_freeSockets;
extern int net_numsockets;
extern int DEFAULTnet_hostport;
extern int net_hostport;

extern int net_driverlevel;
extern cvar_t hostname;

extern int messagesSent;
extern int messagesReceived;
extern int unreliableMessagesSent;
extern int unreliableMessagesReceived;


typedef struct
{
	char name[16];
	char map[16];
	char cname[32];
	int users;
	int maxusers;
	int driver;
	int ldriver;
	struct qsockaddr addr;
} hostcache_t;

extern int hostCacheCount;
#define HOSTCACHESIZE 8
extern hostcache_t hostcache[HOSTCACHESIZE];

extern double net_time;
extern sizebuf_t net_message;
extern int net_activeconnections;

extern bool serialAvailable;
extern bool ipxAvailable;
extern bool tcpipAvailable;
extern char my_ipx_address[NET_NAMELEN];
extern char my_tcpip_address[NET_NAMELEN];
extern void (*GetComPortConfig)(int portNumber, int* port, int* irq, int* baud, bool* useModem);
extern void (*SetComPortConfig)(int portNumber, int port, int irq, int baud, bool useModem);
extern void (*GetModemConfig)(int portNumber, char* dialType, char* clear, char* init, char* hangup);
extern void (*SetModemConfig)(int portNumber, char* dialType, char* clear, char* init, char* hangup);

extern bool slistInProgress;
extern bool slistSilent;
extern bool slistLocal;

qsocket_t* NET_NewQSocket(void);
void NET_FreeQSocket(qsocket_t*);
double SetNetTime(void);

void NET_Init(void);
void NET_Shutdown(void);

struct qsocket_s* NET_CheckNewConnections(void);
// returns a new_ connection number if there is one pending, else -1

struct qsocket_s* NET_Connect(char* host);
// called by client to connect to a host.  Returns -1 if not able to

bool NET_CanSendMessage(qsocket_t* sock);
// Returns true or false if the given qsocket can currently accept a
// message to be transmitted.


int NET_GetMessage(struct qsocket_s* sock);
// returns data in net_message sizebuf
// returns 0 if no data is waiting
// returns 1 if a message was received
// returns 2 if an unreliable message was received
// returns -1 if the connection died

int NET_SendMessage(struct qsocket_s* sock, sizebuf_t* data);
int NET_SendUnreliableMessage(struct qsocket_s* sock, sizebuf_t* data);
// returns 0 if the message connot be delivered reliably, but the connection
// is still considered valid
// returns 1 if the message was sent properly
// returns -1 if the connection died

int NET_SendToAll(sizebuf_t* data, int blocktime);
// This is a reliable *blocking* send to all attached clients.


void NET_Close(struct qsocket_s* sock);
// if a dead connection is returned by a get or send function, this_ function
// should be called when it is convenient

// Server calls when a client is kicked off for a game related misbehavior
// like an illegal protocal conversation.  Client calls when disconnecting
// from a server.
// A netcon_t number will not be reused until this_ function is called for it

void NET_Poll(void);


typedef struct _PollProcedure
{
	struct _PollProcedure* next;
	double nextTime;
	void (*procedure)();
	void* arg;
} PollProcedure;

void SchedulePollProcedure(PollProcedure* pp, double timeOffset);

void NET_Slist_f(void);

