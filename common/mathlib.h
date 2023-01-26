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
along with this_ program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// mathlib.h
#pragma once
#include <array>


constexpr size_t PITCH = 0; // up / down
constexpr size_t YAW = 1; // left / right
constexpr size_t ROLL = 2; // fall over


using vec_t = float;
using vec3_t = std::array<vec_t, 3>;
using  vec5_t = std::array<vec_t, 5>;

using fixed4_t = int;
using fixed8_t = int;
using fixed16_t = int;

extern vec3_t vec3_origin;

double DEG2RAD(double degrees);

bool VectorCompare(const vec3_t& lhs, const vec3_t& rhs);

void VectorCopy(const vec3_t& self, vec3_t* out);

double Length(const vec3_t& self);
void VectorAdd(const vec3_t& lhs, const vec3_t& rhs, vec3_t* out);
void VectorSubtract(const vec3_t& lhs, const vec3_t& rhs, vec3_t* out);
void VectorScale(const vec3_t& in, double scale, vec3_t* out);
double DotProduct(const vec3_t& lhs, const vec3_t& rhs);
void CrossProduct(const vec3_t& lhs, const vec3_t& rhs, vec3_t* out);
void VectorMA(const vec3_t& veca, double scale, const vec3_t& vecb, vec3_t* out);

double VectorNormalize(vec3_t* self); // returns vector length
void VectorInverse(vec3_t* self);

void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3]);
void R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4]);

void AngleVectors(const vec3_t& angles, vec3_t* forward, vec3_t* right, vec3_t* up);
float anglemod(float a);



void RotatePointAroundVector(vec3_t* dst, const vec3_t& dir, const vec3_t& point, double degrees);