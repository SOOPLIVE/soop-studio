#include "CExpressOBSDisplay.h"

#include <QWidget>
#include <QWindow>


#include "DisplayMisc.h"
#include "include/qt-wrapper.h"


AFExpressOBSDisplay::AFExpressOBSDisplay(QWidget* pView)
	:m_pConnectedView(pView)
{
}

AFExpressOBSDisplay::~AFExpressOBSDisplay()
{
	m_obsDisplay = nullptr;
	m_pConnectedView = nullptr;
}

bool AFExpressOBSDisplay::CreateDisplay(uint32_t bgColor)
{
	if (m_obsDisplay)
		return false;

	if (m_bDestroying)
		return false;

	if (m_pConnectedView == nullptr)
		return false;
    
    QWindow* pNativeWindow = m_pConnectedView->windowHandle();
    if (pNativeWindow == nullptr)
        return false;

	//if (!m_pConnectedView->windowHandle()->isExposed())
	//	return false;
	//

	if (!pNativeWindow->isVisible())
		return false;

	QSize size = GetPixelSize(m_pConnectedView);

	gs_init_data info = {};
	info.cx = size.width();
	info.cy = size.height();
	info.format = GS_BGRA;
	info.zsformat = GS_ZS_NONE;

	if (!QTToGSWindow(pNativeWindow, info.window))
		return false;

	m_obsDisplay = obs_display_create(&info, bgColor);

	return true;
}

bool AFExpressOBSDisplay::ResizeDisplay(uint32_t bgColor, uint32_t cx, uint32_t cy)
{
	if (m_obsDisplay == nullptr)
		return false;
	else
	{
		obs_display_resize(m_obsDisplay, cx, cy);
		return true;
	}
}

void AFExpressOBSDisplay::ResetDisplay()
{
	m_obsDisplay = nullptr;
}

void AFExpressOBSDisplay::DestroyDisplay()
{
	m_obsDisplay = nullptr;
	m_bDestroying = true;
}

void AFExpressOBSDisplay::UpdateDisplayBackgroundColor(uint32_t bgColor)
{
	if (m_obsDisplay == nullptr)
		return;

	obs_display_set_background_color(m_obsDisplay, bgColor);
}

void AFExpressOBSDisplay::UpdateDisplayColorSpace()
{
	if (m_obsDisplay == nullptr)
		return;

	obs_display_update_color_space(m_obsDisplay);
}
