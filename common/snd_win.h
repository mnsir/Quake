#pragma once

#include "common.h"
#include <dsound.h>
#include <windows.h>

// initializes cycling through a DMA buffer and returns information on it
bool SNDDMA_Init();
// gets the current DMA position
int SNDDMA_GetDMAPos();
// shutdown the DMA xfer.
void SNDDMA_Shutdown();

void SNDDMA_Submit();

void S_BlockSound();
void S_UnblockSound();

extern LPDIRECTSOUND pDS;

extern LPDIRECTSOUNDBUFFER pDSBuf;


extern DWORD gSndBufSize;