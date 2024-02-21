#pragma once


#ifdef __cplusplus
extern "C" {
#endif

	void Memory_Init();
	char* Memory_GetData();
	size_t Memory_GetSize();
	size_t Memory_GetMinimum();

#ifdef __cplusplus
}
#endif
