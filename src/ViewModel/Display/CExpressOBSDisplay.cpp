#include "CExpressOBSDisplay.h"

#include <QWidget>
#include <QWindow>

#include "DisplayMisc.h"
#include "display-helpers.hpp"


static bool QTToGSWindow(QWindow* window, gs_window& gswindow)
{
	bool success = true;

#ifdef _WIN32
	gswindow.hwnd = (HWND)window->winId();
#elif __APPLE__
	gswindow.view = (id)window->winId();
#else
	switch(obs_get_nix_platform()) {
		case OBS_NIX_PLATFORM_X11_EGL:
			gswindow.id = window->winId();
			gswindow.display = obs_get_nix_platform_display();
			break;
#ifdef ENABLE_WAYLAND
		case OBS_NIX_PLATFORM_WAYLAND: {
			QPlatformNativeInterface* native = QGuiApplication::platformNativeInterface();
			gswindow.display = native->nativeResourceForWindow("surface", window);
			success = gswindow.display != nullptr;
			break;
		}
#endif
		default:
			success = false;
			break;
	}
#endif
	return success;
}

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

	if (m_destroying)
		return false;

	if (m_pConnectedView == nullptr)
		return false;
    
    QWindow* pNativeWindow = m_pConnectedView->windowHandle();
    if (pNativeWindow == nullptr)
        return false;

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
	m_destroying = true;
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
