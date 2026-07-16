#include "CSelectSourceButton.h"

#include <QStyle>
#include <QBoxLayout>
#include <QFile>
#include <QMovie>
#include <QScreen>
#include <QPainter>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Icon/CIconContext.h"

#include "MainFrame/CMainFrame.h"

AFQSelectSourceButton::AFQSelectSourceButton(QString id, QString name, QWidget* parent)
	: m_srcId(id)
	, QPushButton(parent)
{
	setAttribute(Qt::WA_Hover);

	std::string absPath;
	GetDataFilePath("assets", absPath);

	QString convertID = id;

#ifdef __APPLE__
	if (convertID == "syphon-input")
		convertID = "game_capture";
	else if (convertID == "screen_capture")
		convertID = "monitor_capture";
	else if (convertID == "av_capture_input")
		convertID = "dshow_input";
	else if (convertID == "sck_audio_capture")
		convertID = "wasapi_input_capture";
	else if (convertID == "coreaudio_input_capture")
		convertID = "wasapi_input_capture";
	else if (convertID == "text_ft2_source")
		convertID = "text_gdiplus";
#endif

	QBoxLayout* layout = nullptr;

	layout = new QHBoxLayout(this);
	setFixedSize(160, 42);

	m_iconLabel = new QLabel(this);
	m_textLabel = new AFQElidedSlideLabel(this);
	m_textLabel->setObjectName("label_SourceBtnText");
	m_textLabel->setProperty("buttonState", "idle");

	QIcon icSource;
	if (id.compare("scene") == 0)
		icSource = ICON_CONTEXT.GetSceneIcon();
	else if (id.compare("group") == 0)
		icSource = ICON_CONTEXT.GetGroupIcon();
	else
		icSource = ICON_CONTEXT.GetSourceIcon(id.toStdString().c_str());

	m_iconLabel->setFixedSize(24, 24);
	m_iconLabel->setPixmap(icSource.pixmap(QSize(24, 24), Qt::KeepAspectRatio));

	m_textLabel->setText(name);
	m_textLabel->setAlignment(Qt::AlignLeft);

	connect(this, &AFQSelectSourceButton::qSignalHoverButton,
		m_textLabel, &AFQElidedSlideLabel::qslotHoverButton);

	connect(this, &AFQSelectSourceButton::qSignalLeaveButton,
		m_textLabel, &AFQElidedSlideLabel::qslotLeaveButton);

	layout->addWidget(m_iconLabel);
	layout->addWidget(m_textLabel);

	setLayout(layout);
}

AFQSelectSourceButton::~AFQSelectSourceButton()
{
	m_iconLabel = nullptr;
	m_textLabel = nullptr;	
}

QString AFQSelectSourceButton::GetSourceId()
{
	return m_srcId;
}

bool AFQSelectSourceButton::event(QEvent* e)
{
	switch (e->type()) {
	case QEvent::HoverEnter:
		{
			m_hover = true;

			m_textLabel->setProperty("buttonState", "hover");
			PolishStyleSheet(m_textLabel);
			emit qSignalHoverButton(m_srcId);
		}
		break;
	case QEvent::HoverLeave:
		{
			m_hover = false;

			m_textLabel->setProperty("buttonState", "idle");
			PolishStyleSheet(m_textLabel);
			emit qSignalLeaveButton();
		}
		break;

	case QEvent::MouseButtonPress:
		{
			m_textLabel->setProperty("buttonState", "press");
			PolishStyleSheet(m_textLabel);
		}
		break;

	case QEvent::MouseButtonRelease:
		{
			m_textLabel->setProperty("buttonState", "press");
			PolishStyleSheet(m_textLabel);
		}
		break;

	case QEvent::Hide:
	{

	}
	break;

	default:
		break;
	}

	return QPushButton::event(e);
}
