#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QStandardItem>
#include "qt-wrapper.h"
#include "Application/CApplication.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "PopupWindows/SettingPopup/setting-utils/ffmpeg-utils.hpp"
#include "util/lexer.h"

#include "algorithm"

#ifndef MY_UTILSA_H
#define MY_UTILSA_H

static constexpr uint32_t ENCODER_HIDE_FLAGS2 = (OBS_ENCODER_CAP_DEPRECATED | OBS_ENCODER_CAP_INTERNAL);

static const double vals[] = { 1.0, 1.25, (1.0 / 0.75), 1.5,  (1.0 / 0.6), 1.75,
				  2.0, 2.25, 2.5,          2.75, 3.0 };

static const size_t numVals = sizeof(vals) / sizeof(double);

namespace AFSettingUtilsA
{
	inline QString QTStr(const char* lookupVal)
	{
		return QString::fromUtf8(AFLocaleTextManager::GetSingletonInstance().Str(lookupVal));
	}
	struct BaseLexer {
		lexer lex;

	public:
		inline BaseLexer() { lexer_init(&lex); }
		inline ~BaseLexer() { lexer_free(&lex); }
		operator lexer* () { return &lex; }
	};

	#define COMBO_CHANGED   &QComboBox::currentIndexChanged
	#define CBEDIT_CHANGED  &QComboBox::editTextChanged
	#define EDIT_CHANGED    &QLineEdit::textChanged
	#define CHECK_CHANGED   &QCheckBox::toggled
	#define SCROLL_CHANGED  &QSpinBox::valueChanged
	#define DSCROLL_CHANGED	&QDoubleSpinBox::valueChanged

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
	void SaveCombo(QComboBox* widget, const char* section, const char* value);
	void SaveComboData(QComboBox* widget, const char* section, const char* value);
	void SaveCheckBox(QAbstractButton* widget, const char* section, const char* value, bool invert = false);
	void SaveSpinBox(QSpinBox* widget, const char* section, const char* value);
	void SaveDoubleSpinBox(QDoubleSpinBox* widget, const char* section, const char* value);
	void SaveEdit(QLineEdit* widget, const char* section, const char* value);
	bool SetInvalidValue(QComboBox* combo, const char* name, const char* data = nullptr);

	void AddCodec(QComboBox* combo, const FFmpegCodec& codec);
	void SelectFormat(QComboBox* combo, const char* name, const char* mimeType);
	int FindEncoder(QComboBox* combo, const char* name, int id);
	void SelectEncoder(QComboBox* combo, const char* name, int id);
	bool return_first_id(void* data, const char* id);
	bool EncoderAvailable(const char* encoder);
	bool service_supports_codec(const char** codecs, const char* codec);
	bool service_supports_encoder(const char** codecs, const char* encoder);
	std::tuple<int, int> aspect_ratio(int cx, int cy);
	bool ConvertResText(const char* res, uint32_t& cx, uint32_t& cy);
	bool ResTooHigh(uint32_t cx, uint32_t cy);
	bool ResTooLow(uint32_t cx, uint32_t cy);
	QString GetComboData(QComboBox* combo);
	std::string GetFormatExt(const char* container);
	std::string GetRecordingFilename(const char* path, const char* container, bool noSpace, bool overwrite,
		const char* format, bool ffmpeg);
	std::string GetOutputFilename(const char* path, const char* container, bool noSpace,
		bool overwrite, const char* format);
	void SetupAutoRemux(const char*& container);
	std::string GenerateSpecifiedFilename(const char* extension, bool noSpace, const char* format);
	void ensure_directory_exists(std::string& path);
	void FindBestFilename(std::string& strPath, bool noSpace);

	OBSData GetDataFromJsonFile(const char* jsonFile);

	std::string ResString(uint32_t cx, uint32_t cy);
}
#endif MY_UTILSA_H