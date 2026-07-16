#pragma once

#include "obs.hpp"
#include <QPointer>

#include "UIComponent/CTopBaseWindow.h"
#include "ui_aqua-subtitle-source.h"
#include "CDowoomiPopup.h"

namespace Ui {
    class AFQAquaSubtitleDialog;
}

class AFQAquaSubtitleDialog : public AFTTopBaseDialog
{
    Q_OBJECT

public:
    AFQAquaSubtitleDialog(QWidget* parent, OBSSource source);
    ~AFQAquaSubtitleDialog();

private slots:
    void _qslotSubTitleSettingButtonClicked();
    void _qslotDetailSettingButtonClicked();
    void _qslotFontTransparencySliderChanged(int value);
    void _qslotFontTransparencySpinboxChanged(int value);
    void _qslotFontOutlinSizeSliderChanged(int value);
    void _qslotFontOutlinSizeSpinboxChanged(int value);
    void _qslotBackgroundTransparencySliderChanged(int value);
    void _qslotBackgroundTransparencySpinboxChanged(int value);
    void _qslotAnimationSpeedSliderChanged(int value);
    void _qslotAnimationSpeedSpinboxChanged(int value);
    void _qslotAnimationRepeatChanged(int value);

    void _qslotAnimeThemeButtonClicked(int index);
    void _qslotStyleIndexChanged(int index);
    void _qslotClickedButtonBox(QAbstractButton* button);

    void _qslotFontColorChanged();
    void _qslotOutlineColorChanged();
    void _qslotBackgroundColorChanged();


private:
    void _InitUIComponent();
    void _SetColorLabel(QLabel* colorLabel, QColor color);
    void _SetThemeButtonWidget(int themeIdx);
    void _RefreshStyle();
    void _RefreshSubtitleInfo(bool init = false);
    void _SetFontOptionEnableUI(int themeIdx);
    void _SetAnimationEnableUI(int useAnimation);
    void _ResetAnimeSubtitleInfo();
    void _SetAnimeSubtitleThemeUI(int themeIdx, bool reset = false);
    void _UpdateAnimeSubtitleInfo();

    static void _SourceRemoved(void* data, calldata_t* params);

private:
    Ui::AFQAquaSubtitleDialog* ui = nullptr;
    OBSWeakSource m_weakSource = nullptr;

    using properties_delete_t = decltype(&obs_properties_destroy);
    using properties_t = std::unique_ptr<obs_properties_t, properties_delete_t>;
    properties_t m_props;

    int m_checkedThemeIdx;
    QList<AFQHoverWidget*> m_themeButtons;
    QList<QLabel*> m_themeButtonsChecked;

    QColor m_fontColor;
    QColor m_outlineColor;
    QColor m_backgroundColor;

    OBSSignal removeSignal;
};

