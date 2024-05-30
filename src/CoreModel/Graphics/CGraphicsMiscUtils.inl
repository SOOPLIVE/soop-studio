#pragma once

#include <obs.hpp>

static inline void StartGraphicsViewRegion(int vX, int vY, int vCX, int vCY, float oL,
                                           float oR, float oT, float oB)
{
    gs_projection_push();
    gs_viewport_push();
    gs_set_viewport(vX, vY, vCX, vCY);
    gs_ortho(oL, oR, oT, oB, -100.0f, 100.0f);
}

static inline void EndGraphicsViewRegion()
{
    gs_viewport_pop();
    gs_projection_pop();
}
