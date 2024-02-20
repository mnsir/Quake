#include "memory.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <vector>

#include "args.h"

std::vector<char> memory_;

constexpr size_t MINIMUM_WIN_MEMORY = 0x0880000;
constexpr size_t MAXIMUM_WIN_MEMORY = 0x1000000;


void Memory_Init()
{
	MEMORYSTATUS lpBuffer{ .dwLength = sizeof(MEMORYSTATUS) };
	GlobalMemoryStatus(&lpBuffer);
	const size_t available = lpBuffer.dwAvailPhys;
	const size_t halfTotal = lpBuffer.dwTotalPhys >> 1;

	// take the greater of all the available memory or half the total memory,
	// but at least 8 Mb and no more than 16 Mb, unless they explicitly
	// request otherwise
	auto memsize = available;

	if (memsize < MINIMUM_WIN_MEMORY)
		memsize = MINIMUM_WIN_MEMORY;

	if (memsize < halfTotal)
		memsize = halfTotal;

	if (memsize > MAXIMUM_WIN_MEMORY)
		memsize = MAXIMUM_WIN_MEMORY;

	if (auto heapsize = Args_GetInt("-heapsize"))
		memsize = *heapsize * 1024;

	memory_ = std::vector<char>(memsize, '\0');

	//if (!memory_)
	//	Sys_Error("Not enough memory free; check disk space\n");

	//if (g_pAppApi->Mode_IsStandard())
	//	minimum_memory = MINIMUM_MEMORY;
	//else
	//	minimum_memory = MINIMUM_MEMORY_LEVELPAK;
	//
	//if (g_pAppApi->Args_GetIndex("-minmemory"))
	//	parms->memsize = minimum_memory;
	//
	//if (parms->memsize < minimum_memory)
	//	Sys_Error("Only %4.1f megs of memory available, can't execute game", parms->memsize / (float)0x100000);


	//Con_Printf("%4.1f megabyte heap\n", parms->memsize / (1024 * 1024.0));
}


char* Memory_GetData()
{
	return memory_.data();
}

size_t Memory_GetSize()
{
	return memory_.size();
}