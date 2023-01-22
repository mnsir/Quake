#pragma once

#include "common.h"
#include <dsound.h>
#include <windows.h>

// initializes cycling through a DMA buffer and returns information on it
bool SNDDMA_Init(void);
// gets the current DMA position
int SNDDMA_GetDMAPos(void);
// shutdown the DMA xfer.
void SNDDMA_Shutdown(void);

void SNDDMA_Submit(void);

void S_BlockSound(void);
void S_UnblockSound(void);

extern LPDIRECTSOUND pDS;

extern LPDIRECTSOUNDBUFFER pDSBuf;


extern DWORD gSndBufSize;