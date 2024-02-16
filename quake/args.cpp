#include "args.h"

#define _CRT_SECURE_NO_WARNINGS

#include <cstddef>
#include <cstring>

int argc = 0;

#define MAX_NUM_ARGVS 50
char* argv[MAX_NUM_ARGVS];
char empty_string[] = "";

void InitCommandLine(const char* str)
{ 
    argc = 1;
    argv[0] = empty_string;

    static char arr[128];
    char* lpCmdLine = strcpy(arr, str);

    while (*lpCmdLine && (argc < MAX_NUM_ARGVS))
    {
        while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
            lpCmdLine++;

        if (*lpCmdLine)
        {
            argv[argc] = lpCmdLine;
            argc++;

            while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
                lpCmdLine++;

            if (*lpCmdLine)
            {
                *lpCmdLine = 0;
                lpCmdLine++;
            }

        }
    }

}


char** GetArgv() { return argv; }
int GetArgc() { return argc; }