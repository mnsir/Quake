#pragma once


#ifdef __cplusplus
extern "C" {
#endif

	void Memory_Init();
	char* Memory_GetData();
	size_t Memory_GetSize();

#ifdef __cplusplus
}
#endif
