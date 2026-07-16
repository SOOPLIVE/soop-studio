#pragma once

#include "obs.hpp"
#include <QPointer>
#include <QTimer>
#include "UIComponent/CTopBaseWindow.h"
#include "ui_dowoomi-score.h"
#include "CDowoomiPopup.h"

namespace Ui {
    class AFQDowoomiScoreDialog;
}


class AFQDowoomiScoreDialog : public AFTTopBaseDialog
{
    Q_OBJECT

public:
    AFQDowoomiScoreDialog(QWidget* parent, OBSSource source);
    ~AFQDowoomiScoreDialog();

private slots:
    void qslotClearAll();
    void qslotNameChanged(const QString& text);
    void qslotScoreChanged(int score);
    void qslotUseSet(bool checked);
    void qslotSetChanged(int set);
    void qslotUseTimer(bool checked);
    void qslotTimerPlay();
    void qslotTimerPause();
    void qslotTimerReset();
    void qslotSetThema();

    void qslotHelpPopupClicked();
    void qslotBrowserSizeClicked();

private:
    void _GetScoreInfo();
    void _UpdateScoreInfo();
    void _SetupAutoRefresh();
    void _HandleAutoRefresh();

    static void _SourceRemoved(void* data, calldata_t* params);

private:
    Ui::AFQDowoomiScoreDialog* ui = nullptr;
    OBSWeakSource m_weakSource = nullptr;

    using properties_delete_t = decltype(&obs_properties_destroy);
    using properties_t = std::unique_ptr<obs_properties_t, properties_delete_t>;
    properties_t m_props;

    QPointer<AFQDowoomiPopup> m_dowoomiPopup = nullptr;
    OBSSignal removeSignal;

    QTimer* m_refreshTimer = nullptr;
};

