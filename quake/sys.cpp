#include "sys.h"

#include <windows.h>
#include "args.h"


static double pfreq;
static double curtime = 0.0;
static double lastcurtime = 0.0;
static int lowshift;

/*
================
Sys_FloatTime
================
*/
double Sys_FloatTime()
{
    static int sametimecount;
    static unsigned int oldtime;
    static int first = 1;
    LARGE_INTEGER PerformanceCount;
    unsigned int temp, t2;
    double time;

    QueryPerformanceCounter(&PerformanceCount);

    temp = ((unsigned int)PerformanceCount.LowPart >> lowshift) |
        ((unsigned int)PerformanceCount.HighPart << (32 - lowshift));

    if (first)
    {
        oldtime = temp;
        first = 0;
    }
    else
    {
        // check for turnover or backward time
        if ((temp <= oldtime) && ((oldtime - temp) < 0x10000000))
        {
            oldtime = temp; // so we can't get stuck
        }
        else
        {
            t2 = temp - oldtime;

            time = (double)t2 * pfreq;
            oldtime = temp;

            curtime += time;

            if (curtime == lastcurtime)
            {
                sametimecount++;

                if (sametimecount > 100000)
                {
                    curtime += 1.0;
                    sametimecount = 0;
                }
            }
            else
            {
                sametimecount = 0;
            }

            lastcurtime = curtime;
        }
    }

    return curtime;
}


/*
================
Sys_InitFloatTime
================
*/
void Sys_InitFloatTime()
{
    Sys_FloatTime();

    curtime = Args_GetDouble("-starttime").value_or(0.0);

    lastcurtime = curtime;
}


/*
================
Sys_Init
================
*/
void Sys_Init()
{
    LARGE_INTEGER PerformanceFreq;
    unsigned int lowpart, highpart;

    if (!QueryPerformanceFrequency(&PerformanceFreq))
    {
        //TODO Sys_Error("No hardware timer available");
    }

    // get 32 out of the 64 time bits such that we have around
    // 1 microsecond resolution
    lowpart = (unsigned int)PerformanceFreq.LowPart;
    highpart = (unsigned int)PerformanceFreq.HighPart;
    lowshift = 0;

    while (highpart || (lowpart > 2000000.0))
    {
        lowshift++;
        lowpart >>= 1;
        lowpart |= (highpart & 1) << 31;
        highpart >>= 1;
    }

    pfreq = 1.0 / (double)lowpart;

    Sys_InitFloatTime();
}

