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

static inline bool IsNearlyZero(float value, float Tolerance = 1.e-4f)
{
	return fabs(value) < Tolerance;
};

static inline void GetScaleAndCenterPos(int baseCX, int baseCY, int windowCX,
                                        int windowCY, int &x, int &y,
                                        float &scale)
{
    double windowAspect, baseAspect;
    int newCX, newCY;

    windowAspect = double(windowCX) / double(windowCY);
    baseAspect = double(baseCX) / double(baseCY);

    if (windowAspect > baseAspect) {
        scale = float(windowCY) / float(baseCY);
        newCX = int(double(windowCY) * baseAspect);
        newCY = windowCY;
    } else {
        scale = float(windowCX) / float(baseCX);
        newCX = windowCX;
        newCY = int(float(windowCX) / baseAspect);
    }

    x = windowCX / 2 - newCX / 2;
    y = windowCY / 2 - newCY / 2;
};

static inline void GetCenterPosFromFixedScale(int baseCX, int baseCY,
                                              int windowCX, int windowCY,
                                              int& x, int& y, float scale)
{
    x = (float(windowCX) - float(baseCX) * scale) / 2.0f;
    y = (float(windowCY) - float(baseCY) * scale) / 2.0f;
};

