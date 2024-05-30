#include "CSelectSourceDialog.h"
#include "ui_select-source-dialog.h"

#ifdef _WIN32
#include <Windows.h>
#endif
#include <obs.h>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Locale/CLocaleTextManager.h"


#include "Application/CApplication.h"
#include "MainFrame/CMainFrame.h"
#include "Blocks/SceneSourceDock/CSceneSourceDockWidget.h"

#include "CSelectSourceButton.h"

inline bool checkFileExists(const QString& path)
{
    if (QFile::exists(path))
        return true;

    return false;
}


inline const char* getSourceInfoMessage(const char* id)
{
    if (0 == strcmp(id, "game_capture"))
        return Str("Basic.SelectedSourcePopup.Guide.Game_Capture");
    else if (0 == strcmp(id, "monitor_capture"))
        return Str("Basic.SelectedSourcePopup.Guide.Monitor_Capture");
    else if (0 == strcmp(id, "window_capture"))
        return Str("Basic.SelectedSourcePopup.Guide.Window_Capture");
    else if (0 == strcmp(id, "dshow_input"))
        return Str("Basic.SelectedSourcePopup.Guide.Dshow_Input");
    else if (0 == strcmp(id, "wasapi_input_capture"))
        return Str("Basic.SelectedSourcePopup.Guide.Wsaapi_Input_Capture");
    else if (0 == strcmp(id, "wasapi_output_capture"))
        return Str("Basic.SelectedSourcePopup.Guide.Wsaapi_Output_Capture");
    else if (0 == strcmp(id, "wasapi_process_output_capture"))
        return Str("Basic.SelectedSourcePopup.Guide.Wsaapi_Process_Output_Capture");
    else if (0 == strcmp(id, "browser_source"))
        return Str("Basic.SelectedSourcePopup.Guide.Browser_Source");
    else if (0 == strcmp(id, "image_source"))
        return Str("Basic.SelectedSourcePopup.Guide.Image_Source");
    else if (0 == strcmp(id, "slideshow"))
        return Str("Basic.SelectedSourcePopup.Guide.SlideShow");
    else if (0 == strcmp(id, "ffmpeg_source"))
        return Str("Basic.SelectedSourcePopup.Guide.FFmpeg_Source");
    else if (0 == strcmp(id, "text_gdiplus"))
        return Str("Basic.SelectedSourcePopup.Guide.Text_Gdiplus");
    else if (0 == strcmp(id, "color_source"))
        return Str("Basic.SelectedSourcePopup.Guide.Color_Source");
    else if (0 == strcmp(id, "scene"))
        return Str("Basic.SelectedSourcePopup.Guide.Scene");
    else if (0 == strcmp(id, "group"))
        return Str("Basic.SelectedSourcePopup.Guide.Group");
    return "";
}

AFQSelectSourceDialog::AFQSelectSourceDialog(QWidget *parent) :
    AFQRoundedDialogBase((QDialog*)parent),
    ui(new Ui::AFQSelectSourceDialog)
{
    ui->setupUi(this);

#ifdef __APPLE__
    ui->verticalSpacer_8->changeSize(20, 180);
#endif
    
    setFixedWidth(width());
    SetWidthFixed(true);

    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::reject);

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
    
    
    bool bGIF = false;
    const char* name;
    if (0 == id.compare("scene")) {
        name = Str("Basic.Scene");
    }
    else if (0 == id.compare("group")) {
        name = Str("Group");
    }
    else {
        name = obs_source_get_display_name(QT_TO_UTF8(id));
        bGIF = true;
    }

    ui->labelSourceName->setText(QT_UTF8(name));

    const char* sourceInfoMessage = getSourceInfoMessage(QT_TO_UTF8(convertID));
    ui->labelSourceInfo->setText(QT_UTF8(sourceInfoMessage));

	std::string absPath;
	GetDataFilePath("assets", absPath);

    QString guideImagePath;
    if (bGIF)
        guideImagePath = QString("%1/select-source-dialog/guide-image/guide_%2.gif").
                                  arg(absPath.data(), convertID);
    else
        guideImagePath = QString("%1/select-source-dialog/guide-image/guide_%2.png").
                                  arg(absPath.data(), convertID);

    if (m_movieGIF) {
        ui->labelSourceImage->setMovie(nullptr);
        m_movieGIF->stop();
        m_movieGIF = nullptr;
    }

    if (checkFileExists(guideImagePath)) {

        if (bGIF) {
            m_movieGIF = new QMovie(guideImagePath, QByteArray(), this);
            QSize scaledSize = m_movieGIF->scaledSize();
            scaledSize.scale(285, 160, Qt::IgnoreAspectRatio);
            m_movieGIF->setScaledSize(scaledSize);

            ui->labelSourceImage->setMovie(m_movieGIF);
            m_movieGIF->start();
        }
        else {
            ui->labelSourceImage->setPixmap(QPixmap(guideImagePath).
                scaled(QSize(285, 160), Qt::KeepAspectRatio));
        }
        ui->labelSourceImage->show();
    }
    else {
        ui->labelSourceImage->hide();
    }

    ui->stackedWidget->setCurrentIndex(0);

}

void AFQSelectSourceDialog::qSlotLeaveSelectSourceButton()
{
    //ui->stackedWidget->setCurrentIndex(1);
}

void AFQSelectSourceDialog::qslotAddSourceButtonTriggerd()
{
    AFQSelectSourceButton* button = qobject_cast<AFQSelectSourceButton*>(sender());
      
    m_sourceId = button->GetSourceId();

    done(DialogCode::Accepted);
}


bool AFQSelectSourceDialog::_FindEnableInputSource(const char* id) {

    QVector<QPair<bool, const char*>>::iterator it = m_vLoadInputSource.begin();
    for (; it != m_vLoadInputSource.end(); ++it) {
        if (0 == strcmp((*it).second, id)) {
            (*it).first = true;
            return true;
        }
    }
    return false;
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
            m_vLoadInputSource.push_back(qMakePair(false, unversioned_type));
        }
    }

    size_t arrSize = sizeof(g_pszScreenSectionSource) / sizeof(g_pszScreenSectionSource[0]);
    for (int i = 0; i < arrSize; i++) {
        if (_FindEnableInputSource(g_pszScreenSectionSource[i])) {
            m_vScreenSection.push_back(g_pszScreenSectionSource[i]);
        }
    }

    arrSize = sizeof(g_pszAudioSectionSource) / sizeof(g_pszAudioSectionSource[0]);
    for (int i = 0; i < arrSize; i++) {
        if (_FindEnableInputSource(g_pszAudioSectionSource[i])) {
            m_vAudioSection.push_back(g_pszAudioSectionSource[i]);
        }
    }

    arrSize = sizeof(g_pszEtcSectionSource) / sizeof(g_pszEtcSectionSource[0]);
    for (int i = 0; i < arrSize; i++) {
        if (_FindEnableInputSource(g_pszEtcSectionSource[i])) {
            m_vEtcSection.push_back(g_pszEtcSectionSource[i]);
        }
    }
    m_vEtcSection.push_back("scene");
    m_vEtcSection.push_back("group");

    // For Need load other Plugin
    //QVector<QPair<bool, const char*>>::iterator it = m_vLoadInputSource.begin();
    //for (; it != m_vLoadInputSource.end(); ++it) {
    //    if (false == (*it).first)
    //        m_vEtcSection.push_back((*it).second);
    //}
}


void AFQSelectSourceDialog::_MakeSourceButton()
{
    _CreateSourceSectionButton(m_vScreenSection, ui->gridLayoutScreen);

    _CreateSourceSectionButton(m_vAudioSection, ui->gridLayoutAudio);

    _CreateSourceSectionButton(m_vEtcSection, ui->gridLayoutEtc);

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

		row = index / 2;
		col = index % 2;

		layout->addWidget(button, row, col);

		connect(button, &AFQSelectSourceButton::clicked,
			this, &AFQSelectSourceDialog::qslotAddSourceButtonTriggerd);

		connect(button, &AFQSelectSourceButton::qSignalHoverButton,
			this, &AFQSelectSourceDialog::qSlotHoverSelectSourceButton);

		connect(button, &AFQSelectSourceButton::qSignalLeaveButton,
			this, &AFQSelectSourceDialog::qSlotLeaveSelectSourceButton);

		m_mapButton.insert(sourceId, button);

	}
	layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), row, col);
}