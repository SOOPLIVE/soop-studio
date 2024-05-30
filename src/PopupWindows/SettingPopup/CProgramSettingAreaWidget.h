#ifndef CPROGRAMSETTINGAREAWIDGET_H
#define CPROGRAMSETTINGAREAWIDGET_H

#include <QWidget>

class QAbstractButton;
class QComboBox;
class QSpinBox;

namespace Ui {
    class AFQProgramSettingAreaWidget;
}

class AFQProgramSettingAreaWidget : public QWidget
{
    Q_OBJECT
#pragma region class initializer, destructor


public:
    explicit AFQProgramSettingAreaWidget(QWidget* parent = nullptr);
    ~AFQProgramSettingAreaWidget();
#pragma endregion class initializer, destructor

#pragma region QT Field, CTOR/DTOR
public slots:
    void qslotToggleAdvancedSetting();
    void qslotHideAnetaWindowWarning(int state);
    void qslotGeneralChanged();
    void qslotResetProgramSettingUi();
    void qslotDisableOverFlowCheckBox(bool check);
    void UpdateAutomaticReplayBufferCheckboxes(bool state);
    void qslotSpreadAreaHoverEnter();
    void qslotSpreadAreaHoverLeave();

    void qslotLoginMainAccount();
    void qslotLogoutMainAccount();

signals:
    void qsignalProgramDataChanged();
#pragma endregion QT Field

#pragma region public func
public:
    void ProgramSettingAreaInit();
    void LoadProgramSettings();
    void LoadMainAccount();
    void SaveProgramSettings();
    void ResetProgramSettings();
    void ToggleOnStreaming(bool streaming);
    void SaveProgramPageSpreadState();
    void SetProgramDataChangedVal(bool changed) { m_bProgramDataChanged = changed; };
    bool ProgramDataChanged() { return m_bProgramDataChanged; };
    bool CheckLanguageRestartRequired() { return m_bRestartNeeded; }
    bool CheckSystemTrayToggle() { return m_bSystemTrayToggle; };
#pragma endregion public func

#pragma region protected func
protected:
#pragma endregion protected func

#pragma region private func
private:
    void _LoadLanguageList();
    void _LoadThemeList();
    void _SetProgramSettingSignal();

    void _ChangeLanguage();
#pragma endregion private func


#pragma region public member var
public:
#pragma endregion public member var

#pragma region private member var
private:
    Ui::AFQProgramSettingAreaWidget* ui;
    std::string m_sSavedTheme;

    bool m_bLoading = true;
    bool m_bProgramDataChanged = false;
    bool m_bRestartNeeded = false;
    bool m_bSystemTrayToggle = false;
    bool m_bStudioModeVertical = true;

    int m_iCurrentLanguageIndex = 0;

#pragma endregion private member var
};

#endif // CPROGRAMSETTINGAREAWIDGET_H