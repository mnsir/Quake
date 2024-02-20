#include <dll_func_typedefs.h>
#include "args.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <appapi.h>
#include <assert.h>

#define LIBNAME_HW "QuakeHW"
#define LIBNAME_GL "QuakeGL"

#pragma comment(lib, LIBNAME_HW ".lib")
#pragma comment(lib, LIBNAME_GL ".lib")


typedef enum
{
	hw = 0,
	gl,
} VideoMode;


const char* const aLibFileName[] = {
	LIBNAME_HW ".dll",
	LIBNAME_GL ".dll",
};


const char* const aArgs[] = {
	 "-startwindowed",
	 //"-playback",
	 //"-startwindowed -record",
	 "-width 800 -height 600 -bpp 32 -window -no8bit"
};


HMODULE hModule = NULL;
FromLibFunc FromLib = NULL;
InitializeFunc Initialize = NULL;

HINSTANCE g_hInstance;


HINSTANCE GetAppInstance()
{
	assert(g_hInstance);
	return g_hInstance;
}


const char* GetAppBaseDir()
{
	static char cwd[1024] = { 0 };
	if (!cwd[0] && !GetCurrentDirectory(sizeof(cwd), cwd))
	{
		//Sys_Error("Couldn't determine current directory");
	}

	//if (cwd[Q_strlen(cwd) - 1] == '/')
	//    cwd[Q_strlen(cwd) - 1] = 0;

	return cwd;
}


const char* GetAppCacheDir()
{
	// for development over ISDN lines
	return NULL;
}


AppAPI g_appApi = {
	.GetAppInstance = GetAppInstance,
	.GetAppBaseDir = GetAppBaseDir,
	.GetAppCacheDir = GetAppCacheDir,
	.Args_GetIndex = Args_GetIndex,
	.Args_GetByIndex = Args_GetByIndex,
	.Args_GetCount = Args_GetCount,
	.Args_Reset = Args_Reset,
};

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	BOOL res = TRUE;

	/* previous instances do not exist in Win32 */
	if (hPrevInstance)
		return 0;

	g_hInstance = hInstance;

	const VideoMode mode = hw;
	InitCommandLine(aArgs[mode] /*lpCmdLine*/);

	hModule = LoadLibrary(aLibFileName[mode]);

	if (hModule)
	{
		FromLib = GetProcAddress(hModule, "_FromLib@0");
		Initialize = GetProcAddress(hModule, "_Initialize@4");
	}

	if (Initialize)
	{
		Initialize(&g_appApi);

		if (FromLib)
			res = FromLib();
		else
			MessageBox(NULL, "FromLib not loaded", "FromLib not loaded!", 0);
	}
	else
		MessageBox(NULL, "Initialize not loaded", "Initialize not loaded!", 0);


	FromLib = NULL;
	Initialize = NULL;
	if (hModule)
	{
		FreeLibrary(hModule);
		hModule = NULL;
	}

	return res;
}

