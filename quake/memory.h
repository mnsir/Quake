#pragma once


#ifdef __cplusplus
extern "C" {
#endif

	void Memory_Init();
	char* Memory_GetData();
	unsigned Memory_GetSize();
	unsigned Memory_GetMinimum();

#ifdef __cplusplus
}
#endif
