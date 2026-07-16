#include "CGraphicsContext.h"

#include "Application/CApplication.h"

#include "display-helpers.hpp"

#define DL_OPENGL "libobs-opengl.dylib"
#define DL_METAL ""
#define DL_D3D11 "libobs-d3d11.dll"
//
#define OUTLINE_COLOR 0xFFD0D0D0
#define LINE_LENGTH 0.1f

// Rec. ITU-R BT.1848-1 / EBU R 95
#define ACTION_SAFE_PERCENT 0.035f       // 3.5%
#define GRAPHICS_SAFE_PERCENT 0.05f      // 5.0%
#define FOURBYTHREE_SAFE_PERCENT 0.1625f // 16.25%

void AFGraphicsContext::InitContext()
{

	_InitPrimitives();

	m_inited = true;
}

bool AFGraphicsContext::FinContext()
{
	bool res = false;

	obs_enter_graphics();
	gs_vertexbuffer_destroy(m_pBox);
	gs_vertexbuffer_destroy(m_pBoxLeft);
	gs_vertexbuffer_destroy(m_pBoxTop);
	gs_vertexbuffer_destroy(m_pBoxRight);
	gs_vertexbuffer_destroy(m_pBoxBottom);
	gs_vertexbuffer_destroy(m_pCircle);
	gs_vertexbuffer_destroy(m_pActionSafeMargin);
	gs_vertexbuffer_destroy(m_pGraphicsSafeMargin);
	gs_vertexbuffer_destroy(m_pFourByThreeSafeMargin);
	gs_vertexbuffer_destroy(m_pLeftLine);
	gs_vertexbuffer_destroy(m_pTopLine);
	gs_vertexbuffer_destroy(m_pRightLine);
	obs_leave_graphics();

	res = true;

	if (res)
		m_inited = false;

	return res;
}

const char* AFGraphicsContext::GetRenderModule() const
{
	const char* renderer = config_get_string(APPCONFIG, "Video", "Renderer");
	return (astrcmpi(renderer, "Direct3D 11") == 0) ? DL_D3D11 : DL_OPENGL;
}

void AFGraphicsContext::_InitPrimitives()
{
	obs_enter_graphics();


	gs_render_start(true);
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(0.0f, 1.0f);
	gs_vertex2f(1.0f, 0.0f);
	gs_vertex2f(1.0f, 1.0f);
    m_pBox = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(0.0f, 1.0f);
    m_pBoxLeft = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(1.0f, 0.0f);
    m_pBoxTop = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(1.0f, 0.0f);
	gs_vertex2f(1.0f, 1.0f);
    m_pBoxRight = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 1.0f);
	gs_vertex2f(1.0f, 1.0f);
    m_pBoxBottom = gs_render_save();

	gs_render_start(true);
	for (int i = 0; i <= 360; i += (360 / 20)) {
		float pos = RAD(float(i));
		gs_vertex2f(cosf(pos), sinf(pos));
	}
    m_pCircle = gs_render_save();

	//Safe Area Function Need Check
	InitSafeAreas(&m_pActionSafeMargin, &m_pGraphicsSafeMargin,
				  &m_pFourByThreeSafeMargin, &m_pLeftLine, &m_pTopLine, &m_pRightLine);
	//Safe Area Function Need Check


	obs_leave_graphics();
}