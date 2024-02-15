#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <appapi.h>


typedef int (CALLBACK * FromLibFunc)(LPCSTR lpCmdLine, int nCmdShow);
typedef void (CALLBACK * InitializeFunc)(AppAPI * pAppApi);
