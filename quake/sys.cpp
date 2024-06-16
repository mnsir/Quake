#include "sys.h"

#include <chrono>


/*
================
Sys_FloatTime
================
*/
double Sys_FloatTime()
{
	using Duration = std::chrono::duration<double>;
	const auto now = std::chrono::high_resolution_clock::now();
	return std::chrono::time_point_cast<Duration>(now).time_since_epoch().count();
}
