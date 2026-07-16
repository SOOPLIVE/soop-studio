#pragma once
#include "CSettingUtils.h"
#include <sstream>

#include <ffmpeg-utils.hpp>
#include "qt-wrappers.hpp"

#include "Common/SettingsMiscDef.h"

#include "CoreModel/Config/CConfigManager.h"

namespace AFSettingUtils
{
	bool WidgetChanged(QWidget* widget)
	{
		return widget->property("changed").toBool();
	}

	void SetComboByName(QComboBox* combo, const char* name)
	{
		int idx = combo->findText(QT_UTF8(name));

		if (idx != -1)
			combo->setCurrentIndex(idx);
		else
			combo->setCurrentIndex(0);
	}

	bool SetComboByValue(QComboBox* combo, const char* name)
	{
		int idx = combo->findData(QT_UTF8(name));

		if (idx != -1) {
			combo->setCurrentIndex(idx);
			return true;
		}
		else
			combo->setCurrentIndex(0);

		return false;
	}

	void SaveCombo(QComboBox* widget, const char* section, const char* value)
	{
		if (WidgetChanged(widget))
			config_set_string(ACTIVECONFIG, section, value, QT_TO_UTF8(widget->currentText()));
	}

	void SaveComboData(QComboBox* widget, const char* section, const char* value)
	{
		if (WidgetChanged(widget)) {
			QString str = GetComboData(widget);
			config_set_string(ACTIVECONFIG, section, value, QT_TO_UTF8(str));
		}
	}

	void SaveCheckBox(QAbstractButton* widget,
		const char* section, const char* value,
		bool invert)
	{
		if (WidgetChanged(widget)) {
			bool checked = widget->isChecked();
			if (invert)
				checked = !checked;

			config_set_bool(ACTIVECONFIG, section, value, checked);
		}
	}

	void SaveSpinBox(QSpinBox* widget, const char* section, const char* value)
	{
		if (WidgetChanged(widget))
			config_set_int(ACTIVECONFIG, section, value, widget->value());
	}

	void SaveDoubleSpinBox(QDoubleSpinBox* widget, const char* section, const char* value)
	{
		if (WidgetChanged(widget))
			config_set_double(ACTIVECONFIG, section, value, widget->value());
	}

	void SaveEdit(QLineEdit* widget, const char* section, const char* value)
	{
		if (WidgetChanged(widget))
			config_set_string(ACTIVECONFIG, section, value, QT_TO_UTF8(widget->text()));
	}

	bool SetInvalidValue(QComboBox* combo, const char* name, const char* data)
	{
		combo->insertItem(0, name, data);

		QStandardItemModel* model = dynamic_cast<QStandardItemModel*>(combo->model());
		if (!model)
			return false;

		QStandardItem* item = model->item(0);
		item->setFlags(Qt::NoItemFlags);

		combo->setCurrentIndex(0);
		return true;
	}

	QString GetComboData(QComboBox* combo)
	{
		int idx = combo->currentIndex();
		if (idx == -1)
			return QString();

		return combo->itemData(idx).toString();
	}

	std::string DeserializeConfigText(const char* value)
	{
		OBSDataAutoRelease data = obs_data_create_from_json(value);
		return obs_data_get_string(data, "text");
	}

	void WriteJsonData(OBSPropertiesView* view, const char* path)
	{
		char full_path[512];

		if (!view || !WidgetChanged(view))
			return;

		int ret = GetProfilePath(full_path, sizeof(full_path), path);
		if (ret > 0) {
			obs_data_t* settings = view->GetSettings();
			if (settings) {
				obs_data_save_json_safe(settings, full_path, "tmp",
					"bak");
			}
		}
	}
	/* we really need a way to find fallbacks in a less hardcoded way. maybe. */
	QString get_adv_fallback(const QString& enc)
	{
		if(enc == "obs_nvenc_hevc_tex" || enc == "obs_nvenc_av1_tex" ||
		   enc == "jim_hevc_nvenc" || enc == "jim_av1_nvenc")
			return "obs_nvenc_h264_tex";
		if(enc == "h265_texture_amf" || enc == "av1_texture_amf")
			return "h264_texture_amf";
		if(enc == "com.apple.videotoolbox.videoencoder.ave.hevc")
			return "com.apple.videotoolbox.videoencoder.ave.avc";
		if(enc == "obs_qsv11_av1")
			return "obs_qsv11";
		return "obs_x264";
	}

	QString get_simple_fallback(const QString& enc)
	{
		if (enc == SIMPLE_ENCODER_NVENC_HEVC || enc == SIMPLE_ENCODER_NVENC_AV1)
			return SIMPLE_ENCODER_NVENC;
		if (enc == SIMPLE_ENCODER_AMD_HEVC || enc == SIMPLE_ENCODER_AMD_AV1)
			return SIMPLE_ENCODER_AMD;
		if (enc == SIMPLE_ENCODER_APPLE_HEVC)
			return SIMPLE_ENCODER_APPLE_H264;
		if (enc == SIMPLE_ENCODER_QSV_AV1)
			return SIMPLE_ENCODER_QSV;
		return SIMPLE_ENCODER_X264;
	}

	int FindEncoder(QComboBox* combo, const char* name, int id)
	{
		FFmpegCodec codec {name, id};

		for (int i = 0; i < combo->count(); i++) {
			QVariant v = combo->itemData(i);
			if (!v.isNull()) {
				if (codec == v.value<FFmpegCodec>()) {
					return i;
				}
			}
		}
		return -1;
	}

	void SelectEncoder(QComboBox* combo, const char* name, int id)
	{
		int idx = FindEncoder(combo, name, id);
		if (idx >= 0)
			combo->setCurrentIndex(idx);
	}

	void AddCodec(QComboBox* combo, const FFmpegCodec& codec)
	{
		QString itemText;
		if (codec.long_name)
			itemText = QString("%1 - %2").arg(codec.name, codec.long_name);
		else
			itemText = codec.name;

		combo->addItem(itemText, QVariant::fromValue(codec));
	}

	void SelectFormat(QComboBox* combo, const char* name,
		const char* mimeType)
	{
		FFmpegFormat format{ name, mimeType };

		for (int i = 0; i < combo->count(); i++) {
			QVariant v = combo->itemData(i);
			if (!v.isNull()) {
				if (format == v.value<FFmpegFormat>()) {
					combo->setCurrentIndex(i);
					return;
				}
			}
		}

		combo->setCurrentIndex(0);
	}

	std::string ResString(uint32_t cx, uint32_t cy)
	{
		std::stringstream res;
		res << cx << "x" << cy;
		return res.str();
	}
}