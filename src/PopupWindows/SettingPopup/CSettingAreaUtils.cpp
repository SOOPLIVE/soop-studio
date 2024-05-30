#pragma once
#include "CSettingAreaUtils.h"
#include <sstream>

#include "../Common/SettingsMiscDef.h"

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
			config_set_string(AFConfigManager::GetSingletonInstance().GetBasic(), section, value, QT_TO_UTF8(widget->currentText()));
	}

	void SaveComboData(QComboBox* widget, const char* section, const char* value)
	{
		if (WidgetChanged(widget)) {
			QString str = GetComboData(widget);
			config_set_string(AFConfigManager::GetSingletonInstance().GetBasic(), section, value, QT_TO_UTF8(str));
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

			config_set_bool(AFConfigManager::GetSingletonInstance().GetBasic(), section, value, checked);
		}
	}

	void SaveSpinBox(QSpinBox* widget, const char* section, const char* value)
	{
		if (WidgetChanged(widget))
			config_set_int(AFConfigManager::GetSingletonInstance().GetBasic(), section, value, widget->value());
	}

	void SaveDoubleSpinBox(QDoubleSpinBox* widget, const char* section, const char* value)
	{
		if (WidgetChanged(widget))
			config_set_double(AFConfigManager::GetSingletonInstance().GetBasic(), section, value, widget->value());
	}

	void SaveEdit(QLineEdit* widget, const char* section, const char* value)
	{
		if (WidgetChanged(widget))
			config_set_string(AFConfigManager::GetSingletonInstance().GetBasic(), section, value,
				QT_TO_UTF8(widget->text()));
	}

	bool SetInvalidValue(QComboBox* combo, const char* name, const char* data)
	{
		combo->insertItem(0, name, data);

		QStandardItemModel* model =
			dynamic_cast<QStandardItemModel*>(combo->model());
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

	void WriteJsonData(AFQPropertiesView* view, const char* path)
	{
		char full_path[512];

		if (!view || !WidgetChanged(view))
			return;

		int ret = AFConfigManager::GetSingletonInstance().GetProfilePath(full_path, sizeof(full_path), path);
		if (ret > 0) {
			obs_data_t* settings = view->GetSettings();
			if (settings) {
				obs_data_save_json_safe(settings, full_path, "tmp",
					"bak");
			}
		}
	}
	QString GetAdvFallback(const QString& enc)
	{
		if (enc == "jim_hevc_nvenc" || enc == "jim_av1_nvenc")
			return "jim_nvenc";
		if (enc == "h265_texture_amf" || enc == "av1_texture_amf")
			return "h264_texture_amf";
		if (enc == "com.apple.videotoolbox.videoencoder.ave.hevc")
			return "com.apple.videotoolbox.videoencoder.ave.avc";
		if (enc == "obs_qsv11_av1")
			return "obs_qsv11";
		return "obs_x264";
	}

	QString GetSimpleFallback(const QString& enc)
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
}