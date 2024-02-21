#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


typedef struct
{
    HINSTANCE(*GetAppInstance)();
    const char * (*GetAppBaseDir)();
    const char * (*GetAppCacheDir)();

    int (*Args_GetIndex)(const char *);
    const char* (*Args_GetByIndex)(int);
    int (*Args_GetCount)();
    void (*Args_Reset)(const char**, int);

    int (*Mode_IsStandard)();
    int (*Mode_IsRogue)();
    int (*Mode_IsHipnotic)();

    char* (*Memory_GetData)();
    size_t (*Memory_GetSize)();
    size_t (*Memory_GetMinimum)();

} AppAPI;


extern AppAPI * g_pAppApi; // для использования в dll