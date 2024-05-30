#ifndef CSTUDIOSETTINGDIALOG_H
#define CSTUDIOSETTINGDIALOG_H

#include <QDialog>
#include <QPointer>
#include <QAbstractButton>

#include <obs.hpp>

#include "MainFrame/CMainBaseWidget.h"
#include "./UIComponent/CRoundedDialogBase.h"





class AFMainFrame;
class QComboBox;
class AFQSettingTabButton;

class AFQProgramSettingAreaWidget;
class AFQStreamSettingAreaWidget;
class AFQOutputSettingAreaWidget;
class AFQAudioSettingAreaWidget;
class AFQVideoSettingAreaWidget;
class AFQHotkeySettingAreaWidget;
class AFQAccessibilitySettingAreaWidget;
class AFQAdvancedSettingAreaWidget;

namespace Ui {
class AFQStudioSettingDialog;
}

class AFQStudioSettingDialog : public AFQRoundedDialogBase
{
    Q_OBJECT
#pragma region class initializer, destructor
public:
    enum TabType
    {
        PROGRAM = 0,
        STREAM,
        OUTPUT,
        AUDIO,
        VIDEO,
        HOTKEYS,
        ACCESSIBILITY,
        ADVANCED
    };
    Q_ENUM(TabType)
    
    explicit AFQStudioSettingDialog(QWidget* parent = nullptr);
    ~AFQStudioSettingDialog();
#pragma endregion class initializer, destructor

#pragma region QT Field, CTOR/DTOR
public slots:
    void qslotCloseSetting();
    void qslotTabButtonToggled();
    void qslotResetButtonClicked();
    void qslotResetDownscales(int cx, int cy);
    void qslotSettingPageDataChanged();
    void qslotChangeSettingModeToSimple();
    void qslotChangeSettingModeToAdvanced();
    void qslotSimpleReplayBufferChanged();
    void qslotAdvReplayBufferChanged();
    void qslotSimpleOutVEncoderChanged(QString vEncoder);
    void qslotSimpleOutAEncoderChanged(QString aEncoder);
    void qslotSimpleOutVBitrateChanged(int vBitrate);
    void qslotSimpleOutABitrateChanged(int aBitrate);
    void qslotSimpleRecordingEncoderChanged();
    void qslotStreamEncoderPropsChanged();
    void qslotUpdateStreamDelayEstimate();
    void qslotButtonBoxClicked(QAbstractButton* button);
    void qslotSetAutoRemuxText(QString autoRemuxText);
    void qslotToggleStreamingUI(bool streaming);

signals:
    void qsignalSettingSave();
    void qsignalSettingClose();
#pragma endregion QT Field, CTOR/DTOR


#pragma region public func
public:
    void AFQStudioSettingDialogInit(int type = 0);
    QString GetCurrentAudioBitrate();
    void SetID(QString id);
#pragma endregion public func

#pragma region protected func
protected:
    void reject() override;
#pragma endregion protected func

#pragma region private func
private:
    void _SetSettingAreaWidget();
    void _SetTabButtons();
    void _SetSettingDialogSignal();
    void _SaveSettings();
    void _ApplyDisable();
    void _ApplyEnable();
    void _ClearChanged();
    bool _QueryChanges();
    bool _QueryAllowedToClose();
    QString _GetTabName(TabType type);
    bool _AnyChanges();
    void _ReloadTabConfig();
    void _UpdateResetButtonVisible();
#pragma endregion private func


#pragma region public member var
public:
    QString Streamkey;
    QString Server;

#pragma endregion public member var

#pragma region private member var
private:
    AFMainFrame* m_MainFrame;
    std::vector<AFQSettingTabButton*>   m_vTabButton;

    Ui::AFQStudioSettingDialog* ui;

    QPointer<AFQProgramSettingAreaWidget> m_pProgramSettingAreaWidget;
    QPointer<AFQStreamSettingAreaWidget> m_pStreamSettingAreaWidget;
    QPointer<AFQOutputSettingAreaWidget> m_pOutputSettingAreaWidget;
    QPointer<AFQAudioSettingAreaWidget> m_pAudioSettingAreaWidget;
    QPointer<AFQVideoSettingAreaWidget> m_pVideoSettingAreaWidget;
    QPointer<AFQHotkeySettingAreaWidget> m_pHotkeysSettingAreaWidget;
    QPointer<AFQAccessibilitySettingAreaWidget> m_pAccesibilitySettingAreaWidget;
    QPointer<AFQAdvancedSettingAreaWidget> m_pAdvancedSettingAreaWidget;

    int m_eCurrentTabNum = 0;

    bool m_bRestart = false;
    QPointer<QPushButton> m_pResetButton;

#pragma endregion private member var

};

#endif // CSTUDIOSETTINGDIALOG_H
