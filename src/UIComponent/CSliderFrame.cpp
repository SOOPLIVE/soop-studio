#include "CSliderFrame.h"
#include "ui_slider-frame.h"

#include <QStyleOption>
#include <QPainter>
#include <QGraphicsDropShadowEffect>

#include "platform/platform.hpp"

AFQSysVolumeSlider::AFQSysVolumeSlider(QWidget* parent) :
    AFQMouseClickSlider(parent)
{
    m_qTimer = new QTimer(this);
    connect(m_qTimer, &QTimer::timeout, this, &AFQSysVolumeSlider::qslotUpdate);
}

void AFQSysVolumeSlider::paintEvent(QPaintEvent* event)
{
    QSlider::paintEvent(event);
    
    if (m_bMuted)
        m_fCurrentPeak = m_fMinPeak;

    QStyleOptionSlider opt;
    initStyleOption(&opt);
    QColor drawColor = QColor(0, 224, 255);

    opt.subControls = QStyle::SC_SliderGroove | QStyle::SC_SliderHandle;
    if (tickPosition() != NoTicks) 
    {
        opt.subControls |= QStyle::SC_SliderTickmarks;
    }

    QRect grooveRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
    QRect handleRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

    float fRatio = (m_fCurrentPeak - m_fMinPeak) / (m_fMaxPeak - m_fMinPeak);
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
    m_qTimer->start(50);
}

void AFQSysVolumeSlider::hideEvent(QHideEvent* event) 
{
    m_qTimer->stop();
}

void AFQSysVolumeSlider::qslotUpdate()
{
    repaint();
}

void AFQSysVolumeSlider::SetCurrentPeak(float curPeak)
{
    if (curPeak < m_fMinPeak)
        m_fCurrentPeak = m_fMinPeak;
    else if (curPeak > m_fMaxPeak)
        m_fCurrentPeak = m_fMaxPeak;
    else
        m_fCurrentPeak = curPeak;
}

void AFQSysVolumeSlider::SetMuted(bool muted)
{
    m_bMuted = muted;

    if (muted)
        this->setStyleSheet("QSlider::add-page{ background: rgba(255, 255, 255, 0.40); }");
    else
        this->setStyleSheet("QSlider::add-page{ background: rgba(252, 252, 253, 100%); }");
}

AFQSliderFrame::AFQSliderFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::AFQSliderFrame)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    installEventFilter(this);

    _ApplyShadowEffect();
}

AFQSliderFrame::~AFQSliderFrame()
{
    delete ui;
}

void AFQSliderFrame::InitSliderFrame(const char* imagepath, bool buttonchecked, int sliderTotal, int volume)
{
    std::string absPath;
    GetDataFilePath(imagepath, absPath);
    QString qstrImgPath(absPath.data());
    
    QString style = "QPushButton {image: url(%1) 0 0 0 0 stretch stretch;}";
    //ui->pushButton->setStyleSheet(style.arg(qstrImgPath));
    ui->pushButton->setChecked(buttonchecked);
    ui->verticalSlider->setValue(volume);
    ui->verticalSlider->setMaximum(sliderTotal);

    connect(ui->verticalSlider, &QSlider::valueChanged, this, &AFQSliderFrame::qsignalVolumeChanged);
    connect(ui->pushButton, &QPushButton::clicked, this, &AFQSliderFrame::qsignalMuteButtonClicked);
}

int AFQSliderFrame::VolumeSize()
{
    return ui->verticalSlider->value();
}

void AFQSliderFrame::SetButtonProperty(const char* property)
{
    ui->pushButton->setProperty("AudioSliderBtnType", property);
    style()->unpolish(ui->pushButton);
    style()->polish(ui->pushButton);
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

void AFQSliderFrame::_ApplyShadowEffect()
{
    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
    effect->setXOffset(2);
    effect->setYOffset(2);
    effect->setBlurRadius(10);
    effect->setColor(QColor(0, 0, 0, 110));

    setGraphicsEffect(effect);
}