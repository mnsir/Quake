/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// mathlib.h
#pragma once

typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec5_t[5];

typedef int fixed4_t;
typedef int fixed8_t;
typedef int fixed16_t;

#ifndef M_PI
#define M_PI 3.14159265358979323846 // matches value in gcc v2 math.h
#endif

struct mplane_s;

extern vec3_t MATHLIB_PUB_vec3_origin;

#define MATHLIB_PUB_DotProduct(x,y) (x[0]*y[0]+x[1]*y[1]+x[2]*y[2])
#define MATHLIB_PUB_VectorSubtract(a,b,c) {c[0]=a[0]-b[0];c[1]=a[1]-b[1];c[2]=a[2]-b[2];}
#define MATHLIB_PUB_VectorAdd(a,b,c) {c[0]=a[0]+b[0];c[1]=a[1]+b[1];c[2]=a[2]+b[2];}
#define MATHLIB_PUB_VectorCopy(a,b) {b[0]=a[0];b[1]=a[1];b[2]=a[2];}

#define MATHLIB_PUB_BOX_ON_PLANE_SIDE(emins, emaxs, p) \
 (((p)->type < 3)? \
 ( \
 ((p)->dist <= (emins)[(p)->type])? \
 1 \
 : \
 ( \
 ((p)->dist >= (emaxs)[(p)->type])?\
 2 \
 : \
 3 \
 ) \
 ) \
 : \
 MATHLIB_PUB_BoxOnPlaneSide( (emins), (emaxs), (p)))

int MATHLIB_PUB_IS_NAN(vec_t x);
void MATHLIB_PUB_RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees);
float MATHLIB_PUB_anglemod(float a);
int MATHLIB_PUB_BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct mplane_s* plane);
void MATHLIB_PUB_AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
int MATHLIB_PUB_VectorCompare(vec3_t v1, vec3_t v2);
void MATHLIB_PUB_VectorMA(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc);

vec_t _MATHLIB_PUB_DotProduct(vec3_t v1, vec3_t v2);
void _MATHLIB_PUB_VectorSubtract(vec3_t veca, vec3_t vecb, vec3_t out);
void _MATHLIB_PUB_VectorAdd(vec3_t veca, vec3_t vecb, vec3_t out);
void _MATHLIB_PUB_VectorCopy(vec3_t in, vec3_t out);

void MATHLIB_PUB_CrossProduct(vec3_t v1, vec3_t v2, vec3_t cross);
vec_t MATHLIB_PUB_VectorLength(vec3_t v);
float MATHLIB_PUB_VectorNormalize(vec3_t v); // returns vector length
void MATHLIB_PUB_VectorInverse(vec3_t v);
void MATHLIB_PUB_VectorScale(vec3_t in, vec_t scale, vec3_t out);

void MATHLIB_PUB_R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3]);
void MATHLIB_PUB_R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4]);

void MATHLIB_PUB_FloorDivMod(double numer, double denom, int* quotient,
	int* rem);
int MATHLIB_PUB_GreatestCommonDivisor(int i1, int i2);


