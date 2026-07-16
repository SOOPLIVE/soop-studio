#pragma once

#include "obs.hpp"
#include <QPointer>
#include "UIComponent/CTopBaseWindow.h"
#include "ui_dowoomi-mood-check.h"

namespace Ui {
    class AFQDowoomiMoodCheckDialog;
}

enum mood_check_theme_type {
    basic_theme = 1,
    sticker_notes_theme,
    talk_balloon_theme,
    simple_theme,
    expression_theme,
    pixel_theme,
    award_led_theme,
    award_led_blue_theme,
    award_led_pink_theme,
    award_led_gold_theme
};

typedef struct sMOOD_CHECK_THEME_INFO {
    int theme_index = basic_theme;
    std::string theme_type = "";
    std::string theme_log_type = "";

    sMOOD_CHECK_THEME_INFO() 
        : theme_index(0)
        , theme_type("")
        , theme_log_type("")
    {}

    sMOOD_CHECK_THEME_INFO(int _idx, std::string _type, std::string _theme_log)
        : theme_index(_idx)
        , theme_type(_type)
        , theme_log_type(_theme_log)
    {}
} tMOOD_CHECK_THEME_INFO;
 
class AFQDowoomiMoodCheckDialog : public AFTTopBaseDialog
{
    Q_OBJECT
public:
    AFQDowoomiMoodCheckDialog(QWidget* parent, OBSSource source);
    ~AFQDowoomiMoodCheckDialog();

private slots:
    void qslotShutdownSourceNotVisibleChecked(bool checked);
    void qslotFreecshotControlAudio(bool checked);
    void qslotSetThemeClicked();
    void qslotInteractionClicked();
    void qslotRefreshClicked();
    void qslotBrowserSizeClicked();

private:
    void _GetThemeInfo();
    void _UpdateScoreInfo();
    static void _SourceRemoved(void* data, calldata_t* params);

private:
    Ui::AFQDowoomiMoodCheckDialog* ui = nullptr;
    OBSWeakSource m_weakSource = nullptr;

    QString m_sourceName;

    using properties_delete_t = decltype(&obs_properties_destroy);
    using properties_t = std::unique_ptr<obs_properties_t, properties_delete_t>;
    properties_t m_props;

    OBSSignal removeSignal;
};
