#pragma once

#include "obs.hpp"
#include "UIComponent/CTopBaseWindow.h"
#include "Blocks/CBlockManager.h"

namespace Ui {
    class AFQMissionDonationRankDialog;
}

class AFQMissionDonationRankDialog : public AFTTopBaseDialog
{
#pragma region QT Field, CTOR/DTOR

    Q_OBJECT
public:
    explicit AFQMissionDonationRankDialog(QWidget* parent, OBSSource source, int type);
    ~AFQMissionDonationRankDialog();

private slots:
    void _qslotClickedRefresh();
    void _qslotShowDonorsOnlyClicked(bool checked);
    void _qslotExitOnInvisibleClicked(bool checked);
    void _qslotBrowserSizeClicked();
#pragma endregion QT Field, CTOR/DTOR

protected:
    virtual void showEvent(QShowEvent* event) override;

#pragma region private func
private:
    void _Init();
    static void _SourceRemoved(void* data, calldata_t* params);
#pragma endregion private func

#pragma region private var
private:
    Ui::AFQMissionDonationRankDialog* ui;
    OBSWeakSource m_weakSource = nullptr;

    using properties_delete_t = decltype(&obs_properties_destroy);
    using properties_t = std::unique_ptr<obs_properties_t, properties_delete_t>;
    properties_t m_props;

    OBSSignal removeSignal;
#pragma endregion private var
};
