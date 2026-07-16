#pragma once

#include "obs.hpp"
#include "UIComponent/CTopBaseWindow.h"
#include "Blocks/CBlockManager.h"

namespace Ui {
    class AFQMissionChallengeDialog;
}

class AFQMissionChallengeDialog : public AFTTopBaseDialog
{
#pragma region QT Field, CTOR/DTOR

    Q_OBJECT
public:
    explicit AFQMissionChallengeDialog(QWidget* parent, OBSSource source);
    ~AFQMissionChallengeDialog();

private slots:
    void _qslotClickedRefresh();
    void _qslotThemeChanged(int);
    void _qslotSortOrderChanged(int);
    void _qslotOpacitySliderChanged(int);
    void _qslotOpacitySpinBoxChanged(int);
    void _qslotVolumeMuteClicked(bool checked);
    void _qslotVolumeSliderChanged(int);
    void _qslotVolumeSpinBoxChanged(int);
    void _qslotShowActiveOnlyClicked(bool checked);
    void _qslotExitOnInvisibleClicked(bool checked);
    void _qslotBrowserSizeClicked();
#pragma endregion QT Field, CTOR/DTOR

#pragma region private func
private:
    void _Init();
    void _RequestStringQuery(const char* name, const char* value);
    void _RequestIntQuery(const char* name, const int value);
    void _RequestBoolQuery(const char* name, const bool value);
#pragma endregion private func

#pragma region private var
private:
    Ui::AFQMissionChallengeDialog* ui;
    OBSWeakSource m_weakSource = nullptr;

    using properties_delete_t = decltype(&obs_properties_destroy);
    using properties_t = std::unique_ptr<obs_properties_t, properties_delete_t>;
    properties_t m_props;
#pragma endregion private var
};
