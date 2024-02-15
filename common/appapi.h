#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


typedef struct
{
    HINSTANCE(*GetAppInstance)();
    const char * (*GetAppBaseDir)();
    const char * (*GetAppCacheDir)();
} AppAPI;


extern AppAPI * g_pAppApi; // для использования в dll