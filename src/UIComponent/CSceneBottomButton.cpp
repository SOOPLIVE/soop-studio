#pragma once

#include "CSceneBottomButton.h"

#include <QStyle>
#include <QTimer>
#include <QBoxLayout>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QScreen>

AFQSceneBottomButton::AFQSceneBottomButton(QWidget* parent, OBSScene scene,
    int index, QString name) :
    QFrame(parent),
    m_obsScene(scene)
{
    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->setSpacing(5);
    hLayout->setContentsMargins(10, 4, 10, 5);
    hLayout->setAlignment(Qt::AlignVCenter);

    setFixedHeight(30);

    QString sceneIndex = QString("Scene %1").arg(index + 1);

    m_labelSceneIndex = new QLabel(this);
    m_labelSceneIndex->setFixedHeight(20);
    m_labelSceneIndex->setText(sceneIndex);
    m_labelSceneIndex->setObjectName("sceneButtonIndex");
    m_labelSceneIndex->setStyleSheet("color : rgba(255, 255, 255, 70%); \
                                      font-size : 14px; font-style: normal; font-weight: 400; line-height: normal;  \
                                      ");

    m_labelSceneName = new AFQElidedSlideLabel(this);
    m_labelSceneName->setMaximumWidth(106);
    m_labelSceneName->setFixedHeight(20);
    m_labelSceneName->setObjectName("sceneButtonName");
    m_labelSceneName->setStyleSheet("color : rgba(255, 255, 255, 70%); \
                                     font-size: 14px; font-style: normal; font-weight: 400; line-height: normal;");

    m_labelSceneName->setText(name);
    m_labelNameWidth = m_labelSceneName->width();

    connect(this, &AFQSceneBottomButton::qsignalHoverButton,
            m_labelSceneName, &AFQElidedSlideLabel::qSlotHoverButton);

    connect(this, &AFQSceneBottomButton::qsignalLeaveButton,
            m_labelSceneName, &AFQElidedSlideLabel::qSlotLeaveButton);

    QVBoxLayout* layoutMiddleLine = new QVBoxLayout();
    layoutMiddleLine->setContentsMargins(0, 3, 0, 0);

    m_frameMiddleLine = new QFrame(this);
    m_frameMiddleLine->setFixedSize(QSize(1, 10));
    m_frameMiddleLine->setStyleSheet("QFrame { background-color : rgba(255, 255, 255, 10%);}");

    layoutMiddleLine->addWidget(m_frameMiddleLine, Qt::AlignVCenter);

    m_pScreenshotScene = new QLabel(this);
    m_pScreenshotScene->setWindowFlags(Qt::ToolTip);
    m_pScreenshotScene->setStyleSheet("QLabel { border: 1px solid #00E0FF; }");
    m_pScreenshotScene->setFixedSize(208, 117);
    m_pScreenshotScene->hide();

    hLayout->addWidget(m_labelSceneIndex);
    hLayout->addLayout(layoutMiddleLine);
    hLayout->addWidget(m_labelSceneName);

    this->setLayout(hLayout);

    m_timerScreenShot = new QTimer(this);
    connect(m_timerScreenShot, &QTimer::timeout,
            this, &AFQSceneBottomButton::qslotTimerScreenShot);

    m_timerHoverPreview = new QTimer(this);
    connect(m_timerHoverPreview, &QTimer::timeout,
            this, &AFQSceneBottomButton::qslotTimerHoverPreview);

}

AFQSceneBottomButton::~AFQSceneBottomButton()
{
    m_obsScene = nullptr;
    delete m_pScreenshotObj;
    m_pScreenshotObj = nullptr;
}

void AFQSceneBottomButton::SetSelectedState(bool selected)
{
    if (selected) {
        setProperty("sceneBtnType", "selected");
        m_labelSceneIndex->setStyleSheet("QLabel {					\
										   color : #00E0FF; }");


        m_frameMiddleLine->setStyleSheet("QFrame { \
                                          background-color : #166C7B;}");

        m_labelSceneName->setStyleSheet("QLabel {			\
										   color : #00E0FF;}");
    }
    else {
        setProperty("sceneBtnType", "");
        m_labelSceneIndex->setStyleSheet("QLabel {								\
										   color : rgba(255, 255, 255, 70%);}");

        m_frameMiddleLine->setStyleSheet("QFrame { \
                                          background-color : rgba(255, 255, 255, 10%);}");

        m_labelSceneName->setStyleSheet("QLabel {			\
										   color : rgba(255, 255, 255, 70%);}");
    }

    style()->unpolish(this);
    style()->polish(this);

    m_bSelected = selected;

}

void AFQSceneBottomButton::_ShowPreveiw(bool on)
{
    if (!m_pScreenshotScene || !m_timerScreenShot)
        return;

    if (on /*&& !m_pScreenshotScene->isVisible()*/) {
        if (!m_pScreenshotScene->isVisible()) {
            if (!m_timerScreenShot->isActive()) {
                qslotTimerScreenShot();

                m_timerHoverPreview->start(300);
                m_timerScreenShot->start(1000);
            }
        }
    }

    if (!on /*&& m_pScreenshotScene->isVisible()*/) {
        m_timerScreenShot->stop();
        m_timerHoverPreview->stop();
        m_pScreenshotScene->hide();

        if (m_pScreenshotScene)
            m_pScreenshotScene->setPixmap(QPixmap());
    }
}

void AFQSceneBottomButton::qslotTimerScreenShot()
{
    obs_source_t* source = obs_scene_get_source(m_obsScene);
    delete m_pScreenshotObj;
    m_pScreenshotObj = new AFQScreenShotObj(source,
                                            AFQScreenShotObj::Type::Screenshot_SceneButton);

    connect(m_pScreenshotObj, &AFQScreenShotObj::qsignalSetPreview,
            this, &AFQSceneBottomButton::qslotSetScreenShotPreview);

}

void AFQSceneBottomButton::qslotTimerHoverPreview()
{
    m_timerHoverPreview->stop();

    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
    QPoint pos = QCursor::pos();

    int x = pos.x();
    int y = pos.y();

    if (pos.x() + m_pScreenshotScene->width() > screenRect.width())
        x = pos.x() - m_pScreenshotScene->width() - 10;
    else
        x = pos.x() + 10;

    if (pos.y() + m_pScreenshotScene->height() > screenRect.height())
        y = pos.y() - m_pScreenshotScene->height() - 10;
    else
        y = pos.y() + 10;

    pos.setX(x);
    pos.setY(y);

    m_pScreenshotScene->move(pos);
    m_pScreenshotScene->show();
}


void AFQSceneBottomButton::qslotSetScreenShotPreview()
{
    QPixmap pixmap = m_pScreenshotObj->GetPixmap();
    m_pScreenshotScene->setPixmap(pixmap.scaled(208, 117));
}

void AFQSceneBottomButton::mousePressEvent(QMouseEvent* event)
{
    emit qsignalSceneButtonClicked(m_obsScene);

    _ShowPreveiw(false);

    QFrame::mousePressEvent(event);
}

void AFQSceneBottomButton::mouseDoubleClickEvent(QMouseEvent* event)
{
    emit qsignalSceneButtonDoubleClicked(m_obsScene);

    QFrame::mouseDoubleClickEvent(event);
}

void AFQSceneBottomButton::enterEvent(QEnterEvent* event)
{
    emit qsignalHoverButton(QString());

    if(!m_bSelected)
        _ShowPreveiw(true);

    QFrame::enterEvent(event);
}

void AFQSceneBottomButton::leaveEvent(QEvent* event)
{
    emit qsignalLeaveButton();

    _ShowPreveiw(false);

    QFrame::leaveEvent(event);
}