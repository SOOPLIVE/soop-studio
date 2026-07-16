#include "COverlayCefSourceWidget.h"

#include <qgraphicseffect.h>

#include <util/base.h>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#include "Common/StudioDefine.h"

#include "Utils/OverlayManager.h"

#include "MainFrame/CMainFrame.h"

OverlayCefSourceWidget::OverlayCefSourceWidget(QWidget* parent, bool editable, Types type) : OverlaySourceWidget(parent), editable(editable), type(type)
{
    installEventFilter(parent); 
    raise();

    double x = (double)0;
    double y = (double)0;
    int w = 0;
    int h = 0;

    if (type == Types::chat)
    {
        config_set_default_double(ACTIVECONFIG, "Overlay", "Chat.X", (double)-1);
        x = config_get_double(ACTIVECONFIG, "Overlay", "Chat.X");
        if (x < 0)
        {
            auto width = OVERLAY_MANAGER.TargetRect(this).width();
            x = (double)(width - min_w - 8);
            x /= (double)width;
        }

        config_set_default_double(ACTIVECONFIG, "Overlay", "Chat.Y", (double)-1);
        y = config_get_double(ACTIVECONFIG, "Overlay", "Chat.Y");
        if (y < 0)
        {
            y = (double)8;
            auto height = OVERLAY_MANAGER.TargetRect(this).height();
            y /= (double)height;
        }

        config_set_default_int(ACTIVECONFIG, "Overlay", "Chat.W", (int64_t)-1);
        w = (int)config_get_int(ACTIVECONFIG, "Overlay", "Chat.W");
        if (w < 0)
            w = min_w;

        config_set_default_int(ACTIVECONFIG, "Overlay", "Chat.H", (int64_t)-1);
        h = (int)config_get_int(ACTIVECONFIG, "Overlay", "Chat.H");
        if (h < 0)
        {
            auto height = OVERLAY_MANAGER.TargetRect(this).height();
            h = height * 0.7;
        }

        setMinimumSize(min_w, min_h);
    }

    moveRelative(QPointF((qreal)x, (qreal)y));

    resize(w, h);

    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    if (editable == true)
        setStyleSheet("background-color : rgba(0, 0, 0, 0.7); border-radius : 10px;");

    //

    if (editable == true)
    {
        QHBoxLayout* layout = new QHBoxLayout(this);
        setLayout(layout);

        //

        labelIcon = new QLabel(this);
        labelIcon->setFixedSize(QSize(24, 24));
        
        std::string absPath;
        GetDataFilePath("assets", absPath);
        QString iconPath;
        if (type == Types::chat)
            iconPath = QString("%1/Popup/overlay/icon_%2.svg").arg(absPath.data()).arg("chat");
        SetSvgToLabel(labelIcon, iconPath, QSize(24, 24));

        labelIcon->setAttribute(Qt::WA_TranslucentBackground, true);
        labelIcon->setAttribute(Qt::WA_NoSystemBackground, true);
        labelIcon->setStyleSheet("background-color : transparent; border-radius : 0px;");

        //
        
        labelText = new QLabel(this);
        if (type == Types::chat)
            labelText->setText(QTStr("Popup.LiveOverlay.Chat"));
        
        labelText->setAttribute(Qt::WA_TranslucentBackground, true);
        labelText->setAttribute(Qt::WA_NoSystemBackground, true);
        labelText->setStyleSheet("background-color : transparent; border-radius : 0px; font-size : 14px; color : #D5D7DC;");
        
        //

        layout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Preferred));
        layout->addWidget(labelIcon);
        layout->addWidget(labelText);
        layout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Preferred));
    }
    else
    {
        std::string url = "";

        if (type == Types::chat)
        {
            url = SOOP_AQUA_URL;

            OBSSourceAutoRelease source = obs_source_create_private("soop_chat_source_chat", "overlay_dowoomi_chat", nullptr);
            auto* props = obs_source_properties(source);
            auto* prop = obs_properties_get(props, "style_setting");
#if 0
            auto count = obs_property_list_item_count(prop);
            for (auto i = 0; i < count; i++)
            {
                auto name = obs_property_list_item_name(prop, i);
                auto str = obs_property_list_item_string(prop, i);
                blog(LOG_ERROR, "name : %d, str : %s", name, str);
            }
#endif // 0
            auto key = obs_property_list_item_string(prop, 0);
            if (key != nullptr)
                url += key;
        }

        _URL(url.c_str());
    }
}

OverlayCefSourceWidget::~OverlayCefSourceWidget()
{
    auto h = (int64_t)size().height();
    auto w = (int64_t)size().width();
    auto y = (double)posRelative().y();
    auto x = (double)posRelative().x();

    if (type == Types::chat)
    {
        config_set_int(ACTIVECONFIG, "Overlay", "Chat.H", h);
        config_set_int(ACTIVECONFIG, "Overlay", "Chat.W", w);
        config_set_double(ACTIVECONFIG, "Overlay", "Chat.Y", y);
        config_set_double(ACTIVECONFIG, "Overlay", "Chat.X", x);
    }
}

void OverlayCefSourceWidget::Opacity(int opacity) {
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

void OverlayCefSourceWidget::paintEvent(QPaintEvent* event)
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
        pen.setColor(QColor(255, 255, 255, 127));
        pen.setCapStyle(Qt::FlatCap);

        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        QRect r = rect().adjusted(1, 1, -1, -1);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.drawRoundedRect(r, 10, 10); // border-radius: 10px
    }
}

void OverlayCefSourceWidget::_URL(const char* url)
{
    do
    {
        auto panel_cookies = CEFMANAGER.GetCefCookieManager();
        widget = CEFMANAGER.createWidget(this, "", panel_cookies, "", false);
        if (widget == nullptr)
        {
            blog(LOG_ERROR, "%s (%d) : widget : %p", __FILE__, __LINE__, widget);
            break;
        }

        widget->resize(size());
        widget->setURL(url, 0);
    } while (false);
}

