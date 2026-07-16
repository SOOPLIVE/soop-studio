#include "CAppStyling.h"

#include <QDir>

#include "obs-proxy-style.hpp"
#include "platform/platform.hpp"
#include "qt-wrappers.hpp"

#include "Common/StudioDefine.h"

#include "Application/CApplication.h"

#include "CoreModel/Config/CConfigManager.h"

#include "MainFrame/CMainFrame.h"

#include <util/cf-parser.h>
#include <util/dstr.hpp>

struct CFParser {
	cf_parser cfp = {};
	inline ~CFParser() { cf_parser_free(&cfp); }
	inline operator cf_parser* () { return &cfp; }
	inline cf_parser* operator->() { return &cfp; }
};

bool CAppStyling::InitStyle(QPalette palette)
{
	m_defaultPalette = palette;
	//App()->setStyle(new ProxyStyle());

	const char* themeName = config_get_string(USERCONFIG, "General", "CurrentTheme3");
	if (!themeName)
		themeName = "Black";

	if (strcmp(themeName, "Default") == 0)
		themeName = "Black";

	if (strcmp(themeName, "Dark") != 0 && SetTheme(themeName))
		return true;

	return SetTheme("Black");
}

std::string CAppStyling::GetTheme(std::string name, std::string path)
{
	/* Check user dir first, then preinstalled themes. */
	if (path == "") {
		char userDir[512];
		name = "themes/" + name + ".qss";
		std::string temp = LOCAL_FOLDER_NAME + "/" + name;
		int ret = GetAppConfigPath(userDir, sizeof(userDir), temp.c_str());

		if (ret > 0 && QFile::exists(userDir)) {
			path = std::string(userDir);
		}
		else if (!GetDataFilePath(name.c_str(), path)) {
			//OBSErrorBox(NULL, "Failed to find %s.", name.c_str());
			return "";
		}
	}
	return path;
}

std::string CAppStyling::SetParentTheme(std::string name)
{
	std::string path = GetTheme(name, "");
	if (path.empty())
		return path;
	App()->setPalette(m_defaultPalette);

	ParseExtraThemeData(path.c_str());
	return path;
}

void CAppStyling::ParseExtraThemeData(const char* path)
{
	BPtr<char> data = os_quick_read_utf8_file(path);
	QPalette pal = App()->palette();
	CFParser cfp;
	int ret;

	cf_parser_parse(cfp, data, path);

	bool colorAlpha = false;

	while (cf_go_to_token(cfp, "SOOPTheme", nullptr)) {
		if (!cf_next_token(cfp))
			return;

		int group = -1;

		if (cf_token_is(cfp, ":")) {
			ret = cf_next_token_should_be(cfp, ":", nullptr,
				nullptr);
			if (ret != PARSE_SUCCESS)
				continue;

			if (!cf_next_token(cfp))
				return;

			if (cf_token_is(cfp, "disabled")) {
				group = QPalette::Disabled;
			}
			else if (cf_token_is(cfp, "active")) {
				group = QPalette::Active;
			}
			else if (cf_token_is(cfp, "inactive")) {
				group = QPalette::Inactive;
			}
			else {
				continue;
			}

			if (!cf_next_token(cfp))
				return;
		}

		if (!cf_token_is(cfp, "{"))
			continue;

		for (;;) {
			if (!cf_next_token(cfp))
				return;

			ret = cf_token_is_type(cfp, CFTOKEN_NAME, "name",
				nullptr);
			if (ret != PARSE_SUCCESS)
				break;

			DStr name;
			dstr_copy_strref(name, &cfp->cur_token->str);

			ret = cf_next_token_should_be(cfp, ":", ";", nullptr);
			if (ret != PARSE_SUCCESS)
				continue;

			if (!cf_next_token(cfp))
				return;

			const char* array;
			uint32_t color = 0;
			colorAlpha = false;

			if (cf_token_is(cfp, "#")) {
				array = cfp->cur_token->str.array;
				color = strtol(array + 1, nullptr, 16);

			}
			else if (cf_token_is(cfp, "rgba")) {
				colorAlpha = true;

				ret = cf_next_token_should_be(cfp, "(", ";", nullptr);
				if (ret != PARSE_SUCCESS)
					continue;
				if (!cf_next_token(cfp))
					return;

				array = cfp->cur_token->str.array;
				uint32_t colorRed = static_cast<uint32_t>(strtol(array, nullptr, 10)); // red

				ret = cf_next_token_should_be(cfp, ",", ";", nullptr);
				if (ret != PARSE_SUCCESS)
					continue;
				if (!cf_next_token(cfp))
					return;

				array = cfp->cur_token->str.array;
				uint32_t colorGreen = static_cast<uint32_t>(strtol(array, nullptr, 10)); // green

				ret = cf_next_token_should_be(cfp, ",", ";", nullptr);
				if (ret != PARSE_SUCCESS)
					continue;
				if (!cf_next_token(cfp))
					return;

				array = cfp->cur_token->str.array;
				QString t = QString::fromUtf8(array);
				uint32_t colorBlue = static_cast<uint32_t>(strtol(array, nullptr, 10)) << 0; // blue

				ret = cf_next_token_should_be(cfp, ",", ";", nullptr);
				if (ret != PARSE_SUCCESS)
					continue;
				if (!cf_next_token(cfp))
					return;

				array = cfp->cur_token->str.array;

				// alpha
				uint32_t alpha_;
				QString charToQstr = QString::fromUtf8(array);
				int findCharIndex = charToQstr.indexOf(')');
				if (findCharIndex != -1)
					charToQstr = charToQstr.left(findCharIndex);
				findCharIndex = charToQstr.indexOf('%'); // is alpha value in percent
				if (findCharIndex != -1) {
					charToQstr = charToQstr.left(findCharIndex);
					alpha_ = (charToQstr.toFloat() / 100.0f) * 255.f;
				}
				else {
					alpha_ = charToQstr.toFloat();
				}

				color = (alpha_ << 24) | (colorRed << 16) | (colorGreen << 8) | colorBlue;
			}
			else if (cf_token_is(cfp, "rgb")) {
				ret = cf_next_token_should_be(cfp, "(", ";",
					nullptr);
				if (ret != PARSE_SUCCESS)
					continue;
				if (!cf_next_token(cfp))
					return;

				array = cfp->cur_token->str.array;
				color |= strtol(array, nullptr, 10) << 16;

				ret = cf_next_token_should_be(cfp, ",", ";",
					nullptr);
				if (ret != PARSE_SUCCESS)
					continue;
				if (!cf_next_token(cfp))
					return;

				array = cfp->cur_token->str.array;
				color |= strtol(array, nullptr, 10) << 8;

				ret = cf_next_token_should_be(cfp, ",", ";",
					nullptr);
				if (ret != PARSE_SUCCESS)
					continue;
				if (!cf_next_token(cfp))
					return;

				array = cfp->cur_token->str.array;
				color |= strtol(array, nullptr, 10);

			}
			else if (cf_token_is(cfp, "white")) {
				color = 0xFFFFFF;

			}
			else if (cf_token_is(cfp, "black")) {
				color = 0;
			}

			if (!cf_go_to_token(cfp, ";", nullptr))
				return;

			AddExtraThemeColor(pal, group, name->array, color, colorAlpha);
		}

		ret = cf_token_should_be(cfp, "}", "}", nullptr);
		if (ret != PARSE_SUCCESS)
			continue;
	}


	App()->setPalette(pal);
}

bool CAppStyling::SetTheme(std::string name, std::string path)
{
	m_currentTheme = name;

	path = GetTheme(name, path);
	if (path.empty())
		return false;

	App()->setStyleSheet("");
	std::unique_ptr<AFThemeMeta> themeMeta;
	themeMeta.reset(ParseThemeMeta(path.c_str()));
	std::string parentPath;

	if (themeMeta && !themeMeta->parent.empty()) {
		parentPath = SetParentTheme(themeMeta->parent);
	}

	std::string lpath = path;
	if (parentPath.empty()) {
		App()->setPalette(m_defaultPalette);
	}
	else {
		lpath = parentPath;
	}

	QString mpath = QString("file:///") + lpath.c_str();
	ParseExtraThemeData(path.c_str());
	App()->setStyleSheet(mpath);
	if (themeMeta) {
		m_themeDarkMode = themeMeta->dark;
	}
	else {
		QColor color = App()->palette().text().color();
		m_themeDarkMode = !(color.redF() < 0.5);
	}

#ifdef __APPLE__
    // append App StyleSheet for MacOS
    bool openSuccess = false;
    QString orgStyleSheet;
    {
        QString filePath = App()->styleSheet();
        filePath = filePath.mid(7); // remove "file://"
        QFile file(filePath);
        openSuccess = file.open(QFile::ReadOnly | QFile::Text);
        if (openSuccess)
        {
            QFileInfo fileInfo(filePath);
            QDir::setCurrent(fileInfo.absolutePath());
            QTextStream fileStream(&file);
            orgStyleSheet = fileStream.readAll();
        }
    }
    if (openSuccess)
    {
        QString appendStyleSheet = orgStyleSheet +
                                    "\n QCheckBox::indicator {\n margin-right: 5px; \n }" +
                                    "\n #AFQVideoSettingAreaWidget #checkBox_AdvOutUseRescale::indicator {\n \
                                        margin-left: 5px; \n }" +
                                    "\n #AFQOutputSettingAreaWidget #checkBox_AdvOutRecUseRescale::indicator {\n \
                                        margin-left: 5px; \n }" +
                                    "\n #AFQOutputSettingAreaWidget #checkBox_AdvOutSplitFile::indicator {\n \
                                        margin-left: 5px; \n }" +
                                    "\n #AFQOutputSettingAreaWidget #checkBox_AdvOutFFUseRescale::indicator {\n \
                                        margin-left: 5px; \n }";
//
        App()->setStyleSheet(appendStyleSheet);
    }
    //
    
	SetMacOSDarkMode(m_themeDarkMode);
#endif

	return true;
}

AFThemeMeta* CAppStyling::ParseThemeMeta(const char* path)
{
	BPtr<char> data = os_quick_read_utf8_file(path);
	CFParser cfp;
	int ret;

	if (!cf_parser_parse(cfp, data, path))
		return nullptr;

	if (cf_token_is(cfp, "AFThemeMeta") ||
		cf_go_to_token(cfp, "AFThemeMeta", nullptr)) {

		if (!cf_next_token(cfp))
			return nullptr;

		if (!cf_token_is(cfp, "{"))
			return nullptr;

		AFThemeMeta* meta = new AFThemeMeta();

		for (;;) {
			if (!cf_next_token(cfp)) {
				delete meta;
				return nullptr;
			}

			ret = cf_token_is_type(cfp, CFTOKEN_NAME, "name",
				nullptr);
			if (ret != PARSE_SUCCESS)
				break;

			DStr name;
			dstr_copy_strref(name, &cfp->cur_token->str);

			ret = cf_next_token_should_be(cfp, ":", ";", nullptr);
			if (ret != PARSE_SUCCESS)
				continue;

			if (!cf_next_token(cfp)) {
				delete meta;
				return nullptr;
			}

			ret = cf_token_is_type(cfp, CFTOKEN_STRING, "value",
				";");

			if (ret != PARSE_SUCCESS)
				continue;

			char* str;
			str = cf_literal_to_str(cfp->cur_token->str.array,
				cfp->cur_token->str.len);

			if (strcmp(name->array, "dark") == 0 && str) {
				meta->dark = strcmp(str, "true") == 0;
			}
			else if (strcmp(name->array, "parent") == 0 && str) {
				meta->parent = std::string(str);
			}
			else if (strcmp(name->array, "author") == 0 && str) {
				meta->author = std::string(str);
			}
			bfree(str);

			if (!cf_go_to_token(cfp, ";", nullptr)) {
				delete meta;
				return nullptr;
			}
		}
		return meta;
	}
	return nullptr;
}

void CAppStyling::AddExtraThemeColor(QPalette& pal, int group, const char* name, uint32_t color, bool colorAlpha)
{
	std::function<void(QPalette::ColorGroup)> func;

	if (astrcmpi(name, "alternateBase") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::AlternateBase, color, group, colorAlpha);
			};
	}
	else if (astrcmpi(name, "base") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::Base, color, group, colorAlpha);
			};
	}
	else if (astrcmpi(name, "brightText") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::BrightText, color, group, colorAlpha);
			};
	}
	else if (astrcmpi(name, "button") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::Button, color, group, colorAlpha);
			};
	}
	else if (astrcmpi(name, "buttonText") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::ButtonText, color, group, colorAlpha);
			};
	}
	else if (astrcmpi(name, "dark") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::Dark, color, group, colorAlpha);
			};
	}
	else if (astrcmpi(name, "highlight") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::Highlight, color, group, colorAlpha);
			};
	}
	else if (astrcmpi(name, "highlightedText") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::HighlightedText, color, group, colorAlpha);
			};
	}
	else if (astrcmpi(name, "light") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::Light, color, group, colorAlpha);
			};
	}
	else if (astrcmpi(name, "link") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::Link, color, group, colorAlpha);
			};
	}
	else if (astrcmpi(name, "linkVisited") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::LinkVisited, color, group, colorAlpha);
			};
	}
	else if (astrcmpi(name, "mid") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::Mid, color, group, colorAlpha);
			};
	}
	else if (astrcmpi(name, "midlight") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::Midlight, color, group, colorAlpha);
			};
	}
	else if (astrcmpi(name, "shadow") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::Shadow, color, group, colorAlpha);
			};
	}
	else if (astrcmpi(name, "text") == 0 || astrcmpi(name, "foreground") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::Text, color, group, colorAlpha);
			};
	}
	else if (astrcmpi(name, "toolTipBase") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::ToolTipBase, color, group, colorAlpha);
			};
	}
	else if (astrcmpi(name, "toolTipText") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::ToolTipText, color, group, colorAlpha);
			};
	}
	else if (astrcmpi(name, "windowText") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::WindowText, color, group, colorAlpha);
			};
	}
	else if (astrcmpi(name, "window") == 0 || astrcmpi(name, "background") == 0) {
		func = [&](QPalette::ColorGroup group) {
			AssignColorPalette(pal, QPalette::Window, color, group, colorAlpha);
			};
	}
	else {
		return;
	}

	switch (group) {
	case QPalette::Disabled:
	case QPalette::Active:
	case QPalette::Inactive:
		func((QPalette::ColorGroup)group);
		break;
	default:
		func((QPalette::ColorGroup)QPalette::Disabled);
		func((QPalette::ColorGroup)QPalette::Active);
		func((QPalette::ColorGroup)QPalette::Inactive);
	}
}

void CAppStyling::AssignColorPalette(QPalette& pal, QPalette::ColorRole role, uint color, QPalette::ColorGroup group, bool colorAlpha)
{
	if (colorAlpha)
		pal.setColor(group, role, QColor::fromRgba(color));
	else
		pal.setColor(group, role, QColor::fromRgb(color));
}

void CAppStyling::SetStyle(QWidget* widget)
{
	widget->setStyle(new OBSProxyStyle());
}
