#include "CElidedSlideLabel.h"


#include <QPainter>

AFQElidedSlideLabel::AFQElidedSlideLabel(QWidget* parent) :
	QLabel(parent)
{
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

	m_animation = new QPropertyAnimation(this, "offset", this);
	m_animation->setLoopCount(1);

	connect(m_animation, &QPropertyAnimation::finished,
		this, &AFQElidedSlideLabel::qSlotFinishedAnimation);
}

int  AFQElidedSlideLabel::GetOffset() {
	return m_offset;
}

void AFQElidedSlideLabel::SetOffset(int offset) {
	m_offset = offset;
	update();
}

void AFQElidedSlideLabel::qSlotHoverButton(QString)
{
	startAnimation();

	m_isHoverd = true;
}

void AFQElidedSlideLabel::qSlotLeaveButton()
{
	finishAnimation();

	m_isHoverd = false;
}

void AFQElidedSlideLabel::qSlotFinishedAnimation()
{
	repeatAnimation();
}

void AFQElidedSlideLabel::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	QFontMetrics metrics(font());
	QString elidedText = metrics.elidedText(text(), Qt::ElideRight, width());

	if (m_animation->state() == QPropertyAnimation::Running)
		painter.drawText(-m_offset, 0, m_fullTextWidth, height(), Qt::AlignLeft | Qt::AlignVCenter, text());
	else
		painter.drawText(rect(), Qt::AlignLeft | Qt::AlignVCenter, elidedText);
}

void AFQElidedSlideLabel::startAnimation()
{
	if (!m_animation)
		return;


	QFontMetrics metrics(font());
	int textWidth = metrics.horizontalAdvance(text());
	int duration = textWidth * 1000 / m_speed;

	m_animation->setDuration(duration);

	m_fullTextWidth = textWidth;
	if (textWidth > width()) {
		m_animation->setStartValue(0);
		m_animation->setEndValue(textWidth);
		m_animation->start();
	}
}

void AFQElidedSlideLabel::finishAnimation()
{
	if (m_animation) {
		m_animation->stop();
		SetOffset(0);
	}
}

void AFQElidedSlideLabel::repeatAnimation()
{
	if (!m_isHoverd)
		return;

	QFontMetrics metrics(font());
	int textWidth = metrics.horizontalAdvance(text());
	int duration = (width() + textWidth) * 1000 / m_speed;

	m_animation->setDuration(duration);

	m_fullTextWidth = textWidth;
	if (textWidth > width()) {
		m_animation->setStartValue(-width());
		m_animation->setEndValue(textWidth);
		m_animation->start();
	}
}