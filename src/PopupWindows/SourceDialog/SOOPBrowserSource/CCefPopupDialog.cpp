
#include "CCefPopupDialog.h"
#include "ui_dock-popup-widget.h"

#include <QCloseEvent>

#include "qt-wrappers.hpp"

#include "Common/StudioDefine.h"
#include "Application/CApplication.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/OBSOutput/COutput.h"

#include "MainFrame/Output/COutput.h"

#include "Blocks/CBlockManager.h"

#include "PopupWindows/CCateChangeDialog.h"

const char* k_CefPopupIndex[] = {
    "soop_kbo_graphic_source_all",
    "soop_kbo_graphic_source_score",
    "soop_kbo_graphic_source_stadium",
    "soop_kbo_graphic_source_player",
    "soop_kbo_graphic_source_livetext",
    "soop_commerce_source_goal",
    "soop_commerce_source_rank",
    "soop_football_graphic_source_all",
    "soop_football_graphic_source_player",
    "soop_football_graphic_source_change",
    "soop_football_graphic_source_score",
    "soop_football_graphic_source_livetext",
};
//

std::string AFQCefPopupDialog::GetCefPopupURL(CefPopupType type)
{
    return "";
}

const AFQBlockManager::BlockBaseInfo k_CefPopupInfo[] = {
    /*
    
        bool            noData          = false;
        bool            transparent     = false;
        bool            transformable   = false;
        bool            showLeft         = true;
        bool            showRight        = false;
        bool            needQuestionMark = false;
        QSize           minSize         = QSize(0,0);
        QSize           maxSize         = QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        const char*     windowTitle     = "";
	    QString         questionMarkToolTip = "";
        Qt::WindowFlags flags           = Qt::Window;
        ParentTypes     parentOnPopup   = NoParent;
    */
    // kbo graphic
	{
		false, false, false, true, false, false, false,
		{600,820}, {600,820}, "KBO.List.Title", "",
		Qt::Window, AFQBlockManager::ParentTypes::NoParent
	},
	{
		false, false, false, true, false, false, false,
		{600,820}, {600,820}, "KBO.List.Title", "",
		Qt::Window, AFQBlockManager::ParentTypes::NoParent
	},
	{
		false, false, false, true, false, false, false,
		{600,820}, {600,820}, "KBO.List.Title", "",
		Qt::Window, AFQBlockManager::ParentTypes::NoParent
	},
	{
		false, false, false, true, false, false, false,
		{600,820}, {600,820}, "KBO.List.Title", "",
		Qt::Window, AFQBlockManager::ParentTypes::NoParent
	},
	{
		false, false, false, true, false, false, false,
		{600,820}, {600,820}, "KBO.List.Title", "",
		Qt::Window, AFQBlockManager::ParentTypes::NoParent
	},
    // commerce goal
    {
        false, false, false, true, false, false, false,
        {640,480}, {875,1000}, "Live.Commerce.Setting", "",
        Qt::Window, AFQBlockManager::ParentTypes::NoParent
    },
    // commerce rank
    {
        false, false, false, true, false, false, false,
        {640,480}, {875,1000}, "Live.Commerce.Setting", "",
        Qt::Window, AFQBlockManager::ParentTypes::NoParent
    },

    //football graphic
    {
        false, false, false, true, false, false, false,
        {600,820}, {600,820}, "Football.List.Title", "",
        Qt::Window, AFQBlockManager::ParentTypes::NoParent
    },
    {
        false, false, false, true, false, false, false,
        {600,820}, {600,820}, "Football.List.Title", "",
        Qt::Window, AFQBlockManager::ParentTypes::NoParent
    },
    {
        false, false, false, true, false, false, false,
        {600,820}, {600,820}, "Football.List.Title", "",
        Qt::Window, AFQBlockManager::ParentTypes::NoParent
    },
    {
        false, false, false, true, false, false, false,
        {600,820}, {600,820}, "Football.List.Title", "",
        Qt::Window, AFQBlockManager::ParentTypes::NoParent
    },
    {
        false, false, false, true, false, false, false,
        {600,820}, {600,820}, "Football.List.Title", "",
        Qt::Window, AFQBlockManager::ParentTypes::NoParent
    },
};

AFQCefPopupDialog::CefPopupType AFQCefPopupDialog::GetIndexCefPopup(const char* id)
{
    if(CefPopupType::CefPopupType_None != m_sourceType)
        return m_sourceType;

    for(int idx = 0; idx < CefPopupType_End; idx++) {
        if(0 == strcmp(id, k_CefPopupIndex[idx])) {
            m_sourceType = (CefPopupType)idx;
            break;
        }
    }
    return m_sourceType;
}

AFQCefPopupDialog::AFQCefPopupDialog(QWidget* parent,
                                     obs_source_t* source,
                                     Qt::WindowFlags flag,
                                     bool widthResizable,
                                     bool heightResizable) :
    AFTTopBaseDialog(parent, flag),
    ui(new Ui::AFQBorderPopupBaseWidget)
{
    ui->setupUi(this);

    SetWidthResizeEnabled(widthResizable);
    SetHeightResizeEnabled(heightResizable);
    setAttribute(Qt::WA_DeleteOnClose);

    ui->dockPopupContainerLayout->setContentsMargins(1, 1, 1, 1);

    _RegisterSource(source);

    bool success = _Initialize();
    if(!success) {
        qDebug() << "error!";
    }
}
AFQCefPopupDialog::~AFQCefPopupDialog()
{
    m_weakSource = nullptr;

    delete ui;
}

void AFQCefPopupDialog::qslotDataReceivedFromBrowser(const QCefQuery& query)
{
    qDebug() << "Get Query";
    switch (m_sourceType) {
    case CefPopupType_KBOGraphic_All:
    case CefPopupType_KBOGraphic_Score:
    case CefPopupType_KBOGraphic_Stadium:
    case CefPopupType_KBOGraphic_Player:
    case CefPopupType_KBOGraphic_Livetext:
        _ParseKBOGraphic(query);
        break;

    case CefPopupType_Football_Graphic_All:
    case CefPopupType_Football_Graphic_Player:
    case CefPopupType_Football_Graphic_Change:
    case CefPopupType_Football_Graphic_Score:
    case CefPopupType_Football_Graphic_Livetext:
        _ParseFootballGraphic(query);
        break;

    default:
        break;
    }
}

void AFQCefPopupDialog::qslotCloseCustom()
{
    close();
    deleteLater();
}
//
void AFQCefPopupDialog::SetUrl(const std::string& url)
{
    if(m_pCustomBrowserWidget)
    {
        m_pCustomBrowserWidget->setURL(url);
    }
}
void AFQCefPopupDialog::ReloadCefWidget()
{
    if(m_pCustomBrowserWidget)
        m_pCustomBrowserWidget->reloadPage();
}
void AFQCefPopupDialog::ExecuteScript(const std::string& script)
{
    if(m_pCustomBrowserWidget)
        m_pCustomBrowserWidget->executeJavaScript(script);
}
QWidget* AFQCefPopupDialog::GetWidgetByName(const QString& name)
{
    return this->findChild<QWidget*>(name);
}

void AFQCefPopupDialog::closeEvent(QCloseEvent* event)
{
    if(!event->isAccepted())
        return;

    static int panel_version = -1;
    if(panel_version == -1)
        panel_version = obs_browser_qcef_version();


    if(panel_version >= 2 && !!m_pCustomBrowserWidget)
        m_pCustomBrowserWidget->closeBrowser();

    event->accept();
}
//
void AFQCefPopupDialog::_RegisterSource(obs_source_t* source)
{
    if(source)
    {
        m_sourceId = obs_source_get_id(source);
        m_weakSource = OBSGetWeakRef(source);
    } else {
        m_weakSource = nullptr;
    }
}
OBSSource AFQCefPopupDialog::_GetSource()
{
    return OBSGetStrongRef(m_weakSource);
}

bool AFQCefPopupDialog::_Initialize()
{
    bool success = false;
    do {
        m_sourceType = GetIndexCefPopup(m_sourceId.c_str());
        if(CefPopupType::CefPopupType_None == m_sourceType)
            break;

        std::string url = GetCefPopupURL(m_sourceType);
        QCefWidget* cefWidget = CEFMANAGER.createWidget(this, url);
        if(!cefWidget) {
            qDebug() << "Failed to create cefwidget";
            break;
        }
        m_pCustomBrowserWidget = cefWidget;
        cefWidget->setParent(this);

        connect(cefWidget, SIGNAL(cefQueryRequest(const QCefQuery&)), this, SLOT(qslotDataReceivedFromBrowser(const QCefQuery&)));

        //
        AFQBlockManager::BlockBaseInfo info = k_CefPopupInfo[m_sourceType];
        QSize minSize = info.minSize;

        QSize maxSize = info.maxSize;

        if (MAINFRAME->IsSmallResolution())
        {
            minSize.setHeight(550);
            maxSize.setHeight(550);

            if (maxSize.width() > 800)
                maxSize.setWidth(800);
        }

        setMinimumSize(minSize);
        if(CefPopupType_CommerceGoal == m_sourceType ||
           CefPopupType_CommerceRank == m_sourceType) {
            resize(maxSize);
        } else {
            setMaximumSize(maxSize);
        }

        ui->titleLabel->setText(QTStr(info.windowTitle));

        connect(ui->closeButton, &QPushButton::clicked,
            this, &AFQCefPopupDialog::qslotCloseCustom);

        setWindowTitle(QTStr(info.windowTitle));

        ui->dockPopupContainerLayout->addWidget(cefWidget);

        _ApplyMoveInAllArea(this);

        QSize popupSize = size();
        int posX = MAINFRAME->pos().x() + (MAINFRAME->width() / 2) - (popupSize.width() / 2);
        int posY = MAINFRAME->pos().y() + (MAINFRAME->height() / 2) - (popupSize.height() / 2);

        QRect dockPopupPosition = QRect(posX, posY, popupSize.width(), popupSize.height());
        setGeometry(dockPopupPosition);
        //
        show();
        activateWindow();
        raise();

        QApplication::processEvents();

        success = true;
    } while(false);
    if(!success) {
        qDebug() << "Failed to initialize cefpopup";
    }
    return success;
}
void AFQCefPopupDialog::_ApplyMoveInAllArea(QObject* applyWidget)
{
    applyWidget->setProperty("MoveInAllArea", true);
    QObjectList objects = applyWidget->children();

    foreach(QObject * object, objects)
    {
        if(QWidget* widget = qobject_cast<QWidget*>(object)) {
            if(App()->CheckClickableWidget(object, true))
            {
            } else
            {
                object->setProperty("MoveInAllArea", true);
                _ApplyMoveInAllArea(object);
            }
        }
    }
}

void AFQCefPopupDialog::_ParseKBOGraphic(const QCefQuery& query)
{
    if (AFOutputUtil::IsStreamActive()) {
        if (!MAIN_OUTPUT->IsStreamingOnlySoop()) {
            QString exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage2")
                .arg(QTStr("Basic.SelectedSourcePopup.KBO"));
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                "", exceptionMsg);
            return;
        }
    }

    QString strRequest = query.reqeust();
    if(strRequest.isEmpty())
        return;
}

void AFQCefPopupDialog::_ParseFootballGraphic(const QCefQuery& query)
{
    if (AFOutputUtil::IsStreamActive()) {
        if (!MAIN_OUTPUT->IsStreamingOnlySoop()) {
            QString exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage2")
                .arg(QTStr("Basic.SelectedSourcePopup.Football"));
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                "", exceptionMsg);
            return;
        }
    }

    QString strRequest = query.reqeust();
    if (strRequest.isEmpty())
        return;
}