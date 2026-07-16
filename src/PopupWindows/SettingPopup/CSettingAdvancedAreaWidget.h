#ifndef CADVANCEDSETTINGAREAWIDGET_H
#define CADVANCEDSETTINGAREAWIDGET_H

#include <QWidget>

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
    //void qslotUpdateStreamDelayEstimate();
    //void qslotUpdateStreamDelayEstimateLabel();

signals:
    void qsignalAdvancedDataChanged();
    void qsignalCallOutputSettingUpdateStreamDelayEstimate();
    void qsignalAdvReplayBufferCheckedChanged(bool checked);
#pragma endregion QT Field

#pragma region public func
public:
    void AdvancedSettingAreaInit();
    void LoadAdvancedSettings();
    void SetAdvancedDataChangedVal(bool changed) { m_advancedSettingChanged = changed; };
    bool AdvancedDataChanged() { return m_advancedSettingChanged; };
    void ResetRestart() { m_restartNeeded = false; };
    bool CheckBrowserHardwareAccelerationRestartRequired() { return m_restartNeeded; };
    void SaveAdvancedSettings();
    void ToggleOnStreaming(bool streaming);

    void SetAutoRemuxText(QString autoRemuxText);
    //void EstimateLabelChange(int videoBitrate, int audioBitrate);
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

    bool m_browserAccelerationEnabled = false;
    bool m_needRestartProgram = false;
    bool m_advancedSettingChanged = false;
    bool m_loading = true;
    bool m_restartNeeded = false;

    int m_videoBitrate = 0;
    int m_audioBitrate = 0;
#pragma endregion private member var
};

#endif // CADVANCEDSETTINGAREAWIDGET_H
