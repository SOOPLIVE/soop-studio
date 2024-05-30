#include "CGraphicsContext.h"


#include <util/dstr.h>

#include "CoreModel/Config/CConfigManager.h"


#define DL_OPENGL "libobs-opengl.dylib"
#define DL_METAL ""
#define DL_D3D11 "libobs-d3d11.dll"
//

void AFGraphicsContext::InitContext()
{

	_InitPrimitives();

	m_bInited = true;
}

bool AFGraphicsContext::FinContext()
{
	bool res = false;

	obs_enter_graphics();
	gs_vertexbuffer_destroy(m_vbBox);
	gs_vertexbuffer_destroy(m_vbBoxLeft);
	gs_vertexbuffer_destroy(m_vbBoxTop);
	gs_vertexbuffer_destroy(m_vbBoxRight);
	gs_vertexbuffer_destroy(m_vbBoxBottom);
	gs_vertexbuffer_destroy(m_vbCircle);
	gs_vertexbuffer_destroy(m_vActionSafeMargin);
	gs_vertexbuffer_destroy(m_vGraphicsSafeMargin);
	gs_vertexbuffer_destroy(m_FourByThreeSafeMargin);
	gs_vertexbuffer_destroy(m_LeftLine);
	gs_vertexbuffer_destroy(m_TopLine);
	gs_vertexbuffer_destroy(m_RightLine);
	obs_leave_graphics();

	res = true;

	if (res)
		m_bInited = false;

	return res;
}

void AFGraphicsContext::ClearContext()
{

}

const char* AFGraphicsContext::GetRenderModule() const
{
	const char* renderer =
		config_get_string(AFConfigManager::GetSingletonInstance().GetGlobal(),
						  "Video", "Renderer");

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
	m_vbBox = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(0.0f, 1.0f);
	m_vbBoxLeft = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(1.0f, 0.0f);
	m_vbBoxTop = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(1.0f, 0.0f);
	gs_vertex2f(1.0f, 1.0f);
	m_vbBoxRight = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 1.0f);
	gs_vertex2f(1.0f, 1.0f);
	m_vbBoxBottom = gs_render_save();

	gs_render_start(true);
	for (int i = 0; i <= 360; i += (360 / 20)) {
		float pos = RAD(float(i));
		gs_vertex2f(cosf(pos), sinf(pos));
	}
	m_vbCircle = gs_render_save();

	InitSafeAreas(&m_vActionSafeMargin, &m_vGraphicsSafeMargin,
				  &m_FourByThreeSafeMargin, &m_LeftLine, &m_TopLine, &m_RightLine);   

	obs_leave_graphics();
}
