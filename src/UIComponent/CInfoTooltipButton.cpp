#include "CInfoTooltipButton.h"
#include <QMouseEvent>
#include <QWidgetAction>

#define BALLOON_TIP_WIDTH 24
#define BALLOON_TIP_HEIGHT 4

AFQInfoTooltipButton::AFQInfoTooltipButton(QWidget* parent)
	: QPushButton(parent)
{
	connect(this, &QPushButton::clicked,
		this, &AFQInfoTooltipButton::_qslotShowExplanation);
}

AFQInfoTooltipButton::~AFQInfoTooltipButton()
{
	delete m_explanationWidget;
}

void AFQInfoTooltipButton::SetExplanationText(const QString& explanation, ToolTipPos balloonTipDir)
{
	m_explanationWidgetPos = balloonTipDir;
	m_explanationText = explanation;

	if (m_explanationLabel)
	{
		m_explanationLabel->setText(m_explanationText);
		m_explanationLabel->adjustSize();
		m_explanation->adjustSize();
		m_explanationWidget->adjustSize();
	}
}

#define GAP 5
void AFQInfoTooltipButton::_qslotShowExplanation() 
{
	if (!m_explanationWidget)
	{
		_CreateExplanationWidget();
		
		if (!m_explanationWidget)
			return;
	}

	if (!m_explanationWidget->isVisible())
	{
		QPoint questionBtnGlobalPos = mapToGlobal(QPoint(0, 0));
		
		// Top
		if (m_explanationWidgetPos == ToolTipPos::TopLeft ||
			m_explanationWidgetPos == ToolTipPos::TopCenter ||
			m_explanationWidgetPos == ToolTipPos::TopRight)
		{
			float moveY = m_explanationWidget->height() + GAP;
			questionBtnGlobalPos.setY(questionBtnGlobalPos.y() - moveY);
		}
		// Left
		else if (m_explanationWidgetPos == ToolTipPos::LeftTop ||
			m_explanationWidgetPos == ToolTipPos::LeftCenter ||
			m_explanationWidgetPos == ToolTipPos::LeftBottom) 
		{
			float moveX = m_explanationWidget->width() + GAP;
			questionBtnGlobalPos.setX(questionBtnGlobalPos.x() - moveX);
		}
		// Bottom
		else if (m_explanationWidgetPos == ToolTipPos::BottomLeft ||
			m_explanationWidgetPos == ToolTipPos::BottomCenter ||
			m_explanationWidgetPos == ToolTipPos::BottomRight)
		{
			float moveY = height() + GAP;
			questionBtnGlobalPos.setY(questionBtnGlobalPos.y() + moveY);
		}
		// Right
		else if (m_explanationWidgetPos == ToolTipPos::RightTop ||
			m_explanationWidgetPos == ToolTipPos::RightCenter ||
			m_explanationWidgetPos == ToolTipPos::RightBottom)
		{
			float moveX = width() + GAP;
			questionBtnGlobalPos.setX(questionBtnGlobalPos.x() + moveX);
		}


		// Horizontal Center
		if (m_explanationWidgetPos == ToolTipPos::TopCenter || 
			m_explanationWidgetPos == ToolTipPos::BottomCenter)
		{
			float moveX = (width() - m_explanationWidget->width()) * 0.5f;
			questionBtnGlobalPos.setX(questionBtnGlobalPos.x() + moveX);
		}
		// Horizontal Left
		else if (m_explanationWidgetPos == ToolTipPos::TopLeft ||
			m_explanationWidgetPos == ToolTipPos::BottomLeft)
		{
			float moveX = width() * 0.5f;
			questionBtnGlobalPos.setX(questionBtnGlobalPos.x() - moveX);
		}
		// Horizontal Right
		else if (m_explanationWidgetPos == ToolTipPos::TopRight ||
			m_explanationWidgetPos == ToolTipPos::BottomRight)
		{
			float moveX = m_explanationWidget->width() - width();
			questionBtnGlobalPos.setX(questionBtnGlobalPos.x() - moveX);
		}
		// Vertical Center
		else if (m_explanationWidgetPos == ToolTipPos::LeftCenter ||
			m_explanationWidgetPos == ToolTipPos::RightCenter) 
		{
			float moveY = (height() - m_explanationWidget->height()) * 0.5f;
			questionBtnGlobalPos.setY(questionBtnGlobalPos.y() + moveY);
		}
		// Vertical Top
		else if (m_explanationWidgetPos == ToolTipPos::LeftTop ||
			m_explanationWidgetPos == ToolTipPos::RightTop)
		{
		}
		// Vertical Bottom
		else if (m_explanationWidgetPos == ToolTipPos::LeftBottom ||
			m_explanationWidgetPos == ToolTipPos::RightBottom)
		{
			float moveY = (height() - m_explanationWidget->height());
			questionBtnGlobalPos.setY(questionBtnGlobalPos.y() + moveY);
		}

		m_explanationWidget->show(questionBtnGlobalPos);
		emit qsignalShowExplanationTriggered();
	}
}

void AFQInfoTooltipButton::_CreateExplanationWidget()
{
	m_explanationWidget = new AFQCustomMenu(this);
	m_explanationWidget->setObjectName("menu_toolTipBackground");

	m_explanation = new QWidget(m_explanationWidget);
	QLabel* balloonPoint = new QLabel("");
	m_explanationLabel = new QLabel();

	// Alignment
	Qt::Alignment alignment;
	if (m_explanationWidgetPos == ToolTipPos::TopLeft ||
		m_explanationWidgetPos == ToolTipPos::BottomLeft)
		alignment = Qt::AlignLeft;
	else if (m_explanationWidgetPos == ToolTipPos::TopCenter ||
		m_explanationWidgetPos == ToolTipPos::BottomCenter)
		alignment = Qt::AlignHCenter;
	else if (m_explanationWidgetPos == ToolTipPos::TopRight ||
		m_explanationWidgetPos == ToolTipPos::BottomRight)
		alignment = Qt::AlignRight;
	else if (m_explanationWidgetPos == ToolTipPos::LeftTop ||
		m_explanationWidgetPos == ToolTipPos::RightTop)
		alignment = Qt::AlignTop;
	else if (m_explanationWidgetPos == ToolTipPos::LeftCenter ||
		m_explanationWidgetPos == ToolTipPos::RightCenter)
		alignment = Qt::AlignVCenter;
	else if (m_explanationWidgetPos == ToolTipPos::LeftBottom ||
		m_explanationWidgetPos == ToolTipPos::RightBottom)
		alignment = Qt::AlignBottom;
	else
		return;
	//

	switch (m_explanationWidgetPos)
	{
	case ToolTipPos::TopLeft:
	case ToolTipPos::TopCenter:
	case ToolTipPos::TopRight:
	{
		balloonPoint->setFixedSize(QSize(BALLOON_TIP_WIDTH, BALLOON_TIP_HEIGHT));
		balloonPoint->setObjectName("label_BalloonPointBottom");
		QVBoxLayout* layout = new QVBoxLayout(m_explanation);
		layout->addWidget(m_explanationLabel);
		layout->addWidget(balloonPoint, 0, alignment);
		m_explanation->setLayout(layout);
		break;
	}
	case ToolTipPos::LeftTop:
	case ToolTipPos::LeftCenter:
	case ToolTipPos::LeftBottom:
	{
		balloonPoint->setFixedSize(QSize(BALLOON_TIP_HEIGHT, BALLOON_TIP_WIDTH));
		balloonPoint->setObjectName("label_BalloonPointRight");
		QHBoxLayout* layout = new QHBoxLayout(m_explanation);
		layout->addWidget(m_explanationLabel);
		layout->addWidget(balloonPoint, 0, alignment);
		m_explanation->setLayout(layout);
		break;
	}
	case ToolTipPos::BottomLeft:
	case ToolTipPos::BottomCenter:
	case ToolTipPos::BottomRight:
	{
		balloonPoint->setFixedSize(QSize(BALLOON_TIP_WIDTH, BALLOON_TIP_HEIGHT));
		balloonPoint->setObjectName("label_BalloonPointTop");
		QVBoxLayout* layout = new QVBoxLayout(m_explanation);
		layout->addWidget(balloonPoint, 0, alignment);
		layout->addWidget(m_explanationLabel);
		m_explanation->setLayout(layout);
		break;
	}
	case ToolTipPos::RightTop:
	case ToolTipPos::RightCenter:
	case ToolTipPos::RightBottom:
	{
		balloonPoint->setFixedSize(QSize(BALLOON_TIP_HEIGHT, BALLOON_TIP_WIDTH));
		balloonPoint->setObjectName("label_BalloonPointLeft");
		QHBoxLayout* layout = new QHBoxLayout(m_explanation);
		layout->addWidget(balloonPoint, 0, alignment);
		layout->addWidget(m_explanationLabel);
		m_explanation->setLayout(layout);
		break;
	}
	default:
		return;
	}

	if (m_explanation->layout() == nullptr)
		return;

	m_explanationLabel->setObjectName("label_toolTipItem");
	m_explanationLabel->setText(m_explanationText);

	m_explanation->layout()->setContentsMargins(0, 0, 0, 0);
	m_explanation->layout()->setSpacing(0);

	QWidgetAction* widgetAction = new QWidgetAction(m_explanationWidget);
	widgetAction->setDefaultWidget(m_explanation);

	m_explanationWidget->addAction(widgetAction);
	m_explanationWidget->adjustSize();
}