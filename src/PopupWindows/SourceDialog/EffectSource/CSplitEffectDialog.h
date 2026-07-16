#pragma once
#include "UIComponent/CTopBaseWindow.h"
#include "obs.hpp"
#include <QLabel>

#define SPLIT_FILTER_ID         "soop_shader_filter"
#define PRESET_PROPERTY         "preset"
#define PRE_SPLIT_TYPE_PROPERTY "pre_split_effect_type"
#define SAVED_SPLIT_FILTER_NAME "split_filter_name"

namespace Ui {
    class AFQSplitEffectDialog;
}

class AFQSplitEffectDialog : public AFTTopBaseDialog
{
    Q_OBJECT

public:
    enum class SplitType {
        None,
        TwoWay,
        ThreeWay,
        FourWay,
        FiveWay,
        SixWay,
        HorizontalFlip,
        ColoredSplit
    };

    AFQSplitEffectDialog(QWidget* parent, OBSSource source);
    ~AFQSplitEffectDialog();

    static void SetFilterData(SplitType type, OBSSource source, bool fromToolBar = false);
    static SplitType GetPresetType(const char* type);

signals:
    void qsignalSplitFilterActivated();

private slots:
    void _qslotClickedSplit(SplitType type);

private:
    void _Init();
    void _SetCurrentCheckType(SplitType type, bool checked = false);
    void _UpdateUiStyle(SplitType type, bool checked);
    void _UnCheckAllFilterUi();

public:
    void RefreshSplitFilterUI();

private:
    Ui::AFQSplitEffectDialog* ui = nullptr;
    OBSWeakSource m_weakSource = nullptr;

    SplitType m_currentSplitType = SplitType::None;
};
