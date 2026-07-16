#pragma once

#include <QWidget>
#include "CSettingUtils.h"
#include "ui_setting-accessibility-area.h"

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
	Q_OBJECT

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

	void SetAccessibilityDataChangedVal(bool changed) { m_a11yChanged = changed; };
	bool AccessibilityDataChanged() { return m_a11yChanged; };

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
	void _SmallResolution();
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

	uint32_t m_preset = 0;
	uint32_t m_selectRed = 0x0182FF;		// selected
	uint32_t m_selectGreen = 0x00FF00;	// crop
	uint32_t m_selectBlue = 0x00E0FF;		// hover
	uint32_t m_mixerGreen = 0x267f26;
	uint32_t m_mixerYellow = 0x267f7f;
	uint32_t m_mixerRed = 0x26267f;
	uint32_t m_mixerGreenActive = 0x4cff4c;
	uint32_t m_mixerYellowActive = 0x4cffff;
	uint32_t m_mixerRedActive = 0x4c4cff;

	bool m_a11yChanged = false;
	bool m_loading = true;

	Ui::AFQAccessibilitySettingAreaWidget* ui;
#pragma endregion private member var


};
