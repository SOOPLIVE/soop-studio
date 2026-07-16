#include "CImportGuide.h"
#include "ui_import-guide.h"

#include <QDir>

#include "qt-wrappers.hpp"
#include "util/platform.h"

#include "Common/StudioDefine.h"
#include "Application/CApplication.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/OBSData/CLoadSaveManager.h"
#include "CoreModel/Video/CVideo.h"
#include "CoreModel/Profile/CProfile.h"
#include "CoreModel/Encoder/CEncoder.h"

#include "MainFrame/SceneCollection/CMainSceneCollection.h"
#include "MainFrame/SceneSource/CMainSceneSource.h"
#include "MainFrame/Output/COutput.h"

#include "Utils/importers/importers.hpp"

#include "UIComponent/CMessageBox.h"

#include "MainFrame/Profile/CMainProfile.h"


AFQImportGuide::AFQImportGuide(QWidget *parent) :
    AFTTopBaseDialog(parent, Qt::WindowFlags()),
    ui(new Ui::AFQImportGuide)
{
    ui->setupUi(this);
    ui->titleFrame->setProperty("MoveInAllArea", true);

    setWindowTitle(APPNAME);    
}

AFQImportGuide::~AFQImportGuide()
{
    delete ui;
}

void AFQImportGuide::qslotFromFreecshotTriggered()
{
#ifdef __APPLE__
    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
        "", QTStr("Basic.Guide.FromFreecshot.Mac"));
    return;
#endif

    QString filePath;
    if (GetProfileFilePath("SOOP\\freecshot\\settings", "", filePath))
    {
        _ApplyFreecShotSharedLogic(filePath, "FreecShot Import");

        QDialog::accept();
        close();
    }
}

void AFQImportGuide::qslotFromFreecshotProfileTriggered()
{
    QString filePath;
    QDir appDataDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QString appDataPath = appDataDir.absolutePath() + "/..";
    appDataPath = QDir(appDataPath).canonicalPath() + "/SOOP/freecshot/settings";

    if (GetProfileFilePath("SOOP\\freecshot\\settings", "", filePath)) {
        m_FreecShotProfileDir = filePath;
    }

    emit qsignalLoadFreecShotDataTriggered();
}

void AFQImportGuide::LoadFromStudio2Json()
{
#ifdef __APPLE__
    return;
#endif

    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/../SOOP/freecshot/settings/studio2.json";
    QString filePath = QDir(appDataPath).canonicalPath();

    if (!QFile::exists(filePath)) {
        blog(LOG_WARNING, "studio2.json not found: %s", filePath.toStdString().c_str());
        return;
    }

    _ApplyFreecShotSharedLogic(filePath, "FreecShot Import");

    QDialog::accept();
    close();
}


//Due To Progressbar - not use mainframe load save
void AFQImportGuide::qslotFromOBSProfileTriggered()
{
    m_obsProfileDir = "";
    m_soopProfileDir = "";

    QObject* importAction = reinterpret_cast<QObject*>(sender());

    QString home = QDir::homePath();

    QVariant pathProperty = importAction->property("homePath");
    if (pathProperty.isValid())
    {
        char homePath[512];

        QString stringProperty = pathProperty.toString();
        QByteArray homePatharray = stringProperty.toUtf8();

        int ret = GetAppConfigPath(homePath, 512, homePatharray.constData());

        if (ret > 0) 
        {
            // Check Dir Exists
            QDir dir(homePath);
            if (dir.exists())
                home = QString::fromUtf8(homePath);
        }
    }

    char path[512];
    int ret = GetAppConfigPath(path, 512, (LOCAL_FOLDER_NAME + "/basic/profiles").c_str());
    if (ret <= 0) {
        blog(LOG_WARNING, "Failed to get profile config path");
        emit qsignalLoadOBSDataTriggered();
        return;
    }

    m_obsProfileDir = SelectDirectory(this, QT_UTF8(Str("Basic.MainMenu.Profile.Import")), home);

    if (!m_obsProfileDir.isEmpty() && !m_obsProfileDir.isNull())
    {
        QString inputPath = QString::fromUtf8(path);
        QFileInfo finfo(m_obsProfileDir);
        QString directory = finfo.fileName();
        m_soopProfileDir = inputPath + directory;

        if (AFProfileUtil::ProfileExists(directory.toStdString().c_str()))
        {
            m_obsProfileDir = "";
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                "",
                QTStr("Basic.MainMenu.Profile.Exists"));
        }
        else if (os_mkdir(m_soopProfileDir.toStdString().c_str()) < 0)
        {
            m_obsProfileDir = "";
            blog(LOG_WARNING,
                "Failed to create profile directory '%s'",
                directory.toStdString().c_str());
        }
    }

    emit qsignalLoadOBSDataTriggered();
}

void AFQImportGuide::qslotFromOBSSceneTriggered()
{
    m_obsSceneDir = "";
    m_soopSceneDir = "";

    // Check OBS Dir
    QString home = QDir::homePath();
    char obsScenePath[512];

    QString stringProperty = "obs-studio/basic/scenes/";
    QByteArray homePatharray = stringProperty.toUtf8();

    int ret = GetAppConfigPath(obsScenePath, 512, homePatharray.constData());
    if (ret > 0)
    {
        // Check Dir Exists
        QDir dir(obsScenePath);
        if (dir.exists())
            home = QString::fromUtf8(obsScenePath);
    }
    //

    // Check SOOP Dir
    char soopScenePath[512];
    ret = GetAppConfigPath(soopScenePath, 512, (LOCAL_FOLDER_NAME + "/basic/scenes/").c_str());
    m_soopSceneDir = QString::fromUtf8(soopScenePath);

    if (ret <= 0) {
        blog(LOG_WARNING, "Failed to get scene collection config path");
        emit qsignalLoadOBSDataTriggered();
        return;
    }
    //

    // Get Scene Collection Config File Path
    m_obsSceneDir = OpenFile(this,
                            QT_UTF8(Str("Basic.MainMenu.SceneCollection.Import")),
                            home, 
                            "*.json");

    emit qsignalLoadOBSDataTriggered();
}

void AFQImportGuide::qslotNewSettingTriggered()
{
    ui->stackedWidget_ImportGuide->setCurrentIndex(1);
    m_startChoice = 2;
}

void AFQImportGuide::qslotOBSProfileCopyTriggered()
{
    m_pStepTimer->stop();

    m_step++;
    int val = 25 * m_step;
    switch (m_step)
    {
    case 1:
        ui->progressBar_Importing->setValue(val);
        QFile::copy(m_obsProfileDir + "/basic.ini",
            m_soopProfileDir + "/basic.ini");
        break;
    case 2:
        ui->progressBar_Importing->setValue(val);
        //QFile::copy(m_obsProfileDir + "/service.json",
            //m_soopProfileDir + "/service.json");
        break;
    case 3:
        ui->progressBar_Importing->setValue(val);
        QFile::copy(m_obsProfileDir + "/streamEncoder.json",
            m_soopProfileDir + "/streamEncoder.json");
        break;
    case 4:
        ui->progressBar_Importing->setValue(val);
        QFile::copy(m_obsProfileDir + "/recordEncoder.json",
            m_soopProfileDir + "/recordEncoder.json");
        break;
    }

    if (val <= 100)
        m_pStepTimer->start();
    else 
    {
        m_selectedProfilePath = m_soopProfileDir.toStdString();
        QDialog::accept();
        close();
    }
}

void AFQImportGuide::_qslotApplyPresetTriggered()
{
    std::string name = "";
    std::string file = "";

    if (ui->stackedWidget_Preset->currentWidget() == ui->page_PresetGame) {
        m_presetType = BroadPresetType::Game;
        name = "Game";
    }
    else if (ui->stackedWidget_Preset->currentWidget() == ui->page_PresetVisibleRadio) {
        m_presetType = BroadPresetType::VisibleRadio;
        name = "VisibleRadio";
    }
    else if (ui->stackedWidget_Preset->currentWidget() == ui->page_PresetRelayBroad) {
        m_presetType = BroadPresetType::RelayBroadcast;
        name = "RelayBroadcast";
    }
    else {
        QDialog::accept();
        close();
        return;
    }

    std::string fileName;
    if (!CONFIG_CONTEXT.GetFileSafeName(name.c_str(), fileName)) {
        blog(LOG_WARNING, "Failed to create safe file name for '%s'", fileName.c_str());
    }

    char dst[512];
    GetAppConfigPath(dst, 512, (LOCAL_FOLDER_NAME + "/basic/scenes/").c_str());
    std::string collectionFile;
    collectionFile.reserve(strlen(dst) + fileName.size());
    collectionFile.append(dst).append(fileName);


    if (!CONFIG_CONTEXT.GetClosestUnusedFileName(collectionFile, "json")) {
        blog(LOG_WARNING, "Failed to get closest file name for %s", fileName.c_str());
    }

    config_set_string(USERCONFIG, "Basic", "SceneCollection", name.c_str());
    config_set_string(USERCONFIG, "Basic", "SceneCollectionFile", collectionFile.c_str());
    config_save_safe(USERCONFIG, "tmp", nullptr);

    QDialog::accept();
    close();
}

void AFQImportGuide::_qslotSelfSettingTriggered()
{
    m_presetType = BroadPresetType::New;

    config_set_string(USERCONFIG, "Basic", "SceneCollection",
                      config_get_default_string(USERCONFIG, "Basic", "SceneCollection"));
    config_set_string(USERCONFIG, "Basic", "SceneCollectionFile",
                      config_get_default_string(USERCONFIG, "Basic", "SceneCollectionFile"));
    config_save_safe(USERCONFIG, "tmp", nullptr);

    QDialog::accept();
    close();
}

void AFQImportGuide::_qslotAddPresetTriggered()
{
    if (ui->stackedWidget_Preset->currentWidget() == ui->page_PresetGame) {
        m_presetType = BroadPresetType::Game;
    }
    else if (ui->stackedWidget_Preset->currentWidget() == ui->page_PresetVisibleRadio) {
        m_presetType = BroadPresetType::VisibleRadio;
    }
    else if (ui->stackedWidget_Preset->currentWidget() == ui->page_PresetRelayBroad) {
        m_presetType = BroadPresetType::RelayBroadcast;

        if (0 != AFSourceUtil::CheckAddSoopVodSource("soop_directbroad_source"))
            return;
    }

    QDialog::accept();
    close();
}

void AFQImportGuide::_qslotSwitchPreset(bool left)
{
    ui->pushButton_PresetLeft->setDisabled(false);
    ui->pushButton_PresetRight->setDisabled(false);

    int currentIdx = ui->stackedWidget_Preset->currentIndex();
    int pageCount = ui->stackedWidget_Preset->count();
    int newIdx;

    if (left) {
        newIdx = currentIdx - 1;
        if(newIdx >= 0)
            ui->stackedWidget_Preset->setCurrentIndex(newIdx);
        if(newIdx == 0)
            ui->pushButton_PresetLeft->setDisabled(true);
        
    }
    else {
        newIdx = currentIdx + 1;
        if(newIdx <= pageCount)
            ui->stackedWidget_Preset->setCurrentIndex(newIdx);

        if (newIdx == pageCount - 1)
            ui->pushButton_PresetRight->setDisabled(true);
    }
}

void AFQImportGuide::_qslotSetApplyOBSDataBtnActive()
{
    bool checkSceneDirData = (!m_obsSceneDir.isEmpty() && !m_obsSceneDir.isNull()) && 
                             (!m_soopSceneDir.isEmpty() && !m_soopSceneDir.isNull());
    bool checkProfileDirData = (!m_obsProfileDir.isEmpty() && !m_obsProfileDir.isNull()) &&
                               (!m_soopProfileDir.isEmpty() && !m_soopProfileDir.isNull());
    
    ui->checkBox_FromOBSScene->setChecked(checkSceneDirData);
    ui->checkBox_FromOBSProfile->setChecked(checkProfileDirData);
    ui->checkBox_FromOBSRecentScene->setChecked(checkSceneDirData);
    ui->checkBox_FromOBSRecentProfile->setChecked(checkProfileDirData);

    if (checkSceneDirData /*||*/ && checkProfileDirData) {
        ui->pushButton_ApplyOBSSettings->setEnabled(true);
        ui->pushButton_ApplyRecentOBSSettings->setEnabled(true);
    }
    else {
        ui->pushButton_ApplyOBSSettings->setEnabled(false);
        ui->pushButton_ApplyRecentOBSSettings->setEnabled(false);
    }
}

void AFQImportGuide::_qslotSetApplyFreecShotDataBtnActive()
{
    bool checkProfileDirData = (!m_FreecShotProfileDir.isEmpty() && !m_FreecShotProfileDir.isNull());

    ui->checkBox_FromFreecShotRecentProfile->setChecked(checkProfileDirData);

    if (checkProfileDirData) {
        ui->pushButton_ApplyRecentFreecShotSettings->setEnabled(true);
    }
    else {
        ui->pushButton_ApplyRecentFreecShotSettings->setEnabled(false);
    }
}

void AFQImportGuide::_qslotApplyOBSDataTriggered()
{
    _LoadOBSSceneData();
    _LoadOBSProfileData();
    MAIN_SCENECOLLECTION->RefreshSceneCollections(true);
    MAIN_PROFILE->RefreshProfiles();

    QDialog::accept();
    close();
}

void AFQImportGuide::_qslotApplyFreecShotDataTriggered()
{
    _LoadFreecShotData();
    MAIN_SCENECOLLECTION->RefreshSceneCollections(true);
    MAIN_PROFILE->RefreshProfiles();
    QDialog::accept();
    close();
}

void AFQImportGuide::_qslotPressedFromFreecshotWidget() 
{
    _UpdateWidgetStyle(
        { ui->widgetHover_FromFreecshot, ui->label_FreecShotImg, ui->label_FromFreecShotButtonText },
        { {"pressed", "true"}});
}

void AFQImportGuide::_qslotReleasedFromFreecshotWidget() 
{
    _UpdateWidgetStyle(
        { ui->widgetHover_FromFreecshot, ui->label_FreecShotImg, ui->label_FromFreecShotButtonText },
        { {"pressed", "false"} });
}

void AFQImportGuide::_qslotPressedNewSettingWidget() 
{
    _UpdateWidgetStyle(
        { ui->widgetHover_NewSetting, ui->label_SoopImg, ui->label_NewSettingButtonText },
        { {"pressed", "true"} });
}

void AFQImportGuide::_qslotReleasedNewSettingWidget() 
{
    _UpdateWidgetStyle(
        { ui->widgetHover_NewSetting, ui->label_SoopImg, ui->label_NewSettingButtonText },
        { {"pressed", "false"} });
}

void AFQImportGuide::_qslotHoverEnterSelfSettingWidget() 
{
    _UpdateWidgetStyle(
        { ui->label_SelfSetting, ui->label_SelfSettingImg },
        { {"hover", "true"},
          {"pressed", "false"} });
}

void AFQImportGuide::_qslotHoverLeaveSelfSettingWidget() 
{
    _UpdateWidgetStyle(
        { ui->label_SelfSetting, ui->label_SelfSettingImg },
        { {"hover", "false"},
          {"pressed", "false"} });
}

void AFQImportGuide::_qslotPressedSelfSettingWidget() 
{
    _UpdateWidgetStyle(
        { ui->label_SelfSetting, ui->label_SelfSettingImg },
        { {"hover", "false"},
          {"pressed", "true"} });
}

void AFQImportGuide::ImportGuideInit(bool addPreset)
{
    if (!addPreset) 
        ui->stackedWidget_ImportGuide->setCurrentIndex(0);        
    
    else {
        ui->stackedWidget_ImportGuide->setCurrentIndex(1);
        ui->label_SelfSetting->hide();
        ui->label_SelfSettingImg->hide();
    }
    
    ui->stackedWidget_Preset->setCurrentIndex(0);

    ui->pushButton_Close->setProperty("buttonType", "closeButton");
    ui->checkBox_FromOBSProfile->setProperty("homePath", "obs-studio/basic/profiles/");

    ui->pushButton_PresetLeft->setDisabled(true);

    // FreecShot, OBS Setting Page
    connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFQImportGuide::close);
    if (!addPreset) {
        connect(ui->widgetHover_FromFreecshot, &AFQHoverWidget::qsignalMouseClick,
            this, &AFQImportGuide::qslotFromFreecshotTriggered);
        connect(ui->widgetHover_NewSetting, &AFQHoverWidget::qsignalMouseClick,
            this, &AFQImportGuide::qslotNewSettingTriggered);
        connect(ui->checkBox_FromOBSProfile, &QCheckBox::clicked,
            this, &AFQImportGuide::qslotFromOBSProfileTriggered);
        connect(ui->checkBox_FromOBSScene, &QCheckBox::clicked,
            this, &AFQImportGuide::qslotFromOBSSceneTriggered);
        connect(this, &AFQImportGuide::qsignalLoadOBSDataTriggered,
            this, &AFQImportGuide::_qslotSetApplyOBSDataBtnActive);
        connect(ui->pushButton_ApplyOBSSettings, &QPushButton::clicked,
            this, &AFQImportGuide::_qslotApplyOBSDataTriggered);
    }    

    // Preset Page
    connect(ui->pushButton_PresetLeft, &QPushButton::clicked,
            this, [this] {_qslotSwitchPreset(true); });
    connect(ui->pushButton_PresetRight, &QPushButton::clicked,
            this, [this] {_qslotSwitchPreset(false); });
    if (!addPreset) {
        connect(ui->widget_SelfSetting, &AFQHoverWidget::qsignalMouseClick,
            this, &AFQImportGuide::_qslotSelfSettingTriggered);
        connect(ui->pushButton_PresetStart, &QPushButton::clicked,
            this, &AFQImportGuide::_qslotApplyPresetTriggered);
    }
    else {
        connect(ui->pushButton_PresetStart, &QPushButton::clicked,
            this, &AFQImportGuide::_qslotAddPresetTriggered);
    }    

    // Style Update Signals
    connect(ui->widgetHover_FromFreecshot, &AFQHoverWidget::qsignalMousePressed,
            this, &AFQImportGuide::_qslotPressedFromFreecshotWidget);
    connect(ui->widgetHover_FromFreecshot, &AFQHoverWidget::qsignalMouseReleased,
            this, &AFQImportGuide::_qslotReleasedFromFreecshotWidget);
    connect(ui->widgetHover_NewSetting, &AFQHoverWidget::qsignalMousePressed,
            this, &AFQImportGuide::_qslotPressedNewSettingWidget);
    connect(ui->widgetHover_NewSetting, &AFQHoverWidget::qsignalMouseReleased,
            this, &AFQImportGuide::_qslotReleasedNewSettingWidget);

    connect(ui->widget_SelfSetting, &AFQHoverWidget::qsignalHoverEnter,
            this, &AFQImportGuide::_qslotHoverEnterSelfSettingWidget);
    connect(ui->widget_SelfSetting, &AFQHoverWidget::qsignalHoverLeave,
            this, &AFQImportGuide::_qslotHoverLeaveSelfSettingWidget);
    connect(ui->widget_SelfSetting, &AFQHoverWidget::qsignalMousePressed,
            this, &AFQImportGuide::_qslotPressedSelfSettingWidget);
    connect(ui->widget_SelfSetting, &AFQHoverWidget::qsignalMouseReleased,
            this, &AFQImportGuide::_qslotHoverLeaveSelfSettingWidget);
    //

    ui->page_PresetGame->Init(BroadPresetType::Game);
    ui->page_PresetVisibleRadio->Init(BroadPresetType::VisibleRadio);
    ui->page_PresetRelayBroad->Init(BroadPresetType::RelayBroadcast);

    ui->checkBox_FromOBSScene->setChecked(false);
    ui->checkBox_FromOBSProfile->setChecked(false);
    ui->pushButton_ApplyOBSSettings->setEnabled(false);
}

void AFQImportGuide::ImportRecentGuideInit()
{
    ui->stackedWidget_ImportGuide->setCurrentIndex(3);

    ui->pushButton_Close->setProperty("buttonType", "closeButton");
    ui->checkBox_FromOBSRecentProfile->setProperty("homePath", "obs-studio/basic/profiles/");

    ui->pushButton_ApplyRecentFreecShotSettings->setDisabled(true);
    ui->pushButton_ApplyRecentOBSSettings->setDisabled(true);

    connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFQImportGuide::close);
    connect(ui->checkBox_FromFreecShotRecentProfile, &QCheckBox::clicked,
        this, &AFQImportGuide::qslotFromFreecshotProfileTriggered);
    connect(ui->checkBox_FromOBSRecentScene, &QCheckBox::clicked,
        this, &AFQImportGuide::qslotFromOBSSceneTriggered);
    connect(ui->checkBox_FromOBSRecentProfile, &QCheckBox::clicked,
        this, &AFQImportGuide::qslotFromOBSProfileTriggered);
    connect(this, &AFQImportGuide::qsignalLoadOBSDataTriggered,
        this, &AFQImportGuide::_qslotSetApplyOBSDataBtnActive);
    connect(this, &AFQImportGuide::qsignalLoadFreecShotDataTriggered,
        this, &AFQImportGuide::_qslotSetApplyFreecShotDataBtnActive);
    connect(ui->pushButton_ApplyRecentFreecShotSettings, &QPushButton::clicked,
        this, &AFQImportGuide::_qslotApplyFreecShotDataTriggered);
    connect(ui->pushButton_ApplyRecentOBSSettings, &QPushButton::clicked,
        this, &AFQImportGuide::_qslotApplyOBSDataTriggered);

    ui->checkBox_FromFreecShotRecentProfile->setChecked(false);
    ui->checkBox_FromOBSRecentScene->setChecked(false);
    ui->checkBox_FromOBSRecentProfile->setChecked(false);
}

const QVector<QPair<QString, QRectF>> AFQImportGuide::GetSourcesGeometry(BroadPresetType presetType)
{
    if (presetType == BroadPresetType::Game)
        return ui->page_PresetGame->GetSourcesGeometry();
    else if (presetType == BroadPresetType::VisibleRadio)
        return ui->page_PresetVisibleRadio->GetSourcesGeometry();
    else if (presetType == BroadPresetType::RelayBroadcast)
        return ui->page_PresetRelayBroad->GetSourcesGeometry();
    else
        return QVector<QPair<QString, QRectF>>();

}

bool AFQImportGuide::GetProfileFilePath(const char* folderpath, const char* collection, QString& filePath)
{
    char path[512]; 
    QString home = QDir::homePath();

    int ret = GetAppConfigPath(path, 512, folderpath);
    if (ret <= 0)
    {
        blog(LOG_WARNING, "Failed to get config path");
        return false;
    }
    else
    {
        // Check Dir Exists
        QDir dir(path);
        if (dir.exists())
            home = QString::fromUtf8(path);
    }

    QString Pattern = "(*.json)";
    filePath = OpenFile(this, QT_UTF8(Str("Basic.MainMenu.Profile.Import")), home, QT_UTF8(Str(collection)) + QString(" ") + Pattern);
    // Collection Name

    return true;
}

void AFQImportGuide::_LoadOBSSceneData()
{
    if (m_obsSceneDir.isEmpty() || m_obsSceneDir.isNull())
        return;
    if (m_soopSceneDir.isEmpty() || m_soopSceneDir.isNull())
        return;

    // Get File Name
    QFileInfo obsSceneFileInfo(m_obsSceneDir);
    std::string cur_name = config_get_string(USERCONFIG, "Basic", "SceneCollection");

    ImportersInit();
    
    json11::Json res;
    ImportSC(QT_TO_UTF8(m_obsSceneDir), cur_name, res);
}

void AFQImportGuide::_LoadFreecShotData()
{
    if (m_FreecShotProfileDir.isEmpty() || m_FreecShotProfileDir.isNull())
        return;

    const char* sceneCollection = config_get_string(USERCONFIG, "Basic", "SceneCollection");
    if (!sceneCollection)
        return;

    _ApplyFreecShotSharedLogic(m_FreecShotProfileDir, sceneCollection);
}

void AFQImportGuide::_UpdateWidgetStyle(const QList<QWidget*>& widgets, const QMap<QString, QVariant>& properties)
{
    for (QWidget* widget : widgets) 
    {
        if (widget) 
        {
            for (auto it = properties.begin(); it != properties.end(); ++it) 
            {
                widget->setProperty(it.key().toUtf8().constData(), it.value());
            }

            PolishStyleSheet(widget);
        }
    }
}

void AFQImportGuide::_LoadOBSProfileData()
{
    if (m_obsProfileDir.isEmpty() || m_obsProfileDir.isNull())
        return;
    if (m_soopProfileDir.isEmpty() || m_soopProfileDir.isNull())
        return;

#if 0 // ProgressBar
    ui->stackedWidget_ImportGuide->setCurrentIndex(2);
    ui->progressBar_Importing->setValue(0);
    
    m_pStepTimer = new QTimer(this);
    m_pStepTimer->setInterval(500);
    connect(m_pStepTimer, &QTimer::timeout, this, &AFQImportGuide::qslotOBSProfileCopyTriggered);
    m_pStepTimer->start();
#else // No ProgressBar
    m_selectedProfilePath = m_soopProfileDir.toStdString();

   // Copy File From OBS Profile Dir
    QFile::copy(m_obsProfileDir + "/basic.ini", m_soopProfileDir + "/basic.ini");
    QFile::copy(m_obsProfileDir + "/streamEncoder.json", m_soopProfileDir + "/streamEncoder.json");
    QFile::copy(m_obsProfileDir + "/recordEncoder.json", m_soopProfileDir + "/recordEncoder.json");

    QFile file(m_soopProfileDir + "/basic.ini");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString fileContent = in.readAll();
        file.close();

        const char* nvenc_encoder = AFEncoderUtil::EncoderAvailable("obs_nvenc_h264_tex") ? "obs_nvenc_h264_tex" : "ffmpeg_nvenc";
        QString originalString = "Encoder=jim_nvenc";
        QString newString = QString("Encoder=%1").arg(nvenc_encoder);

        if (fileContent.contains(originalString)) {
            fileContent.replace(originalString, newString);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                QTextStream out(&file);
                out << fileContent;
                file.close();
            }
        }
    }
#endif
}

void AFQImportGuide::_ApplyFreecShotSharedLogic(const QString& filePath, const std::string& collectionName)
{
    std::string errorParse;
    BPtr<char> file_data = os_quick_read_utf8_file(filePath.toStdString().c_str());
    if (!file_data)
        return;

}
