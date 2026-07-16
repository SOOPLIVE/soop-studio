#include "CircularProgress.h"
#include <QPainter>
#include <QColor>

CircularProgress::CircularProgress(QWidget* parent)
{
    value = 60;
    maxValue = 60;
    setFixedSize(84, 84);
}

void CircularProgress::setMaxValue(int v)
{
    maxValue = v;
}

void CircularProgress::setTimeValue(int v)
{
    value = v;
    update();
}

void CircularProgress::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int side = qMin(width(), height());
    float thickness = 4.5f;
    QRectF rect(thickness / 2 + 0.5, thickness / 2 + 0.5,
        side - thickness - 1.0, side - thickness - 1.0);

    painter.setPen(QPen(QColor("#1A1A1A"), thickness));
    painter.drawEllipse(rect);

    double remaining = (double)value / maxValue;
    double elapsed = 1.0 - remaining;            

    int startAngle = (90 - (elapsed * 360)) * 16;

    int spanAngle = -remaining * 360 * 16;

    QPen bluePen(QColor("#0088FF"), thickness);
    bluePen.setCapStyle(Qt::RoundCap);
    painter.setPen(bluePen);

    painter.drawArc(rect, startAngle, spanAngle);

    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 22, QFont::DemiBold));
    painter.drawText(rect.adjusted(0, -5, 0, -5), Qt::AlignCenter, QString::number(value));

    painter.setPen(QColor("#888888"));
    painter.setFont(QFont("Arial", 10));
    painter.drawText(rect.adjusted(0, 22, 0, 22), Qt::AlignCenter, "sec");
}