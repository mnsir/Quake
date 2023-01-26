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
// mathlib.c -- math primitives

#include "mathlib.h"

#include <cmath>
#include <numbers>



vec3_t vec3_origin = {0, 0, 0};

vec3_t& ToVec3(float* ptr) { return reinterpret_cast<vec3_t&>(*ptr); }

/*-----------------------------------------------------------------*/

double DEG2RAD(double degrees) { return degrees * std::numbers::pi / 180; }

void ProjectPointOnPlane(vec3_t& dst, const vec3_t& p, const vec3_t& normal)
{
	auto inv_denom = 1 / DotProduct(normal, normal);
	
	vec3_t n;
	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	auto d = DotProduct(normal, p) * inv_denom;
	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector(vec3_t& dst, const vec3_t& src)
{
	auto minelem = 1.0;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	int pos = 0;
	for (int i = 0; i < 3; i++)
	{
		if (fabs(src[i]) < minelem)
		{
			pos = i;
			minelem = fabs(src[i]);
		}
	}
	vec3_t tempvec;
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane(dst, tempvec, src);
	
	/*
	** normalize the result
	*/
	VectorNormalize(dst);
}

#pragma optimize( "", off )


void RotatePointAroundVector(vec3_t& dst, const vec3_t& dir, const vec3_t& point, double degrees)
{
	vec3_t vf;
	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	vec3_t vr;
	PerpendicularVector(vr, dir);
	vec3_t vup;
	CrossProduct(vr, vf, vup);

	float m[3][3];
	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	float im[3][3];
	memcpy(im, m, sizeof(im));

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	float zrot[3][3];
	memset(zrot, 0, sizeof(zrot));
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	zrot[0][0] = cos(DEG2RAD(degrees));
	zrot[0][1] = sin(DEG2RAD(degrees));
	zrot[1][0] = -sin(DEG2RAD(degrees));
	zrot[1][1] = cos(DEG2RAD(degrees));

	float tmpmat[3][3];
	R_ConcatRotations(m, zrot, tmpmat);
	float rot[3][3];
	R_ConcatRotations(tmpmat, im, rot);

	for (int i = 0; i < 3; i++)
	{
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}

#pragma optimize( "", on )

/*-----------------------------------------------------------------*/


float anglemod(float a)
{
	a = (360.0 / 65536) * ((int)(a * (65536 / 360.0)) & 65535);
	return a;
}


void AngleVectors(const vec3_t& angles, vec3_t& forward, vec3_t& right, vec3_t& up)
{
	auto angle = DEG2RAD(angles[YAW]);
	auto sy = sin(angle);
	auto cy = cos(angle);
	angle = DEG2RAD(angles[PITCH]);
	auto sp = sin(angle);
	auto cp = cos(angle);
	angle = DEG2RAD(angles[ROLL]);
	auto sr = sin(angle);
	auto cr = cos(angle);

	forward[0] = cp * cy;
	forward[1] = cp * sy;
	forward[2] = -sp;
	right[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
	right[1] = (-1 * sr * sp * sy + -1 * cr * cy);
	right[2] = -1 * sr * cp;
	up[0] = (cr * sp * cy + -sr * -sy);
	up[1] = (cr * sp * sy + -sr * cy);
	up[2] = cr * cp;
}

// Скалярное произведение векторов
double DotProduct(const vec3_t& lhs, const vec3_t& rhs) { return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2]; }

double Length(const vec3_t& self) { return std::sqrt(DotProduct(self, self)); }

bool VectorCompare(const vec3_t& lhs, const vec3_t& rhs)
{
	for (int i = 0; i < 3; i++)
		if (lhs[i] != rhs[i])
			return 0;

	return 1;
}

void VectorMA(const vec3_t& veca, double scale, const vec3_t& vecb, vec3_t& out)
{
	out[0] = veca[0] + scale * vecb[0];
	out[1] = veca[1] + scale * vecb[1];
	out[2] = veca[2] + scale * vecb[2];
}


void VectorSubtract(const vec3_t& lhs, const vec3_t& rhs, vec3_t& out)
{
	out[0] = lhs[0] - rhs[0];
	out[1] = lhs[1] - rhs[1];
	out[2] = lhs[2] - rhs[2];
}

void VectorAdd(const vec3_t& lhs, const vec3_t& rhs, vec3_t& out)
{
	out[0] = lhs[0] + rhs[0];
	out[1] = lhs[1] + rhs[1];
	out[2] = lhs[2] + rhs[2];
}

void VectorCopy(const vec3_t& self, vec3_t& out)
{
	out[0] = self[0];
	out[1] = self[1];
	out[2] = self[2];
}

// Векторное произведение векторов
void CrossProduct(const vec3_t& lhs, const vec3_t& rhs, vec3_t& out)
{
	out[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
	out[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
	out[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
}


double VectorNormalize(vec3_t& self)
{
	auto length = Length(self);

	if (length)
	{
		auto ilength = 1 / length;
		self[0] *= ilength;
		self[1] *= ilength;
		self[2] *= ilength;
	}

	return length;
}

void VectorInverse(vec3_t& self)
{
	self[0] = -self[0];
	self[1] = -self[1];
	self[2] = -self[2];
}

void VectorScale(const vec3_t& in, double scale, vec3_t& out)
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}



/*
================
R_ConcatRotations
================
*/
void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +	in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +	in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +	in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +	in1[2][2] * in2[2][2];
}


/*
================
R_ConcatTransforms
================
*/
void R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +	in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +	in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +	in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] +	in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +	in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +	in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +	in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] +	in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +	in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +	in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +	in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] +	in1[2][2] * in2[2][3] + in1[2][3];
}

