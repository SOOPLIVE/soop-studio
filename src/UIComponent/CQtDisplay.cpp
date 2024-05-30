#include "CQtDisplay.h"


#include "include/qt-wrapper.h" 
#include "Common/ColorMiscUtils.h"

#include <QWindow>
#include <QScreen>
#include <QShowEvent>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif


#include "ViewModel/Display/CExpressOBSDisplay.h"



class SurfaceEventFilter : public QObject {
    AFExpressOBSDisplay* display;

public:
	SurfaceEventFilter(AFQTDisplay* src, AFExpressOBSDisplay* core)
        : QObject(src), display(core) {}

protected:
	bool eventFilter(QObject* obj, QEvent* event) override
	{
		bool result = QObject::eventFilter(obj, event);
		QPlatformSurfaceEvent* surfaceEvent;

		switch (event->type()) {
		case QEvent::PlatformSurface:
			surfaceEvent =
				static_cast<QPlatformSurfaceEvent*>(event);

			switch (surfaceEvent->surfaceEventType()) {
			case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed:
				display->DestroyDisplay();
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}

		return result;
	}
};



AFQTDisplay::AFQTDisplay(QWidget* parent, Qt::WindowFlags flags)
	: QWidget(parent, flags)
{
#ifdef __APPLE__
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_OpaquePaintEvent);
#endif
	//setAttribute(Qt::WA_StaticContents);
	//setAttribute(Qt::WA_NoSystemBackground);
	//setAttribute(Qt::WA_DontCreateNativeAncestors);
	setAttribute(Qt::WA_NativeWindow);


	if (m_pViewModel == nullptr)
		m_pViewModel = new AFExpressOBSDisplay(this);


	connect(windowHandle(), &QWindow::visibleChanged, this, &AFQTDisplay::qslotWindowVisible);
	connect(windowHandle(), &QWindow::screenChanged, this, &AFQTDisplay::qslotScreenChagned);

	windowHandle()->installEventFilter(new SurfaceEventFilter(this, m_pViewModel));
}

AFQTDisplay::~AFQTDisplay()
{
	if(m_pViewModel != nullptr)
        delete m_pViewModel;
	this->close();
}

void AFQTDisplay::qslotWindowVisible(bool visible)
{
	if (m_pViewModel == nullptr)
		return;


	if (!visible) {
#if !defined(_WIN32) && !defined(__APPLE__)
		m_pViewModel->ResetDisplay();
#endif
		return;
	}

	
	QSize size = GetPixelSize(this);
	bool bResResize = m_pViewModel->ResizeDisplay(m_bgColor, size.width(), size.height());
	if (!bResResize)
	{
		if (m_pViewModel->CreateDisplay(m_bgColor))
			emit qsignalDisplayCreated(this);
	}
}

void AFQTDisplay::qslotScreenChagned(QScreen*)
{
	if (m_pViewModel == nullptr)
		return;


	if (m_pViewModel->CreateDisplay(m_bgColor))
		emit qsignalDisplayCreated(this);

	QSize size = GetPixelSize(this);
	m_pViewModel->ResizeDisplay(m_bgColor, size.width(), size.height());
}

void AFQTDisplay::paintEvent(QPaintEvent* event)
{
	if (m_pViewModel)
	{
		if (m_pViewModel->CreateDisplay(m_bgColor))
			emit qsignalDisplayCreated(this);
	}


	QWidget::paintEvent(event);
}

void AFQTDisplay::moveEvent(QMoveEvent* event)
{
	QWidget::moveEvent(event);

	OnMove();
}

bool AFQTDisplay::nativeEvent(const QByteArray&, void* message, qintptr*)
{
#ifdef _WIN32
	const MSG& msg = *static_cast<MSG*>(message);
	switch (msg.message) {
	case WM_DISPLAYCHANGE:
		OnDisplayChange();
	}
#else
	UNUSED_PARAMETER(message);
#endif

	return false;
}

void AFQTDisplay::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);

	if (m_pViewModel)
	{
		if (m_pViewModel->CreateDisplay(m_bgColor))
			emit qsignalDisplayCreated(this);
	}

	if (isVisible() && m_pViewModel != nullptr)
	{
		QSize size = GetPixelSize(this);
		m_pViewModel->ResizeDisplay(m_bgColor, size.width(), size.height());
	}

	emit qsignalDisplayResized();
}

QPaintEngine* AFQTDisplay::paintEngine() const
{
	return nullptr;
}

QColor AFQTDisplay::GetDisplayBackgroundColor() const
{
	return rgba_to_color(m_bgColor);
}

void AFQTDisplay::SetDisplayBackgroundColor(const QColor& color)
{
	uint32_t newBackgroundColor = (uint32_t)color_to_int(color);

	if (newBackgroundColor != m_bgColor)
	{
		m_bgColor = newBackgroundColor;

		if (m_pViewModel != nullptr)
			m_pViewModel->UpdateDisplayBackgroundColor(m_bgColor);
	}
}

obs_display_t* AFQTDisplay::GetDisplay() const
{
	if (m_pViewModel != nullptr)
		return m_pViewModel->GetOBSDisplay();
	else
		return nullptr;
}

void AFQTDisplay::OnMove()
{
	if (m_pViewModel != nullptr)
		m_pViewModel->UpdateDisplayColorSpace();
}

void AFQTDisplay::OnDisplayChange()
{
	if (m_pViewModel != nullptr)
		m_pViewModel->UpdateDisplayColorSpace();
}
