#pragma once

#include <QWidget>
#include "ui_studio-setting-accessibility.h"

#include "CSettingUtils.h"
namespace Ui {
	class AFQAccessibilitySettingAreaWidget;
}

enum ColorPreset {
	COLOR_PRESET_DEFAULT,
	COLOR_PRESET_COLOR_BLIND_1,
	COLOR_PRESET_CUSTOM = 99,
};

#define A11Y_CHANGED &AFQAccessibilitySettingAreaWidget::qslotA11yDataChanged

class AFQAccessibilitySettingAreaWidget : public QWidget
{
#pragma region QT Field
	Q_OBJECT;
signals:
	void qsignalA11yDataChanged();
public slots:

private slots:
	void qslotA11yDataChanged();
	void qslotColorPreset(int idx);
	void qslotChoose1Clicked();
	void qslotChoose2Clicked();
	void qslotChoose3Clicked();
	void qslotChoose4Clicked();
	void qslotChoose5Clicked();
	void qslotChoose6Clicked();
	void qslotChoose7Clicked();
	void qslotChoose8Clicked();
	void qslotChoose9Clicked();
#pragma endregion QT Field

#pragma region class initializer, destructor
public:
	explicit AFQAccessibilitySettingAreaWidget(QWidget* parent = nullptr);
	~AFQAccessibilitySettingAreaWidget();
#pragma endregion class initializer, destructor

#pragma region public func
public:
	void LoadAccessibilitySettings(bool presetChange = false);
	void SaveAccessibilitySettings();

	void SetAccessibilityDataChangedVal(bool changed) { m_bA11yChanged = changed; };
	bool AccessibilityDataChanged() { return m_bA11yChanged; };

#pragma endregion public func

#pragma region private func
private:
	void _SetAccessibilityUIProp();
	void _ResetDefaultColors();
	void _SetDefaultColors();
	void _UpdateAccessibilityColors();
	void _SetStyle(QLabel* label, uint32_t colorVal);
	void _ChangeLanguage();
	QColor GetColor(uint32_t colorVal, QString label);
#pragma endregion private func

#pragma region public member var
public:
#pragma endregion public member var

#pragma region private member var
private:
	enum ColorPreset {
		COLOR_PRESET_DEFAULT,
		COLOR_PRESET_COLOR_BLIND_1,
		COLOR_PRESET_CUSTOM = 99,
	};

	uint32_t preset = 0;
	uint32_t selectRed = 0x0000FF;
	uint32_t selectGreen = 0x00FF00;
	uint32_t selectBlue = 0xFF7F00;
	uint32_t mixerGreen = 0x267f26;
	uint32_t mixerYellow = 0x267f7f;
	uint32_t mixerRed = 0x26267f;
	uint32_t mixerGreenActive = 0x4cff4c;
	uint32_t mixerYellowActive = 0x4cffff;
	uint32_t mixerRedActive = 0x4c4cff;

	bool m_bA11yChanged = false;
	bool m_bLoading = true;

	Ui::AFQAccessibilitySettingAreaWidget* ui;
#pragma endregion private member var


};