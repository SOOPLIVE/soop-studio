#include "CVodListScrollAreaContents.h"

#include <QTimer>
#include <QPainter>
#include <QStyleOption>
#include <QScrollBar>

#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>

#include "MainFrame/CMainFrame.h"

#include "CVodSourceDialog.h"
#include "CVodListItem.h"

AFQVodListScrollAreaContents::AFQVodListScrollAreaContents(QWidget* parent) :
    QWidget(parent)
{

}

AFQVodListScrollAreaContents::~AFQVodListScrollAreaContents()
{

}

void AFQVodListScrollAreaContents::paintEvent(QPaintEvent* event)
{
    if (!m_pVodDialog)
        return;

    if (m_pVodDialog->IsEmptyVodPlayList()) {
        QPainter p(this);
        QStyleOption option;
        option.initFrom(this);

        style()->drawPrimitive(QStyle::PE_Widget, &option, &p, this);

        QColor textColor(145, 150, 161);
        p.setPen(textColor);

        m_textNoItem.prepare(QTransform(), p.font());
        m_textNoItem_2.prepare(QTransform(), p.font());

        QSizeF textSize = m_textNoItem.size();
        QSizeF textSize2 = m_textNoItem_2.size();
        QSizeF thisSize = size();
        const qreal spacing = 5.0;

        qreal totalHeight = spacing + textSize.height();

        qreal x = thisSize.width() / 2.0 - textSize.width() / 2.0;
        qreal y = thisSize.height() / 2.0 - totalHeight / 2.0 - 22;

        QRect rcWidget = QRect(0,0,thisSize.width(), thisSize.height());
        p.fillRect(rcWidget, QColor(25, 27, 32));
        p.drawStaticText(x, y, m_textNoItem);

        x = thisSize.width() / 2.0 - textSize2.width() / 2.0;
        p.drawStaticText(x, y + 22, m_textNoItem_2);
    }
    else {
        QPainter painter(this);
        QStyleOption option;
        option.initFrom(this);

        style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);

        QWidget::paintEvent(event);
    }
}

void AFQVodListScrollAreaContents::SetVodDialogPtr(AFQVodSourceDialog* vodDialog)
{
    m_pVodDialog = vodDialog;
}

void AFQVodListScrollAreaContents::SetVodEmtpyPlayListMessage(QString msg_1, QString msg_2)
{
    QTextOption opt(Qt::AlignHCenter);
    opt.setWrapMode(QTextOption::NoWrap);
    m_textNoItem.setTextOption(opt);
    m_textNoItem_2.setTextOption(opt);
    m_textNoItem.setText(msg_1.replace("\n", "<br/>"));
    m_textNoItem_2.setText(msg_2.replace("\n", "<br/>"));
}
