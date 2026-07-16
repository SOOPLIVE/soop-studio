#include "CClickableFrame.h"

AFQClickableFrame::AFQClickableFrame(QWidget* parent)
{

}

void AFQClickableFrame::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit qsignalFrameClicked();
    }
}