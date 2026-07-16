
#pragma once

#include "ui_overlay-widget.h"

//#include <QTimer>

inline void ___properties_dummy_addref(obs_properties_t*) {} // obs-studio\plugins\obs-websocket\src\utils\obs.h
using OBSPropertiesAutoDestroy = OBSRef<obs_properties_t*, ___properties_dummy_addref, obs_properties_destroy>; // obs-studio\plugins\obs-websocket\src\utils\obs.h

class AFQOverlayWidget : public QWidget {
#pragma region QT Field
    Q_OBJECT

private slots:
    void _qslotWindowCurrentIndexChanged(int index);
    void _qslotWindowbeforeShowPopup();
   
   //void qslotComboBox_PriorityCurrentIndexChanged(int index);
   //void qslotTimer();

    void _qslotRefreshHotkey();

signals:
    void qsignalCloseTriggered(int type);
#pragma endregion QT Field
#pragma region class initializer, destructor
public:
    explicit AFQOverlayWidget();
    virtual ~AFQOverlayWidget() { delete ui; }
#pragma endregion class initializer, destructor
#pragma region public func
public:
#pragma endregion public func
#pragma region protected func
protected:
    void closeEvent(QCloseEvent* event) override;
#pragma endregion protected func
#pragma region private func
private:
#pragma endregion private func
#pragma region private var
private:
    Ui::AFQOverlayWidget* ui;
    
    obs_source_t* source = nullptr;

    //QTimer* timer;
#pragma endregion private var
};
