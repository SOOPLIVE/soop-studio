#include "CMainFrameGuide.h"
#include "ui_main-frame-guide.h"

#include <QWindow>

#include "QPropertyAnimation"

#include "CoreModel/Locale/CLocaleTextManager.h"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/CMustRaiseMainFrameEventF.h"
#include "platform/platform.hpp"   //[copy-obs] copied

AFMainFrameGuide::AFMainFrameGuide(QWidget *parent) :
    AFQHoverWidget(parent),
    ui(new Ui::AFMainFrameGuide)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    
    m_pMainRaiseEventFilter = new AFQMustRaiseMainFrameEventFilter();
    connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFMainFrameGuide::qsignalCloseGuide);
}

AFMainFrameGuide::~AFMainFrameGuide()
{
    delete m_pMainRaiseEventFilter;
    delete ui;
}

void AFMainFrameGuide::qslotLoginTriggered()
{
    emit qsignalLoginTrigger();
    emit qsignalMissionCleared(1);
    close();
}

void AFMainFrameGuide::qslotSceneSourceTriggered()
{
    emit qsignalSceneSourceTriggered(true, 0);
    emit qsignalMissionCleared(0);

    close();
}

void AFMainFrameGuide::qslotBroadTriggered()
{
    emit qsignalBroadTriggered();
    emit qsignalMissionCleared(2);
    close();
}

bool AFMainFrameGuide::event(QEvent* e)
{
    switch (e->type())
    {
    case QEvent::Show:
        if (m_installedEventFilter == false)
        {
            m_installedEventFilter = true;
            // Must Get windowHandle, This First Show
            QWidget* parentWidget = qobject_cast<QWidget*>(this->parent());
            parentWidget->windowHandle()->installEventFilter(m_pMainRaiseEventFilter);
        }
        break;
    case QEvent::WindowActivate:
        MAINFRAME->raise();
    }
    return AFQHoverWidget::event(e);
}

void AFMainFrameGuide::resizeEvent(QResizeEvent* event)
{
    qDebug() << "resized";

}

void AFMainFrameGuide::SceneSourceGuide(QRect position)
{
    ui->widget_Login->close();
    ui->pushButton_GuideBroad->close();

    QPropertyAnimation* scenesourceAnim = new QPropertyAnimation(ui->pushButton_SceneSource, "geometry", this);

    scenesourceAnim->setDuration(500);
    scenesourceAnim->setLoopCount(-1);

    QPoint buttonpos = QPoint(position.x(), position.y());
    QSize buttonSize = QSize(52, 52);

    scenesourceAnim->setStartValue(QRect(buttonpos.x(), buttonpos.y(), buttonSize.width(), buttonSize.height()));
    scenesourceAnim->setKeyValueAt(0.5, QRect(buttonpos.x() + 5, buttonpos.y() + 5, buttonSize.width() - 10, buttonSize.height() - 10));
    scenesourceAnim->setEndValue(QRect(buttonpos.x(), buttonpos.y(), buttonSize.width(), buttonSize.height()));

    scenesourceAnim->start(QPropertyAnimation::DeleteWhenStopped);

    connect(ui->pushButton_SceneSource, &QPushButton::clicked, this, &AFMainFrameGuide::qslotSceneSourceTriggered);
}

void AFMainFrameGuide::LoginGuide(QRect position)
{
    ui->pushButton_GuideBroad->close();
    ui->pushButton_SceneSource->close();

    QPropertyAnimation* loginanim = new QPropertyAnimation(ui->widget_Login, "geometry", this);

    loginanim->setDuration(500);
    loginanim->setLoopCount(-1);

    QPoint buttonpos = QPoint(position.x(), position.y());
    QSize buttonSize = QSize(60, 30);

    loginanim->setStartValue(QRect(buttonpos.x(), buttonpos.y(), buttonSize.width(), buttonSize.height()));
    loginanim->setKeyValueAt(0.5, QRect(buttonpos.x() + 5, buttonpos.y() + 5, buttonSize.width() - 10, buttonSize.height() - 10));
    loginanim->setEndValue(QRect(buttonpos.x(), buttonpos.y(), buttonSize.width(), buttonSize.height()));

    loginanim->start(QPropertyAnimation::DeleteWhenStopped);


    QPropertyAnimation* loginanimlabel = new QPropertyAnimation(ui->label_Login, "fontPointSize", this);

    loginanimlabel->setDuration(500);
    loginanimlabel->setLoopCount(-1);

    loginanimlabel->setStartValue(14);
    loginanimlabel->setKeyValueAt(0.5, 10);
    loginanimlabel->setEndValue(14);

    loginanimlabel->start(QPropertyAnimation::DeleteWhenStopped);

    connect(ui->widget_Login, &AFQHoverWidget::qsignalMouseClick, this, &AFMainFrameGuide::qslotLoginTriggered);
}

void AFMainFrameGuide::BroadGuide(QRect position)
{
    ui->widget_Login->close();
    ui->pushButton_SceneSource->close();

    QPropertyAnimation* broadAnim = new QPropertyAnimation(ui->pushButton_GuideBroad, "geometry", this);

    broadAnim->setDuration(500);
    broadAnim->setLoopCount(-1);

    QPoint buttonpos = QPoint(position.x(), position.y());
    QSize buttonSize = QSize(85, 34);

    broadAnim->setStartValue(QRect(buttonpos.x(), buttonpos.y(), buttonSize.width(), buttonSize.height()));
    broadAnim->setKeyValueAt(0.5, QRect(buttonpos.x() + 5, buttonpos.y() + 5, buttonSize.width() - 10, buttonSize.height() - 10));
    broadAnim->setEndValue(QRect(buttonpos.x(), buttonpos.y(), buttonSize.width(), buttonSize.height()));

    broadAnim->start(QPropertyAnimation::DeleteWhenStopped);

    connect(ui->pushButton_GuideBroad, &QPushButton::clicked, this, &AFMainFrameGuide::qslotBroadTriggered);
}

void AFMainFrameGuide::TutorialInit(QRect geo)
{
    const char* locale = LOCALE_CONTEXT.GetCurrentLocale();
    QString localeStr(locale);

    resize(geo.width(), geo.height());
    std::string absPath;

    if (geo.height() < 601)
    {
        QString svgPath = QString("assets/guide/%1/guide-gnb-small.svg").arg(localeStr);
        GetDataFilePath(svgPath.toUtf8().constData(), absPath);
        m_pGnbGuide = new QSvgWidget(QString::fromStdString(absPath), this);
        m_pGnbGuide->setStyleSheet("QWidget{background-color:rgba(0,0,0,0%);}");

        svgPath = QString("assets/guide/%1/guide-channel-small.svg").arg(localeStr);
        GetDataFilePath(svgPath.toUtf8().constData(), absPath);
        m_pChannelGuide = new QSvgWidget(QString::fromStdString(absPath), this);
        m_pChannelGuide->setStyleSheet("QWidget{background-color:rgba(0,0,0,0%);}");

        svgPath = QString("assets/guide/%1/guide-broad-small.svg").arg(localeStr);
        GetDataFilePath(svgPath.toUtf8().constData(), absPath);
        m_pBroadGuide = new QSvgWidget(QString::fromStdString(absPath), this);
        m_pBroadGuide->setStyleSheet("QWidget{background-color:rgba(0,0,0,0%);}");

        svgPath = QString("assets/guide/%1/guide-button-small.svg").arg(localeStr);
        GetDataFilePath(svgPath.toUtf8().constData(), absPath);
        m_pButtonGuide = new QSvgWidget(QString::fromStdString(absPath), this);
        m_pButtonGuide->setStyleSheet("QWidget{background-color:rgba(0,0,0,0%);}");
    }
    else
    {
        QString svgPath = QString("assets/guide/%1/guide-gnb.svg").arg(localeStr);
        GetDataFilePath(svgPath.toUtf8().constData(), absPath);
        m_pGnbGuide = new QSvgWidget(QString::fromStdString(absPath), this);
        m_pGnbGuide->setStyleSheet("QWidget{background-color:rgba(0,0,0,0%);}");

        svgPath = QString("assets/guide/%1/guide-channel.svg").arg(localeStr);
        GetDataFilePath(svgPath.toUtf8().constData(), absPath);
        m_pChannelGuide = new QSvgWidget(QString::fromStdString(absPath), this);
        m_pChannelGuide->setStyleSheet("QWidget{background-color:rgba(0,0,0,0%);}");

        svgPath = QString("assets/guide/%1/guide-broad.svg").arg(localeStr);
        GetDataFilePath(svgPath.toUtf8().constData(), absPath);
        m_pBroadGuide = new QSvgWidget(QString::fromStdString(absPath), this);
        m_pBroadGuide->setStyleSheet("QWidget{background-color:rgba(0,0,0,0%);}");

        svgPath = QString("assets/guide/%1/guide-button.svg").arg(localeStr);
        GetDataFilePath(svgPath.toUtf8().constData(), absPath);
        m_pButtonGuide = new QSvgWidget(QString::fromStdString(absPath), this);
        m_pButtonGuide->setStyleSheet("QWidget{background-color:rgba(0,0,0,0%);}");
    }

    QString svgPath = QString("assets/guide/guide-close.svg").arg(localeStr);
    GetDataFilePath(svgPath.toUtf8().constData(), absPath);
    ui->pushButton_Close->setIcon(QIcon(QString::fromStdString(absPath)));
    ui->pushButton_Close->setFixedSize(44, 44);
    ui->pushButton_Close->setIconSize(QSize(44, 44));
}

void AFMainFrameGuide::TutorialPosition(QRect gnb, QRect channel, QRect broad, QRect button)
{
    //Close Button
    int closeRightMargin = 16;
    int closeTopMargin = 16;
    int closeX = width() - closeRightMargin - ui->pushButton_Close->width();
    int closeY = closeTopMargin;
    ui->pushButton_Close->move(closeX, closeY);

    //GNB Guide
    m_pGnbGuide->setGeometry(gnb);

    //Channel Guide
    m_pChannelGuide->setGeometry(channel);

    //Broad Guide
    m_pBroadGuide->setGeometry(broad);

    //Button Guide
    m_pButtonGuide->setGeometry(button);
}

