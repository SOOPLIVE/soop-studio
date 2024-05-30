#ifndef CADVANCEDSETTINGAREAWIDGET_H
#define CADVANCEDSETTINGAREAWIDGET_H

#include <QWidget>
#include "CoreModel/Config/CConfigManager.h"
#include <qt-wrapper.h>

class QAbstractButton;
class QComboBox;
class QSpinBox;

namespace Ui {
class AFQAdvancedSettingAreaWidget;
}

class AFQAdvancedSettingAreaWidget : public QWidget
{
    Q_OBJECT
#pragma region class initializer, destructor
public:
    explicit AFQAdvancedSettingAreaWidget(QWidget* parent = nullptr);
    ~AFQAdvancedSettingAreaWidget();
#pragma endregion class initializer, destructor

#pragma region QT Field, CTOR/DTOR

public slots:
    void qslotResetAdvancedSettingUi();
    void qslotAdvancedChanged();
    void qslotAdvancedChangedRestart();
    void qslotOutputReconnectEnableClicked();
    void qslotFilenameFormattingTextEdited(const QString& text);

signals:
    void qsignalAdvancedDataChanged();
    void qsignalCallOutputSettingUpdateStreamDelayEstimate();
    void qsignalAdvReplayBufferCheckedChanged(bool checked);
#pragma endregion QT Field

#pragma region public func
public:
    void AdvancedSettingAreaInit();
    void LoadAdvancedSettings();
    void SetAdvancedDataChangedVal(bool changed) { m_bAdvancedSettingChanged = changed; };
    bool AdvancedDataChanged() { return m_bAdvancedSettingChanged; };
    void ResetRestart() { m_bRestartNeeded = false; };
    bool CheckBrowserHardwareAccelerationRestartRequired() { return m_bRestartNeeded; };
    void SaveAdvancedSettings();
    void ToggleOnStreaming(bool streaming);

    void SetAutoRemuxText(QString autoRemuxText);
#pragma endregion public func

#pragma region protected func
protected:
#pragma endregion protected func

#pragma region private func
private:
    void _SetAdvancedSettings();
    void _SetAdvancedSettingSignal();
    void _ChangeLanguage();
    void _UpdateAdvNetworkGroup();
#pragma endregion private func


#pragma region public member var
public:
#pragma endregion public member var

#pragma region private member var
private:
    Ui::AFQAdvancedSettingAreaWidget* ui;

    bool m_bBrowserAccelerationEnabled = false;
    bool m_bNeedRestartProgram = false;
    bool m_bAdvancedSettingChanged = false;
    bool m_bLoading = true;
    bool m_bRestartNeeded = false;

    int m_iVideoBitrate = 0;
    int m_iAudioBitrate = 0;
#pragma endregion private member var
};

#endif // CADVANCEDSETTINGAREAWIDGET_H
