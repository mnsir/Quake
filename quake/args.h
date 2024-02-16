#pragma once



#ifdef __cplusplus
extern "C" {
#endif

	void InitCommandLine(const char* str);
	int GetArgc();
	char** GetArgv();

#ifdef __cplusplus
}
#endif