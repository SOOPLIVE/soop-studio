#include "COverlayLabelSourceWidget.h"

#include <QTimer>

#include <qgraphicseffect.h>

#include <util/base.h>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp" 

#include "Utils/OverlayManager.h"

#include "CoreModel/Auth/CAuthManager.h"

#include "MainFrame/CMainFrame.h"

OverlayLabelSourceWidget::OverlayLabelSourceWidget(QWidget* parent, bool editable, Types type) : OverlaySourceWidget(parent), editable(editable), type(type)
{
    installEventFilter(parent);
    raise();

    double x = (double)0;
    double y = (double)0;

    if (type == Types::time)
    {
        config_set_default_double(ACTIVECONFIG, "Overlay", "Time.X", (double)-1);
        x = config_get_double(ACTIVECONFIG, "Overlay", "Time.X");
        if (x < 0)
        {
            auto w = OVERLAY_MANAGER.TargetRect(this).width();
            x = (double)(w - fix_w - 8);
            x /= (double)w;
        }

        config_set_default_double(ACTIVECONFIG, "Overlay", "Time.Y", (double)-1);
        y = config_get_double(ACTIVECONFIG, "Overlay", "Time.Y");
        if (y < 0)
        {
            auto h = OVERLAY_MANAGER.TargetRect(this).height();
            y = (double)(h - fix_h - 8); 
            y /= (double)h;
        }
    }
    else if (type == Types::gift)
    {
        config_set_default_double(ACTIVECONFIG, "Overlay", "Gift.X", (double)-1);
        x = config_get_double(ACTIVECONFIG, "Overlay", "Gift.X");
        if (x < 0)
        {
            x = (double)8;
            auto w = OVERLAY_MANAGER.TargetRect(this).width();
            x /= (double)w;
        }

        config_set_default_double(ACTIVECONFIG, "Overlay", "Gift.Y", (double)-1);
        y = config_get_double(ACTIVECONFIG, "Overlay", "Gift.Y");
        if (y < 0)
        {
            y = (double)8;
            auto h = OVERLAY_MANAGER.TargetRect(this).height();
            y /= (double)h;
        }
    }
    else if (type == Types::user)
    {
        config_set_default_double(ACTIVECONFIG, "Overlay", "User.X", (double)-1);
        x = config_get_double(ACTIVECONFIG, "Overlay", "User.X");
        if (x < 0)
        {
            x = (double)8;
            auto w = OVERLAY_MANAGER.TargetRect(this).width();
            x /= (double)w;
        }

        config_set_default_double(ACTIVECONFIG, "Overlay", "User.Y", (double)-1);
        y = config_get_double(ACTIVECONFIG, "Overlay", "User.Y");
        if (y < 0)
        {
            auto h = OVERLAY_MANAGER.TargetRect(this).height();
            y = (double)(h - fix_h - 8);
            y /= (double)h;
        }
    }
    else if (type == Types::up)
    {
        config_set_default_double(ACTIVECONFIG, "Overlay", "Up.X", (double)-1);
        x = config_get_double(ACTIVECONFIG, "Overlay", "Up.X");
        if (x < 0)
        {
            x = (double)(8 + fix_w + 8);
            auto w = OVERLAY_MANAGER.TargetRect(this).width();
            x /= (double)w;
        }

        config_set_default_double(ACTIVECONFIG, "Overlay", "Up.Y", (double)-1);
        y = config_get_double(ACTIVECONFIG, "Overlay", "Up.Y");
        if (y < 0)
        {
            auto h = OVERLAY_MANAGER.TargetRect(this).height();
            y = (double)(h - fix_h - 8); 
            y /= (double)h; 
        }
    }

    moveRelative(QPointF((qreal)x, (qreal)y));

    setFixedSize(fix_w, fix_h);

    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, editable);
    setStyleSheet("background-color : rgba(0, 0, 0, 0.7); border-radius : 10px;");

    //

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(13, 13, 13, 13);
    layout->setSpacing(6);
    setLayout(layout);

    //

    labelIcon = new QLabel(this);
    labelIcon->setFixedSize(QSize(24, 24));
    
    std::string absPath;
    GetDataFilePath("assets", absPath);
    QString iconName;
    switch (type) {
    case Types::time: iconName = "broadtime"; break;
    case Types::gift: iconName = "giftcount"; break;
    case Types::user: iconName = "usercount"; break;
    case Types::up:   iconName = "upcount"; break;
    default: break;
    }
    QString iconPath = QString("%1/Popup/overlay/icon_%2.svg")
        .arg(QString::fromStdString(absPath), iconName);
    SetSvgToLabel(labelIcon, iconPath, QSize(24, 24));

    labelIcon->setAttribute(Qt::WA_TranslucentBackground, true);
    labelIcon->setAttribute(Qt::WA_NoSystemBackground, true);
    labelIcon->setStyleSheet("background-color : transparent; border-radius : 0px;");
    layout->addWidget(labelIcon);

    //

    labelText = new QLabel(this);
    if (editable == true)
    {
        if (type == Types::time)
            labelText->setText(QTStr("Popup.LiveOverlay.BroadTime"));
        else if (type == Types::gift)
            labelText->setText(QTStr("Popup.LiveOverlay.GiftCount"));
        else if (type == Types::user)
            labelText->setText(QTStr("Popup.LiveOverlay.UserCount"));
        else if (type == Types::up)
            labelText->setText(QTStr("Popup.LiveOverlay.UpCount"));
    }
    else
        labelText->setText("");

    labelText->setAttribute(Qt::WA_TranslucentBackground, true);
    labelText->setAttribute(Qt::WA_NoSystemBackground, true);
    if (editable == true || type != Types::time)
        labelText->setStyleSheet("background-color : transparent; border-radius : 0px; font-size : 14px; color : #D5D7DC;");
    else
        labelText->setStyleSheet("background-color : transparent; border-radius : 0px; font-size : 14px; color : #FF0051;");
    layout->addWidget(labelText);
    
    //

    if (editable == false)
    {
        do
        {
            _Timer();

            auto timer = new QTimer(this);
            connect(timer, &QTimer::timeout, this, &OverlayLabelSourceWidget::_Timer);
            if (timer == nullptr)
            {
                blog(LOG_ERROR, "%s (%d) : timer : %p", __FILE__, __LINE__, timer);
                break;
            }

            timer->start(1000);
        } while (false);
    }
}

OverlayLabelSourceWidget::~OverlayLabelSourceWidget()
{
    auto y = (double)posRelative().y();
    auto x = (double)posRelative().x();

    if (type == Types::up)
    {    
        config_set_double(ACTIVECONFIG, "Overlay", "Up.Y",y);
        config_set_double(ACTIVECONFIG, "Overlay", "Up.X", x);
    }
    else if (type == Types::user)
    {
        config_set_double(ACTIVECONFIG, "Overlay", "User.Y", y);
        config_set_double(ACTIVECONFIG, "Overlay", "User.X", x);
    }
    else if (type == Types::gift)
    {
        config_set_double(ACTIVECONFIG, "Overlay", "Gift.Y", y);
        config_set_double(ACTIVECONFIG, "Overlay", "Gift.X", x);
    }
    else if (type == Types::time)
    {
        config_set_double(ACTIVECONFIG, "Overlay", "Time.Y", y);
        config_set_double(ACTIVECONFIG, "Overlay", "Time.X", x);
    }
}

void OverlayLabelSourceWidget::Opacity(int opacity) {
    do
    {
        auto effect = static_cast<QGraphicsOpacityEffect*>(graphicsEffect());
        if (effect == nullptr)
        {
            effect = new QGraphicsOpacityEffect(this);
            if (effect == nullptr)
            {
                blog(LOG_ERROR, "%s (%d) : effect : %p", __FILE__, __LINE__, effect);
                break;
            }
        }

        effect->setOpacity((qreal)(100 - opacity) / (qreal)100);
        setGraphicsEffect(effect);
    } while (false);
}

void OverlayLabelSourceWidget::paintEvent(QPaintEvent* event)
{
    if (editable == true)
    {
        Q_UNUSED(event);

        QStyleOption opt;
        opt.initFrom(this);

        QPainter painter(this);

        style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

        QPen pen;
        pen.setWidth(2);
        pen.setStyle(Qt::DashLine);
        pen.setDashPattern({ 2, 2 });
        pen.setColor(QColor(255, 255, 255, 127)); // RGBA: alpha 127 = 0.5
        pen.setCapStyle(Qt::FlatCap);

        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        QRect r = rect().adjusted(1, 1, -1, -1);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.drawRoundedRect(r, 10, 10); // border-radius: 10px
    }
}

void OverlayLabelSourceWidget::_Timer()
{
    do
    {
        if (labelText == nullptr)
        {
            blog(LOG_ERROR, "%s (%d) : labelText : %p", __FILE__, __LINE__, labelText);
            break;
        }

        QString str;

        if (type == Types::time)
            str = MAINFRAME->GetBroadTimerUITime();
        else if (type == Types::gift)
        {
            auto broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
            if (broadInfo == nullptr)
            {
                blog(LOG_ERROR, "%s (%d) : broadInfo : %p", __FILE__, __LINE__, broadInfo);
                break;
            }

            if (count == 0)
            {
                broadInfo->ReceiveGift();
                count = 30;
            }
            else
                --count;

            str = broadInfo->Gift();
        }
        else if (type == Types::user)
        {
            QLocale locale = QLocale(QLocale::Korean);

            auto broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
            if (broadInfo == nullptr) 
            {
                blog(LOG_ERROR, "%s (%d) : broadInfo : %p", __FILE__, __LINE__, broadInfo);
                break;
            }

            int totalViewer = broadInfo->CurrentViewer() + broadInfo->RelayViewer();
            str = QString("%1").arg(locale.toString(totalViewer));
        }
        else if (type == Types::up)
        {
            QLocale locale = QLocale(QLocale::Korean);

            auto broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
            if (broadInfo == nullptr)
            {
                blog(LOG_ERROR, "%s (%d) : broadInfo : %p", __FILE__, __LINE__, broadInfo);
                break;
            }
             
            if (count == 0)
            {
                broadInfo->ReceiveUp();
                count = 30;
            }
            else
                --count;

            str = QString("%1").arg(locale.toString(broadInfo->UpTotal()));
        }

        labelText->setText(str);
    } while (false);
}
