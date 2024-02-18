#pragma once



#ifdef __cplusplus
extern "C" {
#endif

	void InitCommandLine(const char* str);
	void Args_Reset(const char** ptr, int size);
	int Args_GetIndex(const char* str);
	const char* Args_GetByIndex(int index);
	int Args_GetCount();

#ifdef __cplusplus
}
#endif