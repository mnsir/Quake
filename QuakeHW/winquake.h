
// winquake.h: Win32-specific Quake header file

#include <windows.h>
#define WM_MOUSEWHEEL 0x020A

#include <ddraw.h>
#include <dsound.h>

extern HINSTANCE global_hInstance;
extern int global_nCmdShow;

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

typedef enum { MS_WINDOWED, MS_FULLSCREEN, MS_FULLDIB, MS_UNINIT } modestate_t;

extern modestate_t modestate;

extern HWND mainwindow;
extern bool ActiveApp, Minimized;

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

extern int (PASCAL FAR* pWSAStartup)(WORD wVersionRequired, LPWSADATA lpWSAData);
extern int (PASCAL FAR* pWSACleanup)();
extern int (PASCAL FAR* pWSAGetLastError)();
extern SOCKET(PASCAL FAR* psocket)(int af, int type, int protocol);
extern int (PASCAL FAR* pioctlsocket)(SOCKET s, long cmd, u_long FAR* argp);
extern int (PASCAL FAR* psetsockopt)(SOCKET s, int level, int optname, const char FAR* optval, int optlen);
extern int (PASCAL FAR* precvfrom)(SOCKET s, char FAR* buf, int len, int flags, struct sockaddr FAR* from, int FAR* fromlen);
extern int (PASCAL FAR* psendto)(SOCKET s, const char FAR* buf, int len, int flags, const struct sockaddr FAR* to, int tolen);
extern int (PASCAL FAR* pclosesocket)(SOCKET s);
extern int (PASCAL FAR* pgethostname)(char FAR* name, int namelen);
extern struct hostent FAR* (PASCAL FAR* pgethostbyname)(const char FAR* name);
extern struct hostent FAR* (PASCAL FAR* pgethostbyaddr)(const char FAR* addr, int len, int type);
extern int (PASCAL FAR* pgetsockname)(SOCKET s, struct sockaddr FAR* name, int FAR* namelen);
