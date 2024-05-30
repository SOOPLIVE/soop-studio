#include "CMainFrameGuide.h"
#include "ui_main-frame-guide.h"

#include <QWindow>

#include "QPropertyAnimation"

#include "MainFrame/CMustRaiseMainFrameEventF.h"


AFMainFrameGuide::AFMainFrameGuide(QWidget *parent) :
    AFQHoverWidget(parent),
    ui(new Ui::AFMainFrameGuide)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    
    m_pMainRaiseEventFilter = new AFQMustRaiseMainFrameEventFilter();
}

AFMainFrameGuide::~AFMainFrameGuide()
{
    delete m_pMainRaiseEventFilter;
    delete ui;
}

void AFMainFrameGuide::qslotLoginTriggered()
{
    emit qsignalLoginTrigger();

    close();
}

void AFMainFrameGuide::qslotSceneSourceTriggered()
{
    emit qsignalSceneSourceTriggered(true, 0);

    close();
}

void AFMainFrameGuide::qslotBroadTriggered()
{
    emit qsignalBroadTriggered();
    close();
}

bool AFMainFrameGuide::event(QEvent* e)
{
    switch (e->type())
    {
    case QEvent::Show:
    {
        if (m_bInstalledEventFilter == false)
        {
            m_bInstalledEventFilter = true;
            // Must Get windowHandle, This First Show
            QWidget* parentWidget = qobject_cast<QWidget*>(this->parent());
            parentWidget->windowHandle()->installEventFilter(m_pMainRaiseEventFilter);
        }
        break;
    }
    default:
        break;
    }
    return QWidget::event(e);
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

