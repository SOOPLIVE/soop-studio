#ifndef SETTING_UTILS_H
#define SETTING_UTILS_H

#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QStandardItem>
#include "ffmpeg-utils.hpp"
#include "Application/CApplication.h"
#include "properties-view.hpp"
#include "CoreModel/Locale/CLocaleTextManager.h"

static constexpr uint32_t ENCODER_HIDE_FLAGS = (OBS_ENCODER_CAP_DEPRECATED | OBS_ENCODER_CAP_INTERNAL);
static const double vals[] = { 1.0, 1.25, (1.0 / 0.75), 1.5,  (1.0 / 0.6), 1.75,
							   2.0, 2.25, 2.5, 2.75, 3.0 };
static const size_t numVals = sizeof(vals) / sizeof(double);

namespace AFSettingUtils
{
	#define CBEDIT_CHANGED  &QComboBox::editTextChanged
	#define COMBO_CHANGED   &QComboBox::currentIndexChanged
	#define CHECK_CHANGED   &QCheckBox::toggled
	#define SCROLL_CHANGED  &QSpinBox::valueChanged
	#define DSCROLL_CHANGED	&QDoubleSpinBox::valueChanged
	#define BUTTON_CLICKED  &QPushButton::clicked
	#define EDIT_CHANGED    &QLineEdit::textChanged

	template<typename SignalWidget, typename SlotWidget,
		typename SignalWidgetParent, typename SlotWidgetParent, 
		typename... SignalArgs, typename... SlotArgs>
	void HookWidget(SignalWidget* sigWidget, SlotWidget* slotWidget,
		void (SignalWidgetParent::* signal)(SignalArgs...),
		void (SlotWidgetParent::* slot)(SlotArgs...))
	{
		QObject::connect(sigWidget, signal, slotWidget, slot);
		sigWidget->setProperty("changed", QVariant(false));
	}

	bool WidgetChanged(QWidget* widget);

	void SetComboByName(QComboBox* combo, const char* name);
	bool SetComboByValue(QComboBox* combo, const char* name);
	bool SetInvalidValue(QComboBox* combo, const char* name, const char* data = nullptr);
	
	void SaveCombo(QComboBox* widget, const char* section, const char* value);
	void SaveComboData(QComboBox* widget, const char* section, const char* value);
	void SaveCheckBox(QAbstractButton* widget, const char* section, const char* value, bool invert = false);
	void SaveSpinBox(QSpinBox* widget, const char* section, const char* value);
	void SaveDoubleSpinBox(QDoubleSpinBox* widget, const char* section, const char* value);
	void SaveEdit(QLineEdit* widget, const char* section, const char* value);

	QString GetComboData(QComboBox* combo);

	std::string DeserializeConfigText(const char* value);
	void WriteJsonData(OBSPropertiesView* view, const char* path);

	QString get_adv_fallback(const QString& enc);
	QString get_simple_fallback(const QString& enc);

	int FindEncoder(QComboBox* combo, const char* name, int id);
	void SelectEncoder(QComboBox* combo, const char* name, int id);
	void AddCodec(QComboBox* combo, const FFmpegCodec& codec);
	void SelectFormat(QComboBox* combo, const char* name, const char* mimeType);

	std::string ResString(uint32_t cx, uint32_t cy);
}
#endif // SETTING_UTILS_H