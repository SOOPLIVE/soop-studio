#pragma once


#include <graphics/vec2.h>
#include <graphics/vec3.h>


#define DEF_TOL_EPSILON 0.01f


static vec3 GetTransformedPos(float x, float y, const matrix4& mat)
{
	vec3 result;
	vec3_set(&result, x, y, 0.0f);
	vec3_transform(&result, &result, &mat);
	return result;
}

static void RotatePos(vec2* pos, float rot)
{
	float cosR = cos(rot);
	float sinR = sin(rot);

	vec2 newPos;

	newPos.x = cosR * pos->x - sinR * pos->y;
	newPos.y = sinR * pos->x + cosR * pos->y;

	vec2_copy(pos, &newPos);
}

static inline bool IsNearlyZero(float value, float Tolerance = 1.e-4f)		//앱실론 데이터 0근처
{
	return fabs(value) < Tolerance;
};