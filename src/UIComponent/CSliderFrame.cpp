#include "CSliderFrame.h"
#include "ui_slider-frame.h"

#include <QStyleOption>
#include <QPainter>
#include <QGraphicsDropShadowEffect>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#define HANDLE_STATE_PROPERTY "sliderState"
#define HANDLE_STATE_PROPERTY_DEFAULT "default"
#define HANDLE_STATE_PROPERTY_HOVER "hover"
#define HANDLE_STATE_PROPERTY_PRESSED "pressed"

AFQSysVolumeSlider::AFQSysVolumeSlider(QWidget* parent) :
    AFQMouseClickSlider(parent)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &AFQSysVolumeSlider::qslotUpdate);
}

void AFQSysVolumeSlider::paintEvent(QPaintEvent* event)
{
    QSlider::paintEvent(event);

    if (m_muted)
        m_currentPeak = m_minPeak;

    QStyleOptionSlider opt;
    initStyleOption(&opt);

    //QColor drawColor = palette().light().color();
    QColor drawColor = QColor(1, 130, 255);

    opt.subControls = QStyle::SC_SliderGroove | QStyle::SC_SliderHandle;
    if (tickPosition() != NoTicks)
    {
        opt.subControls |= QStyle::SC_SliderTickmarks;
    }

    QRect grooveRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
    QRect handleRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

    float fRatio = (m_currentPeak - m_minPeak) / (m_maxPeak - m_minPeak);
    fRatio *= -1.f;

    int drawRectHeight = grooveRect.height() * fRatio;

    // Prevent drawing above the handle
    if (grooveRect.bottom() + drawRectHeight < handleRect.y() + handleRect.height())
        drawRectHeight = (handleRect.y() + handleRect.height()) - grooveRect.bottom();

    QRect drawRect(grooveRect.left(), grooveRect.bottom() + 1, grooveRect.width(), drawRectHeight);
    QPainter painter(this);
    painter.fillRect(drawRect, drawColor);
}

void AFQSysVolumeSlider::showEvent(QShowEvent* event)
{
    m_timer->start(50);
}

void AFQSysVolumeSlider::hideEvent(QHideEvent* event) 
{
    m_timer->stop();
}

void AFQSysVolumeSlider::qslotUpdate()
{
    repaint();
}

void AFQSysVolumeSlider::SetCurrentPeak(float curPeak)
{
    if (curPeak < m_minPeak)
        m_currentPeak = m_minPeak;
    else if (curPeak > m_maxPeak)
        m_currentPeak = m_maxPeak;
    else
        m_currentPeak = curPeak;
}

void AFQSysVolumeSlider::SetMuted(bool muted)
{
    m_muted = muted;

    if (muted)
        this->setProperty("Mute", true);
    else
        this->setProperty("Mute", false);

    PolishStyleSheet(this);
}

AFQSliderFrame::AFQSliderFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::AFQSliderFrame)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    //setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    installEventFilter(this);

    ui->verticalSlider->setAttribute(Qt::WA_Hover, true);
    ui->verticalSlider->installEventFilter(this);
    _SetSliderStateProperty(HANDLE_STATE_PROPERTY_DEFAULT);

    //_ApplyShadowEffect();
}

AFQSliderFrame::~AFQSliderFrame()
{
    delete ui;
}

void AFQSliderFrame::qslotSliderValueChanged(int sliderValue)
{
    auto normalize = (float)(sliderValue - ui->verticalSlider->maximum()) / (float)ui->verticalSlider->maximum(); // -1 ~ 0
    emit qsignalVolumeChanged(normalize);
}

void AFQSliderFrame::InitSliderFrame(const char* /*imagepath*/, bool buttonchecked, int sliderTotal, int volume)
{
    ui->pushButton->setChecked(buttonchecked);
    ui->verticalSlider->setMaximum(sliderTotal);

    if (volume > sliderTotal)
        volume = sliderTotal;
    else if (volume < 0)
        volume = 0;

    ui->verticalSlider->setValue(volume);

    connect(ui->verticalSlider, &QSlider::valueChanged, this, &AFQSliderFrame::qslotSliderValueChanged);
    connect(ui->pushButton, &QPushButton::clicked, this, &AFQSliderFrame::qsignalMuteButtonClicked);
}

int AFQSliderFrame::VolumeSize()
{
    return ui->verticalSlider->value();
}

void AFQSliderFrame::SetButtonProperty(const char* property)
{
    ui->pushButton->setProperty("AudioSliderBtnType", property);

    PolishStyleSheet(ui->pushButton);
}

void AFQSliderFrame::SetVolumeSize(int volume)
{
    ui->verticalSlider->setValue(volume);
}

void AFQSliderFrame::SetVolumePeak(float peak)
{
    ui->verticalSlider->SetCurrentPeak(peak);
}

void AFQSliderFrame::SetVolumeMuted(bool muted)
{
    ui->verticalSlider->SetMuted(muted);
}

void AFQSliderFrame::SetVolumeSliderEnabled(bool enabled) 
{
    ui->verticalSlider->setEnabled(enabled);
}

bool AFQSliderFrame::ButtonIsChecked()
{
    return ui->pushButton->isChecked();
}

void AFQSliderFrame::BlockSliderSignal(bool block)
{
    ui->verticalSlider->blockSignals(block);
}

bool AFQSliderFrame::IsVolumeMuted()
{
    return ui->verticalSlider->IsMuted();
}

bool AFQSliderFrame::event(QEvent* e)
{
    switch (e->type())
    {
    case QEvent::HoverLeave:
        emit qsignalMouseLeave();
        break; 
    case QEvent::HoverEnter:
        emit qsignalMouseEnterSlider();
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

bool AFQSliderFrame::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui->verticalSlider) {
        if (event->type() == QEvent::HoverEnter) {
            _SetSliderStateProperty(HANDLE_STATE_PROPERTY_HOVER);
        }
        else if (event->type() == QEvent::HoverLeave) {
            _SetSliderStateProperty(HANDLE_STATE_PROPERTY_DEFAULT);
        }
        else if (event->type() == QEvent::MouseButtonPress) {
            _SetSliderStateProperty(HANDLE_STATE_PROPERTY_PRESSED);
        }
        else if (event->type() == QEvent::MouseButtonRelease) {
            _SetSliderStateProperty(HANDLE_STATE_PROPERTY_HOVER);
        }
    }
    return QWidget::eventFilter(obj, event);
}

void AFQSliderFrame::_SetSliderStateProperty(QString state)
{
    if (ui->verticalSlider->property(HANDLE_STATE_PROPERTY).toString() == state)
        return;

    ui->verticalSlider->setProperty(HANDLE_STATE_PROPERTY, state);
    PolishStyleSheet(ui->verticalSlider);
}

void AFQSliderFrame::_ApplyShadowEffect()
{
    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
    effect->setXOffset(2);
    effect->setYOffset(2);
    effect->setBlurRadius(10);
    effect->setColor(QColor(0, 0, 0, 110));

    setGraphicsEffect(effect);
}
