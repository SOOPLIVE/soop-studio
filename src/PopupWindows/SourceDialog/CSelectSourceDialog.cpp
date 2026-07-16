#include "CSelectSourceDialog.h"
#include "ui_select-source-dialog.h"

#include <obs.h>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#include "Application/CApplication.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Source/CSource.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/Output/COutput.h"

#include "Blocks/SceneSourceDock/CSceneSourceDockWidget.h"

#include "CSelectSourceButton.h"
#include "PopupWindows/SettingPopup/CAddStreamWidget.h"

#include "CoreModel/OBSOutput/COutput.h"

inline bool checkFileExists(const QString& path)
{
    if (QFile::exists(path))
        return true;

    return false;
}

inline const char* getSourceInfoMessage(const char* id)
{
    struct SourceGuideInfo {
        const char* sourceId;
        const char* localeKey;
    };

    static const SourceGuideInfo table[] = {
        //
        {"game_capture",                   "Basic.SelectedSourcePopup.Guide.Game_Capture"},
        {"monitor_capture",                "Basic.SelectedSourcePopup.Guide.Monitor_Capture"},
        {"window_capture",                 "Basic.SelectedSourcePopup.Guide.Window_Capture"},
        {"window_area_capture",            "Basic.SelectedSourcePopup.Guide.Window_Area_Capture"},
        {"dshow_input",                    "Basic.SelectedSourcePopup.Guide.Dshow_Input"},
        {"wasapi_input_capture",           "Basic.SelectedSourcePopup.Guide.Wsaapi_Input_Capture"},
        {"wasapi_output_capture",          "Basic.SelectedSourcePopup.Guide.Wsaapi_Output_Capture"},
        {"wasapi_process_output_capture",  "Basic.SelectedSourcePopup.Guide.Wsaapi_Process_Output_Capture"},
        {"browser_source",                 "Basic.SelectedSourcePopup.Guide.Browser_Source"},
        {"image_source",                   "Basic.SelectedSourcePopup.Guide.Image_Source"},
        {"slideshow",                      "Basic.SelectedSourcePopup.Guide.SlideShow"},
        {"ffmpeg_source",                  "Basic.SelectedSourcePopup.Guide.FFmpeg_Source"},
        {"ffmpeg_list_source",             "Basic.SelectedSourcePopup.Guide.FFmpeg_List_Source"},
        {"text_gdiplus",                   "Basic.SelectedSourcePopup.Guide.Text_Gdiplus"},
        {"color_source",                   "Basic.SelectedSourcePopup.Guide.Color_Source"},
        {"soop_spout2",                    "Basic.SelectedSourcePopup.Guide.Spout2"},
        {"scene",                          "Basic.SelectedSourcePopup.Guide.Scene"},
        {"group",                          "Basic.SelectedSourcePopup.Guide.Group"},
        {"soop_particle_effect_source",    "Basic.SelectedSourcePopup.Guide.ParticleEffect"},
        {"painter_source",                 "Basic.SelectedSourcePopup.Guide.PainterSource"},

        // SOOP SOURCE
        {"soop_directbroad_source",        "Basic.SelectedSourcePopup.Guide.DirectBroad"},
        {"soop_tv_cable_source",           "Basic.SelectedSourcePopup.Guide.TvLive"},
        {"soop_anivod_source",             "Basic.SelectedSourcePopup.Guide.Animation"},
        {"soop_sportvod_source",           "Basic.SelectedSourcePopup.Guide.SportVod"},
        {"soop_dramavod_source",           "Basic.SelectedSourcePopup.Guide.DramaVod"},
        {"soop_movievod_source",           "Basic.SelectedSourcePopup.Guide.MovieVod"},
        {"soop_chat_source_chat",          "Basic.SelectedSourcePopup.Guide.Chat"},
        {"soop_chat_source_notice",        "Basic.SelectedSourcePopup.Guide.Notice"},
        {"soop_chat_source_goal",          "Basic.SelectedSourcePopup.Guide.Goal"},
        {"soop_chat_source_banner",        "Basic.SelectedSourcePopup.Guide.Banner"},
        {"soop_chat_source_subtitle",      "Basic.SelectedSourcePopup.Guide.SubTitle"},
        {"soop_chat_source_c_mission",     "Basic.SelectedSourcePopup.Guide.Mission"},
        {"soop_chat_source_timer",         "Basic.SelectedSourcePopup.Guide.Timer"},
        {"soop_chat_source_score",         "Basic.SelectedSourcePopup.Guide.Score"},
        {"soop_chat_source_anmSubtitle",   "Basic.SelectedSourcePopup.Guide.AnimationSubtitle"},
        {"soop_chat_source_mood_check",    "Basic.SelectedSourcePopup.Guide.MoodCheck"},
        {"soop_kbo_graphic_source_all",    "Basic.SelectedSourcePopup.Guide.KBO.All"},
        {"soop_kbo_graphic_source_score",  "Basic.SelectedSourcePopup.Guide.KBO.Score"},
        {"soop_kbo_graphic_source_stadium","Basic.SelectedSourcePopup.Guide.KBO.Stadium"},
        {"soop_kbo_graphic_source_player", "Basic.SelectedSourcePopup.Guide.KBO.Player"},
        {"soop_kbo_graphic_source_livetext","Basic.SelectedSourcePopup.Guide.KBO.LiveText"},
        {"soop_football_graphic_source_all",    "Basic.SelectedSourcePopup.Guide.FOOTBALL.All"},
        {"soop_football_graphic_source_player", "Basic.SelectedSourcePopup.Guide.FOOTBALL.Player"},
        {"soop_football_graphic_source_change","Basic.SelectedSourcePopup.Guide.FOOTBALL.Change"},
        {"soop_football_graphic_source_score",  "Basic.SelectedSourcePopup.Guide.FOOTBALL.Score"},
        {"soop_football_graphic_source_livetext","Basic.SelectedSourcePopup.Guide.FOOTBALL.LiveText"},
        {"soop_commerce_source_rank",      "Basic.SelectedSourcePopup.Guide.CommerceRank"},
        {"soop_commerce_source_goal",      "Basic.SelectedSourcePopup.Guide.CommerceGoal"},
        {"soop_videoballoon_source",       "Basic.SelectedSourcePopup.Guide.VideoBalloon"},
        {"soop_aimanager_source",          "Basic.SelectedSourcePopup.Guide.AIManager"},
    };

    for (const auto& e : table) {
        if (strcmp(id, e.sourceId) == 0)
            return Str(e.localeKey);
    }

    return "";
}

AFQSelectSourceDialog::AFQSelectSourceDialog(QWidget *parent) :
    AFTTopBaseDialog((QDialog*)parent),
    ui(new Ui::AFQSelectSourceDialog)
{
    ui->setupUi(this);

    setWindowTitle(QTStr("AddSource"));

    ui->stackedWidget->setCurrentIndex(1);
    ui->stackedSourceType->setCurrentIndex(0);
    
    ui->frameSoopSource->hide();

#ifdef _WIN32
    ui->titleFrame->setProperty("MoveInAllArea", true);
#elif defined(__APPLE__)
    setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    
    ui->verticalSpacer_4->changeSize(20, 180);
    ui->titleFrame->hide();
#endif

    SetWidthResizeEnabled(false);

    connect(ui->pushButton_Close, &QPushButton::clicked, this, &QDialog::reject);

    ui->label_EmptyGuide->setText(Str("Basic.SelectedSourcePopup.Guide"));

    std::string absPath;
    GetDataFilePath("assets", absPath);
    QString emptySourceImagePath = QString("%1/select-source-dialog/empty_source_guide.svg").arg(absPath.data());
    
    ui->label_EmptySource->setPixmap(QPixmap(emptySourceImagePath).scaled(QSize(167, 160), 
                                     Qt::KeepAspectRatio));
    
    _LoadInputSource();

    _MakeSourceButton();
}

AFQSelectSourceDialog::~AFQSelectSourceDialog()
{
    delete ui;
}

void AFQSelectSourceDialog::qSlotHoverSelectSourceButton(QString id)
{
    QString convertID = id;
    
#ifdef __APPLE__
    if (convertID == "syphon-input")
        convertID = "game_capture";
    else if (convertID == "screen_capture")
        convertID = "monitor_capture";
    else if (convertID == "av_capture_input")
        convertID = "dshow_input";
    else if (convertID == "sck_audio_capture")
        convertID = "wasapi_input_capture";
    else if (convertID == "coreaudio_input_capture")
        convertID = "wasapi_input_capture";
    else if (convertID == "text_ft2_source")
        convertID = "text_gdiplus";
#endif
    
    auto matchId = [&](std::initializer_list<const char*> list) {
        return std::any_of(list.begin(), list.end(),
            [&](const char* s) { return id == s; });
    };
   
    bool bGIF = false;
    QString sourceName;
    if (0 == id.compare("scene")) {
        sourceName = Str("Basic.Scene");
    }
    else if (0 == id.compare("group")) {
        sourceName = Str("Group");
    }
    else {
        sourceName = obs_source_get_display_name(QT_TO_UTF8(id));
        bGIF = true;
    }

    //
    bGIF = !matchId({
        "soop_anivod_source",
        "soop_sportvod_source",
        "soop_movievod_source",
        "soop_tv_cable_source",
        "soop_dramavod_source",
        "soop_kbo_graphic_source_score",
        "soop_commerce_source_goal",
        "soop_commerce_source_rank",
        "soop_kbo_graphic_source_stadium",
        "soop_kbo_graphic_source_player",
        "soop_kbo_graphic_source_livetext",
        "soop_kbo_graphic_source_all",
        "soop_football_graphic_source_player",
        "soop_football_graphic_source_change",
        "soop_football_graphic_source_score",
        "soop_football_graphic_source_livetext",
        "soop_football_graphic_source_all",
        "soop_chat_source_mood_check",
        "scene",
        "group"
        });

    /*if (0 == id.compare("soop_chat_source_mood_check")) {
        sourceName = "SARSA " + sourceName;
    }*/
    ui->label_SourceName->setText(sourceName);

    bool isSoopSource = AFSourceUtil::IsSoopMediaSource(QT_TO_UTF8(convertID));
    if (isSoopSource) {

        QString formatSoopSource = QTStr("Basic.SelectedSourcePopup.OnlySOOPSourceMessage");
        QString soopSourceInfo = QString(formatSoopSource).arg(sourceName);
        ui->pushButton_SoopSourceInfo->SetExplanationText(soopSourceInfo, ENUM_TOOLTIP_POSITION::TopCenter);

        ui->frameSoopSource->show();
    }
    else {
        ui->frameSoopSource->hide();
    }

    const char* sourceInfoMessage = getSourceInfoMessage(QT_TO_UTF8(convertID));
    ui->labelSourceInfo->setText(QT_UTF8(sourceInfoMessage));

    bool localeChangeGuide = matchId({
        "soop_chat_source_chat",
        "soop_chat_source_goal",
        "soop_chat_source_notice",
        "soop_chat_source_score",
        "soop_chat_source_subtitle",
        "soop_chat_source_c_mission",
        "soop_commerce_source_goal",
        "soop_commerce_source_rank"
        });

	std::string absPath;
	GetDataFilePath("assets", absPath);

    QString guideLangPath;
    QString locale = QString::fromStdString(LOCALE_CONTEXT.GetCurrentLocaleStr());
    if (localeChangeGuide && locale == "ko-KR") {
        guideLangPath = QString("%1/select-source-dialog/guide-image/ko-KR").arg(absPath.data());
    }
    else {
        guideLangPath = QString("%1/select-source-dialog/guide-image").arg(absPath.data());
    }

    QString guideImagePath;
    if (bGIF)
        guideImagePath = QString("%1/guide_%2.gif").
                                  arg(guideLangPath, convertID);
    else
        guideImagePath = QString("%1/guide_%2.png").
                                  arg(guideLangPath, convertID);

    if (m_movieGIF) {
        ui->label_SourceImage->setMovie(nullptr);
        m_movieGIF->stop();
        m_movieGIF = nullptr;
    }

    if (checkFileExists(guideImagePath)) {

        if (bGIF) {
            m_movieGIF = new QMovie(guideImagePath, QByteArray(), this);
            QSize scaledSize = m_movieGIF->scaledSize();
            scaledSize.scale(300, 300, Qt::IgnoreAspectRatio);
            m_movieGIF->setScaledSize(scaledSize);

            ui->label_SourceImage->setMovie(m_movieGIF);
            m_movieGIF->start();
        }
        else {
            ui->label_SourceImage->setPixmap(QPixmap(guideImagePath).
                scaled(QSize(300, 300), Qt::KeepAspectRatio));
        }
        ui->label_SourceImage->show();
    }
    else {
        ui->label_SourceImage->hide();
    }

    ui->stackedWidget->setCurrentIndex(0);

}

void AFQSelectSourceDialog::qSlotLeaveSelectSourceButton()
{
    //ui->stackedWidget->setCurrentIndex(1);
}

void AFQSelectSourceDialog::_qslotAddSourceButtonTriggerd()
{
    AFQSelectSourceButton* button = qobject_cast<AFQSelectSourceButton*>(sender());

    m_sourceId = button->GetSourceId();

    auto isKboSource = [&](const QString& id) -> bool {
        for (const char* kboId : g_pszSoopKBOSectionSource) {
            if (id == kboId)
                return true;
        }
        return false;
        };

    auto isFootballSource = [&](const QString& id) -> bool {
        for (const char* fbId : g_pszSoopFootballSectionSource) {
            if (id == fbId)
                return true;
        }
        return false;
        };

    bool needGeoCheck = isKboSource(m_sourceId);
    bool needGeoCheck_2 = isFootballSource(m_sourceId);
    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    if ((needGeoCheck || needGeoCheck_2) && broadInfo)
    {
        QString nation = broadInfo->GetUserNation();

        bool isKorea =
            (nation == "kr") ||
            (nation == "ko_kr") ||
            (nation == "ko");

        if (!isKorea) {
            QString msg = QTStr("Basic.GeoBlock.Blocked");
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", msg, false, true, "", 0, 0, "type1");
            return;
        }
    }

    done(DialogCode::Accepted);
}

void AFQSelectSourceDialog::_qslotBasicSourceTabButton()
{
    if (ui->stackedSourceType->currentIndex() == 0)
        return;

    ui->pushButton_BasicSource->setProperty("tabButtonType", "clicked");
    ui->pushButton_SoopSource->setProperty("tabButtonType", "unclicked");

    ui->pushButton_BasicSource->style()->unpolish(ui->pushButton_BasicSource);
    ui->pushButton_BasicSource->style()->polish(ui->pushButton_BasicSource);

    ui->pushButton_SoopSource->style()->unpolish(ui->pushButton_SoopSource);
    ui->pushButton_SoopSource->style()->polish(ui->pushButton_SoopSource);

    ui->stackedWidget->setCurrentIndex(1);
    ui->stackedSourceType->setCurrentIndex(0);
}

void AFQSelectSourceDialog::_qslotSOOPSourceTabButton()
{
    //
    if (false == AUTH_CONTEXT.IsSoopRegistered())
    {
        if (AFOutputUtil::IsStreamActive())
        {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr,
                                       "", QTStr("Simulcast.Toggle.Refuse"), false, true);
            return;
        }

        bool ret = MAINFRAME->AddStreamAccount(this, PLATFORM_SOOP);
        if (!ret) {
            return;
        }
        MAIN_OUTPUT->SetStreamingOutput();
        MAINFRAME->LoadAccounts();
    }

    if (ui->stackedSourceType->currentIndex() == 1)
        return;

    ui->pushButton_BasicSource->setProperty("tabButtonType", "unclicked");
    ui->pushButton_SoopSource->setProperty("tabButtonType", "clicked");

    ui->pushButton_BasicSource->style()->unpolish(ui->pushButton_BasicSource);
    ui->pushButton_BasicSource->style()->polish(ui->pushButton_BasicSource);

    ui->pushButton_SoopSource->style()->unpolish(ui->pushButton_SoopSource);
    ui->pushButton_SoopSource->style()->polish(ui->pushButton_SoopSource);

    ui->stackedWidget->setCurrentIndex(1);
    ui->stackedSourceType->setCurrentIndex(1);
}

bool AFQSelectSourceDialog::_FindEnableInputSource(const char* id) {

    QVector<QPair<bool, const char*>>::iterator it = m_loadInputSources.begin();
    for (; it != m_loadInputSources.end(); ++it) {
        if (0 == strcmp((*it).second, id)) {
            (*it).first = true;
            return true;
        }
    }
    return false;
}

void AFQSelectSourceDialog::_MakeInputSection(QVector<const char*>& vecSections, 
                                              const char* inputSources[], int arrSize)
{
    for (int i = 0; i < arrSize; i++) {
        if (_FindEnableInputSource(inputSources[i])) {
            vecSections.push_back(inputSources[i]);
        }
    }
}

void AFQSelectSourceDialog::_LoadInputSource()
{
    size_t idx = 0;
    const char* unversioned_type;
    const char* type;

    while (obs_enum_input_types2(idx++, &type, &unversioned_type)) {
        const char* name = obs_source_get_display_name(type);
        uint32_t caps = obs_get_source_output_flags(type);

        if ((caps & OBS_SOURCE_CAP_DISABLED) != 0)
            continue;

        if ((caps & OBS_SOURCE_DEPRECATED) == 0) {
            m_loadInputSources.push_back(qMakePair(false, unversioned_type));
        }
     }

    int arrSize = sizeof(g_pszScreenSectionSource) / sizeof(g_pszScreenSectionSource[0]);
    _MakeInputSection(m_screenSections, g_pszScreenSectionSource, arrSize);

    arrSize = sizeof(g_pszAudioSectionSource) / sizeof(g_pszAudioSectionSource[0]);
    _MakeInputSection(m_audioSections, g_pszAudioSectionSource, arrSize);

    arrSize = sizeof(g_pszEtcSectionSource) / sizeof(g_pszEtcSectionSource[0]);
    _MakeInputSection(m_etcSections, g_pszEtcSectionSource, arrSize);
    m_etcSections.push_back("scene");
    m_etcSections.push_back("group");

    // SOOP Source
	arrSize = sizeof(g_pszSoopAquaSectionSource) / sizeof(g_pszSoopAquaSectionSource[0]);
	_MakeInputSection(m_soopAquaSections, g_pszSoopAquaSectionSource, arrSize);
    
    // sarsa check
    QString locale = QString::fromStdString(LOCALE_CONTEXT.GetCurrentLocaleStr());
    if (locale == "ko-KR")
    {
        AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
        if (broadInfo) {
            if (broadInfo->SarsaUser()) {
                if (_FindEnableInputSource("soop_chat_source_mood_check")) {
                    m_soopAquaSections.push_back("soop_chat_source_mood_check");
                }
            }
            if (broadInfo->IsAIManager()) {
                if (_FindEnableInputSource("soop_aimanager_source")) {
                    m_soopAquaSections.push_back("soop_aimanager_source");
                }
            }
        }
    }

	arrSize = sizeof(g_pszSoopEtcSectionSource) / sizeof(g_pszSoopEtcSectionSource[0]);
	_MakeInputSection(m_soopEtcSections, g_pszSoopEtcSectionSource, arrSize);

	arrSize = sizeof(g_pszSoopKBOSectionSource) / sizeof(g_pszSoopKBOSectionSource[0]);
	_MakeInputSection(m_soopKboSections, g_pszSoopKBOSectionSource, arrSize);

    arrSize = sizeof(g_pszSoopFootballSectionSource) / sizeof(g_pszSoopFootballSectionSource[0]);
    _MakeInputSection(m_soopFootballSections, g_pszSoopFootballSectionSource, arrSize);    

	arrSize = sizeof(g_pszLiveCommerceSectionSource) / sizeof(g_pszLiveCommerceSectionSource[0]);
	_MakeInputSection(m_soopLiveCommerceSections, g_pszLiveCommerceSectionSource, arrSize);

}


void AFQSelectSourceDialog::_MakeSourceButton()
{
    ui->pushButton_BasicSource->setProperty("tabButtonType", "clicked");
    ui->pushButton_SoopSource->setProperty("tabButtonType", "uncicked");

    PolishStyleSheet(ui->pushButton_BasicSource);
    PolishStyleSheet(ui->pushButton_SoopSource);

    connect(ui->pushButton_BasicSource, &QPushButton::clicked, this, &AFQSelectSourceDialog::_qslotBasicSourceTabButton);
    connect(ui->pushButton_SoopSource, &QPushButton::clicked, this, &AFQSelectSourceDialog::_qslotSOOPSourceTabButton);

    _CreateSourceSectionButton(m_screenSections, ui->gridLayoutScreen);
    _CreateSourceSectionButton(m_audioSections, ui->gridLayoutAudio); 
    _CreateSourceSectionButton(m_etcSections, ui->gridLayoutEtc);

    _CreateSourceSectionButton(m_soopAquaSections, ui->gridLayoutAqua);
    _CreateSourceSectionButton(m_soopEtcSections, ui->gridLayoutSoopEtc);
    _CreateSourceSectionButton(m_soopKboSections, ui->gridLayoutKBO);
    _CreateSourceSectionButton(m_soopFootballSections, ui->gridLayoutFootball);

    _CreateSourceSectionButton(m_soopLiveCommerceSections, ui->gridLayoutLiveCommerce);

    ui->scrollAreaWidgetContents->adjustSize();
}

void AFQSelectSourceDialog::_CreateSourceSectionButton(QVector<const char*> vSourceSection, QGridLayout* layout)
{
	int row = 0;
	int col = 0;
	int index = 0;
	QVector<const char*>::iterator it = vSourceSection.begin();
	for (; it < vSourceSection.end(); ++it, ++index) {

		const char* sourceId = (*it);

		const char* name;
		if (0 == strcmp(sourceId, "scene"))
			name = Str("Basic.Scene");
		else if (0 == strcmp(sourceId, "group"))
			name = Str("Group");
		else
			name = obs_source_get_display_name(sourceId);

		AFQSelectSourceButton* button = new AFQSelectSourceButton(sourceId, name, this);

        button->setObjectName(QString("pushButton_%1").arg(sourceId));

		row = index / 2;
		col = index % 2;

		layout->addWidget(button, row, col);

		connect(button, &AFQSelectSourceButton::clicked,
			this, &AFQSelectSourceDialog::_qslotAddSourceButtonTriggerd);

		connect(button, &AFQSelectSourceButton::qSignalHoverButton,
			this, &AFQSelectSourceDialog::qSlotHoverSelectSourceButton);

		connect(button, &AFQSelectSourceButton::qSignalLeaveButton,
			this, &AFQSelectSourceDialog::qSlotLeaveSelectSourceButton);

		m_buttons.insert(sourceId, button);

	}
	layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), row, col);
}
