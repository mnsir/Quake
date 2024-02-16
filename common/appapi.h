#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


typedef struct
{
    HINSTANCE(*GetAppInstance)();
    const char * (*GetAppBaseDir)();
    const char * (*GetAppCacheDir)();
    int (*GetArgc)();
    char ** (*GetArgv)();
} AppAPI;


extern AppAPI * g_pAppApi; // ��� ������������� � dll