#pragma once


#ifdef __cplusplus

#include <optional>
#include <string_view>

std::optional<int> Args_GetInt(std::string_view arg);

#endif


#ifdef __cplusplus
extern "C" {
#endif

	void InitCommandLine(const char* str);
	void Args_Reset(const char** ptr, int size);
	int Args_GetIndex(const char* str);
	const char* Args_GetByIndex(int index);
	int Args_GetCount();

	// bool

	int Args_NoAudio();
	int Args_Rogue();
	int Args_Hipnotic();

	// int
	

	// string

	const char * Args_BaseDir();
	const char * Args_CacheDir();


#ifdef __cplusplus
}
#endif
