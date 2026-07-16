
#include "COverlaySceneWidget.h"

#include <util/base.h>

#include <Windows.h>

#include "Utils/OverlayManager.h"

#include "MainFrame/CMainFrame.h"

OverlaySceneWidget::OverlaySceneWidget(QWidget* parent, bool editable) : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint) 
{
    setMinimumSize(min_w, min_h);

    //

    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, editable == true ? false : true);
    if (editable == true)
        setObjectName("overlaySceneWidget");

    setAttribute(Qt::WA_TransparentForMouseEvents, editable == true ? false : true);
    setFocusPolicy(editable == true ? Qt::StrongFocus : Qt::NoFocus);

    auto overlay = reinterpret_cast<HWND>(winId());
    auto exStyle = GetWindowLong(overlay, GWL_EXSTYLE);
    if (editable == true)
        exStyle &= ~(WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
    else
        exStyle |= (WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
    SetWindowLong(overlay, GWL_EXSTYLE, exStyle);

    //

    setting = new OverlaySettingSourceWidget(this);
    if (editable == false)
    {
        do
        {
            if (setting == nullptr)
            {
                blog(LOG_ERROR, "%s (%d) : setting : %p", __FILE__, __LINE__, setting);
                break;
            }

            setting->hide();
        } while (false);
    }
}

void OverlaySceneWidget::Chat(bool editable)
{
    _Widget(chat, editable, OverlayCefSourceWidget::Types::chat);
}

void OverlaySceneWidget::Time(bool editable)
{
    _Widget(time, editable, OverlayLabelSourceWidget::Types::time);
}

void OverlaySceneWidget::Gift(bool editable)
{
    _Widget(gift, editable, OverlayLabelSourceWidget::Types::gift);
}

void OverlaySceneWidget::User(bool editable)
{
    _Widget(user, editable, OverlayLabelSourceWidget::Types::user);
}

void OverlaySceneWidget::Up(bool editable)
{
    _Widget(up, editable, OverlayLabelSourceWidget::Types::up);
}

void OverlaySceneWidget::Opacity(int value)
{
    do
    {
        if (setting == nullptr)
        {
            blog(LOG_ERROR, "%s (%d) : setting : %p", __FILE__, __LINE__, setting);
            break;
        }

        setting->Opacity(value);
    } while (false);

    if (chat != nullptr)
        chat->Opacity(value);

    if (time != nullptr)
        time->Opacity(value);

    if (gift != nullptr)
        gift->Opacity(value);

    if (user != nullptr)
        user->Opacity(value);

    if (up != nullptr)
        up->Opacity(value);
}

bool OverlaySceneWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::ZOrderChange)
    {
        do
        {
            if (setting == nullptr)
            {
                blog(LOG_ERROR, "%s (%d) : setting : %p", __FILE__, __LINE__, setting);
                break;
            }

            setting->raise();
        } while (false);
    }

    return QWidget::eventFilter(watched, event);
}

void OverlaySceneWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape)
        OVERLAY_MANAGER.Editable(false);
    else
        QWidget::keyPressEvent(event);
}

template<typename T>
void OverlaySceneWidget::_Widget(T*& widget, bool editable, typename T::Types type)
{
    do
    {
        widget = new T(this, editable, type);
        if (widget == nullptr)
        {
            blog(LOG_ERROR, "%s (%d) : widget : %p", __FILE__, __LINE__, widget);
            break;
        }

        if (setting == nullptr)
        {
            blog(LOG_ERROR, "%s (%d) : setting : %p", __FILE__, __LINE__, setting);
            break;
        }

        auto opacity = setting->Opacity();
        widget->Opacity(opacity);
    } while (false);
}
