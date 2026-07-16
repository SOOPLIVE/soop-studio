#include "CElidedSlideLabel.h"

#include <QPainter>
#include <QMouseEvent>
#include <QTextBoundaryFinder>

#include "CoreModel/Auth/SBroadInfo.h"		// PREFIX_REC

AFQElidedSlideLabel::AFQElidedSlideLabel(QWidget* parent) :
	QLabel(parent)
{
	setMouseTracking(true);
	setAttribute(Qt::WA_Hover);
	installEventFilter(this);

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

	m_pAnimation = new QPropertyAnimation(this, "offset", this);
	m_pAnimation->setLoopCount(1);

	connect(m_pAnimation, &QPropertyAnimation::finished, this, &AFQElidedSlideLabel::qslotFinishedAnimation);

	connect(this, &AFQElidedSlideLabel::qsignalTextChanged, this, &AFQElidedSlideLabel::qslotTextChanged);
}

int  AFQElidedSlideLabel::GetOffset() {
	return m_offset;
}

void AFQElidedSlideLabel::SetOffset(int offset) {
	m_offset = offset;
	update();
}

void AFQElidedSlideLabel::SetMultiLine(bool multiLine)
{
	m_multiLine = multiLine;
}

void AFQElidedSlideLabel::updateText(const QString& text)
{
	// single line use QLabel::setText() Function
	if (!m_multiLine)
		return;

	QLabel::setText(text);

	m_originText = text;

	emit qsignalTextChanged();
}


void AFQElidedSlideLabel::SetElidedAnimation(bool set)
{
	if (set) {
		connect(this, &AFQElidedSlideLabel::qsignalHoverEnter,
			this, &AFQElidedSlideLabel::qslotHoverLabel);
		connect(this, &AFQElidedSlideLabel::qsignalHoverLeave,
			this, &AFQElidedSlideLabel::qslotLeaveButton);
	}
	else {
		disconnect(this, &AFQElidedSlideLabel::qsignalHoverEnter,
			this, &AFQElidedSlideLabel::qslotHoverLabel);
		disconnect(this, &AFQElidedSlideLabel::qsignalHoverLeave,
			this, &AFQElidedSlideLabel::qslotLeaveButton);
	}
}

void AFQElidedSlideLabel::qslotHoverButton(QString)
{
	if(!m_multiLine)
		startAnimation();

	m_isHoverd = true;
}

void AFQElidedSlideLabel::qslotHoverLabel()
{
	if (!m_multiLine)
		startAnimation();

	m_isHoverd = true;
}

void AFQElidedSlideLabel::qslotHoverWidth(int maxWidth)
{
	if (!m_multiLine)
		startAnimationWithWidth(maxWidth);

	m_isHoverd = true;
}

void AFQElidedSlideLabel::qslotLeaveButton()
{
	if (!m_multiLine)
		finishAnimation();

	m_isHoverd = false;
}

void AFQElidedSlideLabel::qslotFinishedAnimation()
{
	if (!m_multiLine)
		repeatAnimation();
}

void AFQElidedSlideLabel::qslotTextChanged()
{
	refreshMultiLineText();
}

void AFQElidedSlideLabel::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	QFontMetrics metrics(font());

	bool center = this->property("centerPosition").toBool();
	int flags = Qt::AlignLeft | Qt::AlignVCenter;
	if (center)
		flags = Qt::AlignCenter;

	if (false == m_multiLine)
	{
		if (m_pAnimation->state() == QPropertyAnimation::Running)
			painter.drawText(-m_offset, 0, m_fullTextWidth, height(), Qt::AlignLeft | Qt::AlignVCenter, text());
		else {
			QString elidedText = metrics.elidedText(text(), Qt::ElideRight, width());
			painter.drawText(rect(), flags, elidedText);
		}
	}
	else
	{	
		int maxWidth = rect().width();
		int lineHeight = metrics.height();

		painter.drawText(QRect(0, 0, maxWidth, lineHeight), Qt::AlignLeft | Qt::AlignTop, m_multiLine1);

		if (!m_multiLine2.isEmpty()) {
			painter.drawText(QRect(0, lineHeight, maxWidth, lineHeight), Qt::AlignLeft | Qt::AlignTop, m_multiLine2);
		}
	}
}

void AFQElidedSlideLabel::resizeEvent(QResizeEvent* event)
{
	refreshMultiLineText();
}

bool AFQElidedSlideLabel::event(QEvent* e)
{
	QMouseEvent* mouseEvent;
	QPoint mousePos;
	QRect widgetRect;

	if (this->isEnabled()) {
		switch (e->type())
		{
		case QEvent::HoverEnter:
			emit qsignalHoverEnter();
			break;
		case QEvent::HoverLeave:
			emit qsignalHoverLeave();
			break;
		case QEvent::MouseButtonRelease:
			mouseEvent = dynamic_cast<QMouseEvent*>(e);
			if (mouseEvent) {
				mousePos = mouseEvent->pos();
				widgetRect = rect();
				if (!widgetRect.contains(mousePos)) {
					return false;
				}
				if (mouseEvent->button() == Qt::LeftButton) {
					emit qsignalMouseClick();
				}
			}
			break;
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonDblClick:
			// Without double click event, the back of the widget can be clicked
			break;
		default:
			break;
		}
	}

	return QWidget::event(e);
}

void AFQElidedSlideLabel::startAnimation()
{
	if (m_multiLine)
		return;

	if (!m_pAnimation)
		return;

	QFontMetrics metrics(font());

	int textWidth = metrics.horizontalAdvance(text());
	int duration = textWidth * 1000 / m_speed;

	m_pAnimation->setDuration(duration);

	m_fullTextWidth = textWidth;
	if (textWidth > width()) {
		m_pAnimation->setStartValue(0);
		m_pAnimation->setEndValue(textWidth);
		m_pAnimation->start();
	}
}

void AFQElidedSlideLabel::startAnimationWithWidth(int maxWidth)
{
	if (m_multiLine)
		return;

	if (!m_pAnimation)
		return;


	QFontMetrics metrics(font());

	int textWidth = metrics.horizontalAdvance(text());
	int duration = textWidth * 1000 / m_speed;

	m_pAnimation->setDuration(duration);

	m_fullTextWidth = textWidth;
	if (textWidth > maxWidth) {
		m_pAnimation->setStartValue(0);
		m_pAnimation->setEndValue(textWidth);
		m_pAnimation->start();
	}
}

void AFQElidedSlideLabel::finishAnimation()
{
	if (m_multiLine)
		return;

	if (m_pAnimation) {
		m_pAnimation->stop();
		SetOffset(0);
	}
}

void AFQElidedSlideLabel::repeatAnimation()
{
	if (m_multiLine)
		return;

	if (!m_isHoverd)
		return;

	QFontMetrics metrics(font());
	int textWidth = metrics.horizontalAdvance(text());
	int duration = (width() + textWidth) * 1000 / m_speed;

	m_pAnimation->setDuration(duration);

	m_fullTextWidth = textWidth;
	if (textWidth > width()) {
		m_pAnimation->setStartValue(-width());
		m_pAnimation->setEndValue(textWidth);
		m_pAnimation->start();
	}
}

void AFQElidedSlideLabel::refreshMultiLineText()
{
	if (!m_multiLine)
		return;

	QFontMetrics metrics(font());

	QString fullText = m_originText;

	const int adjustW = 5;
	const int maxWidth = rect().width() + adjustW;
	const int lineHeight = metrics.height();

	m_multiLine1.clear();
	
	QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, fullText);

	int width = 0;
	while (finder.position() < fullText.size()) {
		int start = finder.position();
		finder.toNextBoundary();
		int end = finder.position();

		QString grapheme = fullText.mid(start, end - start);
		int w = metrics.horizontalAdvance(grapheme);
		if (width + w > maxWidth)
			break;
		m_multiLine1.append(grapheme);
		width += w;
	}

	m_multiLine2 = fullText.mid(m_multiLine1.length()).trimmed();
	QString tempMultiLine2 = metrics.elidedText(m_multiLine2, Qt::ElideRight, maxWidth);

	if (0 != m_multiLine2.compare(tempMultiLine2)) {
		QLabel::setToolTip(fullText);
	}
	else {
		QLabel::setToolTip("");
	}

	m_multiLine2 = tempMultiLine2;

	if (m_twoLine != (!m_multiLine2.isEmpty())) {
		m_twoLine = !m_multiLine2.isEmpty();

		emit qsignalTextMultiLineChanged(m_twoLine);
	}

	update();
}
