#pragma once

#include "cvar.h"


//
// chase
//
extern cvar_t chase_active;

void Chase_Init();
void Chase_Reset();
void Chase_Update();