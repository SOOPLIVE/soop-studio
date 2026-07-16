#pragma once

#include "obs.hpp"
#include <QPointer>
#include "UIComponent/CTopBaseWindow.h"
#include "ui_dowoomi-source.h"
#include "CDowoomiPopup.h"

#include "PopupWindows/SourceDialog/SOOPBrowserSource/CCefPopupDialog.h"

namespace Ui {
    class AFQDowoomiDialog;
}

 
class AFQDowoomiDialog : public AFTTopBaseDialog
{
    Q_OBJECT

public:
    AFQDowoomiDialog(QWidget* parent, OBSSource source);
    ~AFQDowoomiDialog();

    void IsKBODialog(OBSSource source);
    void IsFootballDialog(OBSSource source);
    void IsCommerceDialog(OBSSource source);
    void IsVideoBalloonDialog(OBSSource source);

signals:
    void qsignalStyleChanged(QString styleData);

private slots:
    void qslotDetailSettingClicked();
    void qslotStyleChanged(int);
    void qslotTypeChanged(int);
    void qslotShutdownSourceNotVisibleChecked(bool checked);
    void qslotFreecshotControlAudio(bool checked);
    void qslotInteractionClicked();
    void qslotRefreshClicked();
    void qslotHelpPopupClicked();
    void qslotBrowserSizeClicked();

protected:
    virtual void showEvent(QShowEvent* event) override;

private:
    void _CreateCefPopupProperties(obs_source_t* source);
    void _RefreshStyleCombobox(bool refreshNeed = false);
    static void _SourceRemoved(void* data, calldata_t* params);

private:
    Ui::AFQDowoomiDialog* ui = nullptr;
    OBSWeakSource m_weakSource = nullptr;

    QString m_sourceName;

    using properties_delete_t = decltype(&obs_properties_destroy);
    using properties_t = std::unique_ptr<obs_properties_t, properties_delete_t>;
    properties_t m_props;

    QPointer<AFQDowoomiPopup> m_dowoomiPopup = nullptr;
    
    bool m_isKBO = false;
    bool m_isFootball = false;
    bool m_isCommerce = false;
    bool m_isVideoBalloon = false;

    QPointer<AFQCefPopupDialog> m_cefPopupProperties = nullptr;
    OBSSignal removeSignal;
};
