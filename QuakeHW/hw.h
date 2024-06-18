#pragma once

#include <dll_func_typedefs.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


__declspec(dllexport) void CALLBACK Setup();
__declspec(dllexport) double CALLBACK Loop(double old_time);

__declspec(dllexport) void CALLBACK Setup_Dedicated();
__declspec(dllexport) double CALLBACK Loop_Dedicated(double old_time);
