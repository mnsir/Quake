#pragma once

#include <dll_func_typedefs.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


int __declspec(dllexport) CALLBACK FromLib(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPCSTR lpCmdLine, int nCmdShow);

