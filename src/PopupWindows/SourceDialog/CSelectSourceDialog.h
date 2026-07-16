#ifndef CSELECTSOURCEDIALOG_H
#define CSELECTSOURCEDIALOG_H

#include <QDialog>
#include <QMap>
#include <QGridLayout>
#include <QMovie>

#include "obs.hpp"

#include "UIComponent/CTopBaseWindow.h"

static const char* g_pszScreenSectionSource[] = {

#ifdef _WIN32
    "game_capture",
    "monitor_capture",
    "window_capture",
    "window_area_capture",
    "dshow_input",
#endif

#ifdef __APPLE__
    "syphon-input",
    "screen_capture",
    "av_capture_input"
#endif
};

static const char* g_pszAudioSectionSource[] = {
#ifdef _WIN32
    "wasapi_input_capture",
    "wasapi_output_capture",
    "wasapi_process_output_capture",
#endif

#ifdef __APPLE__
    "sck_audio_capture",
    "coreaudio_input_capture",
#endif
};

static const char* g_pszEtcSectionSource[] = {
#ifdef _WIN32
    "browser_source",
    "image_source",
    "slideshow",
    "ffmpeg_source",
    "ffmpeg_list_source",
    "text_gdiplus",
    "color_source",
    "soop_spout2",
    "soop_particle_effect_source",
    "painter_source",
#endif

#ifdef __APPLE__
    "browser_source",
    "image_source",
    "slideshow",
    "ffmpeg_source",
    "ffmpeg_list_source",
    "text_ft2_source",
    "color_source",
    "soop_particle_effect_source",
#endif
};

static const char* g_pszSoopAquaSectionSource[] = {
    "soop_chat_source_chat",
    "soop_chat_source_notice",
    "soop_chat_source_goal",
    "soop_chat_source_banner",
    "soop_chat_source_subtitle",
    "soop_chat_source_c_mission",
    "soop_chat_source_timer",
    "soop_chat_source_score",
    "soop_chat_source_anmSubtitle",
};

static const char* g_pszSoopEtcSectionSource[] = {
    "soop_videoballoon_source",
    "soop_directbroad_source",
    "soop_tv_cable_source",
    "soop_anivod_source",
    "soop_sportvod_source",
    "soop_dramavod_source",
    "soop_movievod_source",
};

static const char* g_pszSoopKBOSectionSource[] = {
    "soop_kbo_graphic_source_all",
    "soop_kbo_graphic_source_score",
    "soop_kbo_graphic_source_stadium",
    "soop_kbo_graphic_source_player",
    "soop_kbo_graphic_source_livetext",
};

static const char* g_pszSoopFootballSectionSource[] = {
    "soop_football_graphic_source_all",
    "soop_football_graphic_source_player",
    "soop_football_graphic_source_change",
    "soop_football_graphic_source_score",
    "soop_football_graphic_source_livetext",
};

static const char* g_pszLiveCommerceSectionSource[] = {
    "soop_commerce_source_goal",
    "soop_commerce_source_rank",
};

class AFQSelectSourceButton;

namespace Ui {
class AFQSelectSourceDialog;
};

class AFQSelectSourceDialog : public AFTTopBaseDialog
{
#pragma region class initializer, destructor
public:
    explicit AFQSelectSourceDialog(QWidget* parent = nullptr);
    ~AFQSelectSourceDialog();

#pragma endregion class initializer, destructor

#pragma region QT Field
        Q_OBJECT
signals:

public slots:
    void qSlotHoverSelectSourceButton(QString id);
    void qSlotLeaveSelectSourceButton();

private slots:
    void _qslotAddSourceButtonTriggerd();
    void _qslotBasicSourceTabButton();
    void _qslotSOOPSourceTabButton();
#pragma endregion QT Field

#pragma region public member func
private:
    void    _LoadInputSource();
    bool    _FindEnableInputSource(const char* id);
    void    _MakeInputSection(QVector<const char*>& vecSections, const char* inputSources[], int arrSize);
    void    _MakeSourceButton();

    void    _CreateSourceSectionButton(QVector<const char*> vSourceSection, QGridLayout* layout);

#pragma endregion public member func

#pragma region public member var
public:
    QString m_sourceId;
#pragma endregion public member var

#pragma region private member var
private:
    Ui::AFQSelectSourceDialog* ui;

    QPointer<QMovie> m_movieGIF = nullptr;

    QMap<const char*, AFQSelectSourceButton*> m_buttons;

    QVector<const char*>   m_screenSections;
    QVector<const char*>   m_audioSections;
    QVector<const char*>   m_etcSections;

    QVector<const char*>   m_soopAquaSections;
    QVector<const char*>   m_soopEtcSections;
    QVector<const char*>   m_soopKboSections;
    QVector<const char*>   m_soopFootballSections;
    QVector<const char*>   m_soopLiveCommerceSections;

    QVector<QPair<bool,const char*>>    m_loadInputSources;

#pragma endregion private member var
};

#endif // CSELECTSOURCEDIALOG_H
