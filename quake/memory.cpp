#include "memory.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <vector>

#include "args.h"
#include "mode.h"

std::vector<char> memory_;

constexpr size_t MINIMUM_WIN_MEMORY = 0x0880000;
constexpr size_t MAXIMUM_WIN_MEMORY = 0x1000000;

constexpr size_t MINIMUM_MEMORY = 0x550000;
constexpr size_t MINIMUM_MEMORY_LEVELPAK = MINIMUM_MEMORY + 0x100000;


void Memory_Init()
{
    MEMORYSTATUS lpBuffer{.dwLength = sizeof(MEMORYSTATUS)};
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

    // тут явно должно бросаться исключение
    //if (!memory_)
    //	Sys_Error("Not enough memory free; check disk space\n");

    // почему размер меняется потом??
    //if (g_pAppApi->Args_GetIndex("-minmemory"))
    //	parms->memsize = Memory_GetMinimum();
    //
    //if (parms->memsize < minimum_memory)
    //	Sys_Error("Only %4.1f megs of memory available, can't execute game", parms->memsize / (float)0x100000);

    // это выводилось при инициализации хоста, после даты
    //Con_Printf("%4.1f megabyte heap\n", parms->memsize / (1024 * 1024.0));
}


char * Memory_GetData()
{
    return memory_.data();
}


size_t Memory_GetSize()
{
    return memory_.size();
}


size_t Memory_GetMinimum()
{
    return Mode_IsStandard() ? MINIMUM_MEMORY : MINIMUM_MEMORY_LEVELPAK;
}