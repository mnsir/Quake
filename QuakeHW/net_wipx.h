
// net_wipx.h

int WIPX_Init();
void WIPX_Shutdown();
void WIPX_Listen(bool state);
int WIPX_OpenSocket(int port);
int WIPX_CloseSocket(int socket);
int WIPX_Connect(int socket, struct qsockaddr * addr);
int WIPX_CheckNewConnections();
int WIPX_Read(int socket, byte * buf, int len, struct qsockaddr * addr);
int WIPX_Write(int socket, byte * buf, int len, struct qsockaddr * addr);
int WIPX_Broadcast(int socket, byte * buf, int len);
char * WIPX_AddrToString(struct qsockaddr * addr);
int WIPX_StringToAddr(const char * string, struct qsockaddr * addr);
int WIPX_GetSocketAddr(int socket, struct qsockaddr * addr);
int WIPX_GetNameFromAddr(struct qsockaddr * addr, char * name);
int WIPX_GetAddrFromName(const char * name, struct qsockaddr * addr);
int WIPX_AddrCompare(struct qsockaddr * addr1, struct qsockaddr * addr2);
int WIPX_GetSocketPort(struct qsockaddr * addr);
int WIPX_SetSocketPort(struct qsockaddr * addr, int port);
