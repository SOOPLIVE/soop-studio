#ifndef CSTUDIOSETTINGDIALOG_H
#define CSTUDIOSETTINGDIALOG_H

#include <QDialog>
#include <QPointer>
#include <QAbstractButton>

#include <obs.hpp>

#include "MainFrame/CMainBaseWidget.h"

#include "UIComponent/CTopBaseWindow.h"

class AFMainFrame;
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

class AFQStudioSettingDialog : public AFTTopBaseDialog
{
    Q_OBJECT

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
        ADVANCED,
    };
    Q_ENUM(TabType)
    
    explicit AFQStudioSettingDialog(QWidget* parent = nullptr);
    ~AFQStudioSettingDialog();

public slots:
    void CloseSetting();
    void ToggleTabButton();
    void ResetButtonClicked();
    void ResetDownScales(int cx, int cy);
    void ChangeSettingPageData();
    void ChangeSimpleMode();
    void ChangeAdvanceMode();
    void ChangeSimpleReplayBuffer();
    void ChangeAdvanceReplayBuffer();
    void ChangeSimpleOutVEncoder(QString vEncoder);
    void ChangeSimpleOutAEncoder(QString aEncoder);
    void ChangeSimpleOutVBitrate(int vBitrate);
    void ChangeSimpleOutABitrate(int aBitrate);
    void ChangeImpleRecordingEncoder();
    void ChangeStreamEncoderProps();
    void UpdateStreamDelayEstimate();
    void SetAutoRemuxText(QString autoRemuxText);
    void ToggleStreamingUI(bool streaming);
    void LogoutKR();

    void ButtonBoxClicked(QAbstractButton* button);

public:
    void AFQStudioSettingDialogInit(int type = 0);
    QString GetCurrentAudioBitrate();
    void SetID(QString id, QString platform);

protected:
    //void reject() override;
    void keyPressEvent(QKeyEvent* event);
    void showEvent(QShowEvent* event) override;
    
private:
    void SetSettingAreaWidget();
    void SetTabButtons();
    void SetSettingDialogSignal();
    bool SaveSettings();
    void SaveStreamSettings();
    void ApplyDisable();
    void ApplyEnable();
    void ClearChanged();
    bool QueryChanges(bool isTriggeredByTabChange = false);
    bool QueryAllowedToClose();
    QString GetTabName(TabType type);
    bool AnyChanges();
    void ReloadTabConfig();
    void UpdateResetButtonVisible();

private:
    Ui::AFQStudioSettingDialog* ui;

    QPointer<AFQProgramSettingAreaWidget> programWidget;
    QPointer<AFQStreamSettingAreaWidget> streamWidget;
    QPointer<AFQOutputSettingAreaWidget> outputWidget;
    QPointer<AFQAudioSettingAreaWidget> audioWidget;
    QPointer<AFQVideoSettingAreaWidget> videoWidget;
    QPointer<AFQHotkeySettingAreaWidget> hotkeyWidget;
    QPointer<AFQAccessibilitySettingAreaWidget> accessWidget;
    QPointer<AFQAdvancedSettingAreaWidget> advanceWidget;

    QPointer<QPushButton> resetButton;

    int activeTabType = 0;
    bool restart = false;
    bool videoSizeValid = true;
};

#endif // CSTUDIOSETTINGDIALOG_H
