#pragma once


typedef struct
{
    int (*Q_log2)(int val);


} AppAPI;


extern AppAPI * g_pAppApi; // для использования в dll