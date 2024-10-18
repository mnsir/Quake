
// chase.c -- chase camera code

#include "quakedef.h"

cvar_t chase_back = {(char*)"chase_back", (char*)"100"};
cvar_t chase_up = {(char*)"chase_up", (char*)"16"};
cvar_t chase_right = {(char*)"chase_right", (char*)"0"};
cvar_t chase_active = {(char*)"chase_active", (char*)"0"};

vec3_t chase_pos;
vec3_t chase_angles;

vec3_t chase_dest;
vec3_t chase_dest_angles;


void Chase_Init()
{
    Cvar_RegisterVariable(&chase_back);
    Cvar_RegisterVariable(&chase_up);
    Cvar_RegisterVariable(&chase_right);
    Cvar_RegisterVariable(&chase_active);
}

void Chase_Reset()
{
    // for respawning and teleporting
   // start position 12 units behind head
}

void TraceLine(vec3_t start, vec3_t end, vec3_t impact)
{
    trace_t trace;

    memset(&trace, 0, sizeof(trace));
    SV_RecursiveHullCheck(cl.worldmodel->hulls, 0, 0, 1, start, end, &trace);

    VectorCopy(trace.endpos, impact);
}

void Chase_Update()
{
    int i;
    float dist;
    vec3_t forward, up, right;
    vec3_t dest, stop;


    // if can't see player, reset
    AngleVectors(cl.viewangles, forward, right, up);

    // calc exact destination
    for (i = 0; i < 3; i++)
        chase_dest[i] = r_refdef.vieworg[i]
        - forward[i] * chase_back.value
        - right[i] * chase_right.value;
    chase_dest[2] = r_refdef.vieworg[2] + chase_up.value;

    // find the spot the player is looking at
    VectorMA(r_refdef.vieworg, 4096, forward, dest);
    TraceLine(r_refdef.vieworg, dest, stop);

    // calculate pitch to look at the same spot from camera
    VectorSubtract(stop, r_refdef.vieworg, stop);
    dist = DotProduct(stop, forward);
    if (dist < 1)
        dist = 1;
    r_refdef.viewangles[PITCH] = -atan(stop[2] / dist) / M_PI * 180;

    // move towards destination
    VectorCopy(chase_dest, r_refdef.vieworg);
}

