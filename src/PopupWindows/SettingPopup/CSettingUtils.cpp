#pragma once
#include "CSettingUtils.h"
#include <sstream>

namespace AFSettingUtilsA
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

	std::string ResString(uint32_t cx, uint32_t cy)
	{
		std::stringstream res;
		res << cx << "x" << cy;
		return res.str();
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

	int FindEncoder(QComboBox* combo, const char* name, int id)
	{
		FFmpegCodec codec{ name, id };

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


	bool return_first_id(void* data, const char* id)
	{
		const char** output = (const char**)data;

		*output = id;
		return false;
	}

	bool EncoderAvailable(const char* encoder)
	{
		const char* val;
		int i = 0;

		while (obs_enum_encoder_types(i++, &val))
			if (strcmp(val, encoder) == 0)
				return true;

		return false;
	}

	bool service_supports_codec(const char** codecs, const char* codec)
	{
		if (!codecs)
			return true;

		while (*codecs) {
			if (strcmp(*codecs, codec) == 0)
				return true;
			codecs++;
		}

		return false;
	}
	bool service_supports_encoder(const char** codecs,
		const char* encoder)
	{
		if (!EncoderAvailable(encoder))
			return false;

		const char* codec = obs_get_encoder_codec(encoder);
		return service_supports_codec(codecs, codec);
	}


	std::tuple<int, int> aspect_ratio(int cx, int cy)
	{
		int common = std::gcd(cx, cy);
		int newCX = cx / common;
		int newCY = cy / common;

		if (newCX == 8 && newCY == 5) {
			newCX = 16;
			newCY = 10;
		}

		return std::make_tuple(newCX, newCY);
	}

	std::string GetFormatExt(const char* container)
	{
		std::string ext = container;
		if (ext == "fragmented_mp4")
			ext = "mp4";
		else if (ext == "fragmented_mov")
			ext = "mov";
		else if (ext == "hls")
			ext = "m3u8";
		else if (ext == "mpegts")
			ext = "ts";

		return ext;
	}

	bool ConvertResText(const char* res, uint32_t& cx, uint32_t& cy)
	{
		BaseLexer lex;
		base_token token;

		lexer_start(lex, res);

		/* parse width */
		if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
			return false;
		if (token.type != BASETOKEN_DIGIT)
			return false;

		cx = std::stoul(token.text.array);

		/* parse 'x' */
		if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
			return false;
		if (strref_cmpi(&token.text, "x") != 0)
			return false;

		/* parse height */
		if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
			return false;
		if (token.type != BASETOKEN_DIGIT)
			return false;

		cy = std::stoul(token.text.array);

		/* shouldn't be any more tokens after this */
		if (lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
			return false;

		if (ResTooHigh(cx, cy) || ResTooLow(cx, cy)) {
			cx = cy = 0;
			return false;
		}

		return true;
	}

	bool ResTooHigh(uint32_t cx, uint32_t cy)
	{
		return cx > 16384 || cy > 16384;
	}

	bool ResTooLow(uint32_t cx, uint32_t cy)
	{
		return cx < 32 || cy < 32;
	}


	std::string GetRecordingFilename(
		const char* path, const char* container, bool noSpace, bool overwrite,
		const char* format, bool ffmpeg)
	{
		if (!ffmpeg)
			SetupAutoRemux(container);

		std::string dst = GetOutputFilename(path, container, noSpace, overwrite, format);
		return dst;
	}

	std::string GetOutputFilename(const char* path, const char* container, bool noSpace,
		bool overwrite, const char* format)
	{
		os_dir_t* dir = path && path[0] ? os_opendir(path) : nullptr;

		os_closedir(dir);

		std::string strPath;
		strPath += path;

		char lastChar = strPath.back();
		if (lastChar != '/' && lastChar != '\\')
			strPath += "/";

		std::string ext = GetFormatExt(container);
		strPath += GenerateSpecifiedFilename(ext.c_str(), noSpace, format);
		ensure_directory_exists(strPath);
		if (!overwrite)
			FindBestFilename(strPath, noSpace);

		return strPath;
	}

	void SetupAutoRemux(const char*& container)
	{
		config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();
		bool autoRemux = config_get_bool(basicConfig, "Video", "AutoRemux");
		if (autoRemux && strcmp(container, "mp4") == 0)
			container = "mkv";
	}

	std::string GenerateSpecifiedFilename(const char* extension, bool noSpace,
		const char* format)
	{
		BPtr<char> filename =
			os_generate_formatted_filename(extension, !noSpace, format);
		return std::string(filename);
	}

	void ensure_directory_exists(std::string& path)
	{
		replace(path.begin(), path.end(), '\\', '/');

		size_t last = path.rfind('/');
		if (last == std::string::npos)
			return;

		std::string directory = path.substr(0, last);
		os_mkdirs(directory.c_str());
	}

	void FindBestFilename(std::string& strPath, bool noSpace)
	{
		int num = 2;

		if (!os_file_exists(strPath.c_str()))
			return;

		const char* ext = strrchr(strPath.c_str(), '.');
		if (!ext)
			return;

		int extStart = int(ext - strPath.c_str());
		for (;;) {
			std::string testPath = strPath;
			std::string numStr;

			numStr = noSpace ? "_" : " (";
			numStr += std::to_string(num++);
			if (!noSpace)
				numStr += ")";

			testPath.insert(extStart, numStr);

			if (!os_file_exists(testPath.c_str())) {
				strPath = testPath;
				break;
			}
		}
	}

	OBSData GetDataFromJsonFile(const char* jsonFile) {
		char fullPath[512];
		OBSDataAutoRelease data = nullptr;

		int ret = AFConfigManager::GetSingletonInstance().GetProfilePath(fullPath, sizeof(fullPath), "recordEncoder.json");
		if (ret > 0) {
			BPtr<char> jsonData = os_quick_read_utf8_file(fullPath);
			if (!!jsonData) {
				data = obs_data_create_from_json(jsonData);
			}
		}

		if (!data)
			data = obs_data_create();

		return data.Get();
	}
}