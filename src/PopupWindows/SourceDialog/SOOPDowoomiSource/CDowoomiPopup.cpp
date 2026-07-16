#include "CDowoomiPopup.h"

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"
#include "Application/CApplication.h"

#include "CoreModel/Locale/CLocaleTextManager.h"


AFQDowoomiPopup::AFQDowoomiPopup(QWidget *parent, int type)
    :AFTTopBaseDialog(parent, Qt::WindowFlags())
    , ui(new Ui::AFQDowoomiPopup)
{
    ui->setupUi(this);

#ifdef _WIN32
    setWindowFlags(Qt::Dialog | windowFlags());
    AFQBlockManager::ApplyMoveInAllArea(this);
#elif defined(__APPLE__)
    setWindowFlags(Qt::Dialog|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    ui->titleFrame->hide();
#endif

    SetWidthResizeEnabled(true);
    SetHeightResizeEnabled(false);

    QString locale = LOCALE_CONTEXT.GetCurrentLocale();
    QString title;
    if (type)
    {
        ui->widget_ChatNotification->hide();
        ui->widget_ChatTimer->hide();
        ui->widget_ChatFix->hide();
        title = QTStr("Dowoomi.Command.Score");
        ui->label_Guidance->setText(QTStr("Dowoomi.Command.Score.Guidance"));
        if (locale == "th-TH")
            setFixedSize(360, 270);
        else if (locale == "en-US")
            setFixedSize(516, 270);
        else
            setFixedSize(340, 270);
    }
    else
    {
        ui->widget_Score->hide();
        title = QTStr("Dowoomi.Command.Chat");
        ui->label_Guidance->setText(QTStr("Dowoomi.Command.Chat.Guidance"));

        if (locale == "th-TH")
            setFixedSize(454, 550);
        else if (locale == "en-US")
            setFixedSize(490, 550);
        else if (locale == "zh-CN" || locale == "zh-TW")
            setFixedSize(375, 550);
    }
    
#ifdef _WIN32
    ui->label_WindowTitle->setText(title);
#elif defined(__APPLE__)
    setWindowTitle(title);
#endif
    
    // connect action
    connect(ui->buttonClose, &QPushButton::clicked,
        this, &AFQDowoomiPopup::close);

    QList<QLabel*> labels = findChildren<QLabel*>();
    for (QLabel* label : labels) {
        PolishStyleSheet(label);
    }
}

AFQDowoomiPopup::~AFQDowoomiPopup()
{
    delete ui;
}

void AFQDowoomiPopup::showEvent(QShowEvent* event)
{
    QRect midRect = MAIN_BLOCKMANAGER->GetMidGeometry(this->size());
    QRect adjustRect;
    MAIN_BLOCKMANAGER->AdjustPositionOutSideFullScreen(midRect, adjustRect);

    move(adjustRect.x(), adjustRect.y());
}
