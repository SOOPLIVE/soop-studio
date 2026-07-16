#pragma once

#include "CSceneBottomButton.h"

#include <QStyle>
#include <QTimer>
#include <QBoxLayout>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QScreen>

#include "qt-wrappers.hpp"


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

    QString sceneIndex = QString("%1").arg(index + 1);

    m_plabelSceneIndex = new QLabel(this);
    m_plabelSceneIndex->setFixedHeight(20);
    m_plabelSceneIndex->setText(sceneIndex);
    m_plabelSceneIndex->setObjectName("sceneButtonIndex");

    m_pLabelSceneName = new AFQElidedSlideLabel(this);
    m_pLabelSceneName->setObjectName("sceneButtonName");
    m_pLabelSceneName->setMaximumWidth(106);
    m_pLabelSceneName->setFixedHeight(20);

    m_pLabelSceneName->setText(name);
    m_labelNameWidth = m_pLabelSceneName->width();

    connect(this, &AFQSceneBottomButton::qsignalHoverButton,
            m_pLabelSceneName, &AFQElidedSlideLabel::qslotHoverButton);

    connect(this, &AFQSceneBottomButton::qsignalLeaveButton,
            m_pLabelSceneName, &AFQElidedSlideLabel::qslotLeaveButton);

    QVBoxLayout* layoutMiddleLine = new QVBoxLayout();
    layoutMiddleLine->setContentsMargins(0, 3, 0, 0);

    m_pFrameMiddleLine = new QFrame(this);
    m_pFrameMiddleLine->setObjectName("frameMiddleLine");
    m_pFrameMiddleLine->setFixedSize(QSize(1, 10));

    layoutMiddleLine->addWidget(m_pFrameMiddleLine, Qt::AlignVCenter);


    hLayout->addWidget(m_plabelSceneIndex);
    hLayout->addLayout(layoutMiddleLine);
    hLayout->addWidget(m_pLabelSceneName);

    this->setLayout(hLayout);

    m_pTimerHoverPreview = new QTimer(this);
    connect(m_pTimerHoverPreview, &QTimer::timeout,
            this, &AFQSceneBottomButton::qslotTimerHoverPreview);

}

AFQSceneBottomButton::~AFQSceneBottomButton()
{
    if (m_sceneListPreviewWidget) {
        m_sceneListPreviewWidget->close();
        m_sceneListPreviewWidget = nullptr;
    }
    m_obsScene = nullptr;
}

void AFQSceneBottomButton::SetSelectedState(bool selected)
{
    if (selected) {
        setProperty("sceneBtnType", "selected");
        m_plabelSceneIndex->setProperty("selected", true);
        m_pFrameMiddleLine->setProperty("selected", true);
        m_pLabelSceneName->setProperty("selected", true);
    }
    else {
        setProperty("sceneBtnType", "");
        m_plabelSceneIndex->setProperty("selected", false);
        m_pFrameMiddleLine->setProperty("selected", false);
        m_pLabelSceneName->setProperty("selected", false);
    }

    PolishStyleSheet(m_plabelSceneIndex);
    PolishStyleSheet(m_pFrameMiddleLine);
    PolishStyleSheet(m_pLabelSceneName);
    PolishStyleSheet(this);

    m_selected = selected;

}

void AFQSceneBottomButton::_ShowPreveiw(bool on)
{
    if (on) {
        m_pTimerHoverPreview->start(300);
    }

    if (!on) {
        m_pTimerHoverPreview->stop();
        if (m_sceneListPreviewWidget) {
            m_sceneListPreviewWidget->close();
            m_sceneListPreviewWidget = nullptr;
        }
    }
}

void AFQSceneBottomButton::qslotTimerHoverPreview()
{
    m_pTimerHoverPreview->stop();

    m_sceneListPreviewWidget = new AFQSceneListPreview(nullptr, obs_scene_get_source(m_obsScene));

    QPoint pos = QCursor::pos();

    QRect pointInScreenRect;
    QList<QScreen*> screens = QGuiApplication::screens();
    for (QScreen* screen : screens) {
        QRect rcScreen = screen->geometry();
        if (rcScreen.contains(pos)) {
            pointInScreenRect = rcScreen;
            break;
        }
    }

    int x = pos.x();
    int y = pos.y();

    if (pos.x() + m_sceneListPreviewWidget->width() > pointInScreenRect.x() + pointInScreenRect.width())
        x = pos.x() - m_sceneListPreviewWidget->width() - 10;
    else
        x = pos.x() + 10;

    if (pos.y() + m_sceneListPreviewWidget->height() > pointInScreenRect.y() + pointInScreenRect.height())
        y = pos.y() - m_sceneListPreviewWidget->height() - 10;
    else
        y = pos.y() + 10;

    pos.setX(x);
    pos.setY(y);

    m_sceneListPreviewWidget->move(pos);
    m_sceneListPreviewWidget->show();
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

    if(!m_selected)
        _ShowPreveiw(true);

    QFrame::enterEvent(event);
}

void AFQSceneBottomButton::leaveEvent(QEvent* event)
{
    emit qsignalLeaveButton();

    _ShowPreveiw(false);

    QFrame::leaveEvent(event);
}
