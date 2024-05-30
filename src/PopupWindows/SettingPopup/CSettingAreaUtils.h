#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QStandardItem>
#include "qt-wrapper.h"
#include "Application/CApplication.h"
#include "PopupWindows/SourceDialog/CPropertiesView.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#ifndef SETTING_UTILS_H
#define SETTING_UTILS_H

static constexpr uint32_t ENCODER_HIDE_FLAGS = (OBS_ENCODER_CAP_DEPRECATED | OBS_ENCODER_CAP_INTERNAL);

namespace AFSettingUtils
{
	#define CBEDIT_CHANGED  &QComboBox::editTextChanged
	#define COMBO_CHANGED   &QComboBox::currentIndexChanged
	#define CHECK_CHANGED   &QCheckBox::toggled
	#define SCROLL_CHANGED  &QSpinBox::valueChanged
	#define DSCROLL_CHANGED	&QDoubleSpinBox::valueChanged
	#define BUTTON_CLICKED  &QPushButton::clicked
	#define TEXT_CHANGED    &QLineEdit::textChanged
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

	void WriteJsonData(AFQPropertiesView* view, const char* path);

	QString GetAdvFallback(const QString& enc);
	QString GetSimpleFallback(const QString& enc);
}
#endif // SETTING_UTILS_H