#pragma once

#include "obs.hpp"
#include <QPointer>
#include "UIComponent/CTopBaseWindow.h"

namespace Ui {
    class AFQVideoBalloonProps;
}

 
class AFQVideoBalloonProps : public AFTTopBaseDialog
{
    Q_OBJECT
public:
    AFQVideoBalloonProps(QWidget* parent, OBSSource source);
    ~AFQVideoBalloonProps();

private slots:
    void qslotDetailSettingClicked();
    void qslotFreecShotControlAudioClicked(bool checked);
    void qslotRefreshClicked();

private:
    static void _SourceRemoved(void* data, calldata_t* params);

private:
    Ui::AFQVideoBalloonProps* ui = nullptr;
    OBSWeakSource m_weakSource = nullptr;

    using properties_delete_t = decltype(&obs_properties_destroy);
    using properties_t = std::unique_ptr<obs_properties_t, properties_delete_t>;
    properties_t m_props;

    OBSSignal removeSignal;
};
