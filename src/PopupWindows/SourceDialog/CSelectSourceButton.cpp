#include "CSelectSourceButton.h"

#include <QStyle>
#include <QBoxLayout>
#include <QFile>
#include <QMovie>
#include <QScreen>
#include <QPainter>

#include "qt-wrapper.h"
#include "platform/platform.hpp"

#include "CoreModel/Locale/CLocaleTextManager.h"

AFQSelectSourceButton::AFQSelectSourceButton(QString id, QString name, QWidget* parent)
	: m_srcId(id)
	, QPushButton(parent)
{
	setAttribute(Qt::WA_Hover);
	//setAttribute(Qt::WA_OpaquePaintEvent, true);
    
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
    
	m_strIconPath = QString("%1/select-source-dialog/idle-icon/ic_%2.svg").arg(absPath.data(), convertID);
	m_strIconHoverPath = QString("%1/select-source-dialog/hover-icon/ic_%2.svg").arg(absPath.data(), convertID);

	//checkFileExists(m_strIconPath);
	//checkFileExists(m_strIconHoverPath);
	//checkFileExists(m_strGuidePath);

	QVBoxLayout* layout = new QVBoxLayout(this);

	m_pIconLabel = new QLabel(this);
	m_pTextLabel = new AFQElidedSlideLabel(this);

	m_pIconLabel->setFixedSize(22, 22);
	m_pIconLabel->setPixmap(QPixmap(m_strIconPath).scaled(QSize(22, 22), Qt::KeepAspectRatio));
	
	m_pTextLabel->setStyleSheet("QLabel { color : rgba(255, 255, 255, 70%); }");
	m_pTextLabel->setText(name);
	m_pTextLabel->setAlignment(Qt::AlignLeft);

	connect(this, &AFQSelectSourceButton::qSignalHoverButton,
			m_pTextLabel, &AFQElidedSlideLabel::qSlotHoverButton);

	connect(this, &AFQSelectSourceButton::qSignalLeaveButton,
			m_pTextLabel, &AFQElidedSlideLabel::qSlotLeaveButton);


	layout->addWidget(m_pIconLabel);
	layout->addWidget(m_pTextLabel);

	setLayout(layout);

	setFixedSize(160, 65);
}

AFQSelectSourceButton::~AFQSelectSourceButton()
{
	m_pIconLabel = nullptr;
	m_pTextLabel = nullptr;	
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
			m_bHover = true;

			m_pIconLabel->setPixmap(QPixmap(m_strIconHoverPath).scaled(QSize(m_pIconLabel->width(), m_pIconLabel->height()), Qt::KeepAspectRatio));
			m_pTextLabel->setStyleSheet("QLabel { color : rgba(255,255,255,100%); }");

			emit qSignalHoverButton(m_srcId);
		}
		break;
	case QEvent::HoverLeave:
		{
			m_bHover = false;

			m_pIconLabel->setPixmap(QPixmap(m_strIconPath).scaled(QSize(m_pIconLabel->width(), m_pIconLabel->height()), Qt::KeepAspectRatio));
			m_pTextLabel->setStyleSheet("QLabel { color : rgba(255,255,255,70%); }");

			emit qSignalLeaveButton();
		}
		break;

	case QEvent::MouseButtonPress:
		{
			m_pIconLabel->setPixmap(QPixmap(m_strIconPath).scaled(QSize(m_pIconLabel->width(), m_pIconLabel->height()), Qt::KeepAspectRatio));
			m_pTextLabel->setStyleSheet("QLabel { color : #a7a9ab; }");
		}
		break;

	case QEvent::MouseButtonRelease:
		{
			if(m_bHover)
				m_pIconLabel->setPixmap(QPixmap(m_strIconHoverPath).scaled(QSize(m_pIconLabel->width(), m_pIconLabel->height()), Qt::KeepAspectRatio));
			else
				m_pIconLabel->setPixmap(QPixmap(m_strIconPath).scaled(QSize(m_pIconLabel->width(), m_pIconLabel->height()), Qt::KeepAspectRatio));

			m_pTextLabel->setStyleSheet("QLabel { color : #a7a9ab; }");
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