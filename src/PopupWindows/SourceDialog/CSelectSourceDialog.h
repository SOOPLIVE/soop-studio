#ifndef CSELECTSOURCEDIALOG_H
#define CSELECTSOURCEDIALOG_H

#include <QDialog>
#include <QMap>
#include <QGridLayout>
#include <QMovie>

#include "obs.hpp"

#include "UIComponent/CRoundedDialogBase.h"

static const char* g_pszScreenSectionSource[] = {

#ifdef _WIN32
    "game_capture",
    "monitor_capture",
    "window_capture",
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
    "text_gdiplus",
    "color_source",
#endif

#ifdef __APPLE__
    "browser_source",
    "image_source",
    "slideshow",
    "ffmpeg_source",
    "text_ft2_source",
    "color_source",
#endif
};

class AFQSelectSourceButton;

namespace Ui {
class AFQSelectSourceDialog;
}

//class AFQSelectSourceDialog : public QDialog
class AFQSelectSourceDialog : public AFQRoundedDialogBase
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
    void qslotAddSourceButtonTriggerd();
#pragma endregion QT Field

#pragma region public member func
private:
    void    _LoadInputSource();
    bool    _FindEnableInputSource(const char* id);
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

    QMap<const char*, AFQSelectSourceButton*> m_mapButton;

    QVector<const char*>   m_vScreenSection;
    QVector<const char*>   m_vAudioSection;
    QVector<const char*>   m_vEtcSection;

    QVector<QPair<bool,const char*>>    m_vLoadInputSource;

#pragma endregion private member var
};

#endif // CADDSOURCEDIALOG_H
