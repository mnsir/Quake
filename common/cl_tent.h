#pragma once

#include "mathlib.h"
#include "render.h"

#ifdef GLQUAKE
#include "gl_model.h"
#else
#include "model.h"
#endif


typedef struct
{
	int entity;
	struct model_s* model;
	float endtime;
	vec3_t start, end;
} beam_t;

// cl_tent.h
#define MAX_TEMP_ENTITIES 64 // lightning bolts, etc
extern entity_t cl_temp_entities[MAX_TEMP_ENTITIES];
#define MAX_BEAMS 24
extern beam_t cl_beams[MAX_BEAMS];

void CL_ParseTEnt(void);
void CL_UpdateTEnts(void);
void CL_InitTEnts(void);
