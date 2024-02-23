#pragma once

#include <dll_func_typedefs.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


__declspec(dllexport) int CALLBACK FromLib(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPCSTR lpCmdLine, int nCmdShow);
