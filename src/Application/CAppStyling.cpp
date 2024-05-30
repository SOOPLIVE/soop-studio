#include "CAppStyling.h"

#include <QDir>

#include "CProxyStyle.h"
#include "CoreModel/Config/CConfigManager.h"
#include "platform/platform.hpp"
#include "qt-wrapper.h"

#include <util/cf-parser.h>
#include <util/dstr.hpp>

struct CFParser {
	cf_parser cfp = {};
	inline ~CFParser() { cf_parser_free(&cfp); }
	inline operator cf_parser* () { return &cfp; }
	inline cf_parser* operator->() { return &cfp; }
};

bool AFQAppStyling::InitStyle(QPalette palette)
{
	m_DefaultPalette = palette;
	//App()->setStyle(new AFQProxyStyle());

	const char* themeName =
		config_get_string(AFConfigManager::GetSingletonInstance().GetGlobal(), "General", "CurrentTheme3");
	if (!themeName)
		themeName = "Black";

	if (strcmp(themeName, "Default") == 0)
		themeName = "Black";

	if (strcmp(themeName, "Dark") != 0 && SetTheme(themeName))
		return true;

	return SetTheme("Black");
}

std::string AFQAppStyling::GetTheme(std::string name, std::string path)
{
	/* Check user dir first, then preinstalled themes. */
	if (path == "") {
		char userDir[512];
		name = "themes/" + name + ".qss";
		std::string temp = "SOOPStudio/" + name;
		int ret = AFConfigManager::GetSingletonInstance().GetConfigPath(userDir, sizeof(userDir), temp.c_str());

		if (ret > 0 && QFile::exists(userDir)) {
			path = std::string(userDir);
		}
		else if (!GetDataFilePath(name.c_str(), path)) {
			AFErrorBox(NULL, "Failed to find %s.", name.c_str());
			return "";
		}
	}
	return path;
}

std::string AFQAppStyling::SetParentTheme(std::string name)
{
	std::string path = GetTheme(name, "");
	if (path.empty())
		return path;
	App()->setPalette(m_DefaultPalette);

	ParseExtraThemeData(path.c_str());
	return path;
}

void AFQAppStyling::ParseExtraThemeData(const char* path)
{
	BPtr<char> data = os_quick_read_utf8_file(path);
	QPalette pal = App()->palette();
	CFParser cfp;
	int ret;

	cf_parser_parse(cfp, data, path);

	while (cf_go_to_token(cfp, "OBSTheme", nullptr)) {
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

			if (cf_token_is(cfp, "#")) {
				array = cfp->cur_token->str.array;
				color = strtol(array + 1, nullptr, 16);

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

			AddExtraThemeColor(pal, group, name->array, color);
		}

		ret = cf_token_should_be(cfp, "}", "}", nullptr);
		if (ret != PARSE_SUCCESS)
			continue;
	}


	App()->setPalette(pal);
}

bool AFQAppStyling::SetTheme(std::string name, std::string path)
{
	m_CurrentTheme = name;

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
		App()->setPalette(m_DefaultPalette);
	}
	else {
		lpath = parentPath;
	}

	QString mpath = QString("file:///") + lpath.c_str();
	ParseExtraThemeData(path.c_str());
	App()->setStyleSheet(mpath);
	if (themeMeta) {
		m_ThemeDarkMode = themeMeta->dark;
	}
	else {
		QColor color = App()->palette().text().color();
		m_ThemeDarkMode = !(color.redF() < 0.5);
	}

#ifdef __APPLE__
    // append App StyleSheet for MacOS
    bool openSuccess = false;
    QString orgStyleSheet;
    { // scope
        QString filePath = App()->styleSheet();
        filePath = filePath.mid(7); // "file://" 
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
    
	SetMacOSDarkMode(m_ThemeDarkMode);
#endif

	//emit StyleChanged();
	return true;
}

AFThemeMeta* AFQAppStyling::ParseThemeMeta(const char* path)
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

void AFQAppStyling::AddExtraThemeColor(QPalette& pal, int group, const char* name, uint32_t color)
{
	std::function<void(QPalette::ColorGroup)> func;

#define DEF_PALETTE_ASSIGN(name)                              \
	do {                                                  \
		func = [&](QPalette::ColorGroup group) {      \
			pal.setColor(group, QPalette::name,   \
				     QColor::fromRgb(color)); \
		};                                            \
	} while (false)

	if (astrcmpi(name, "alternateBase") == 0) {
		DEF_PALETTE_ASSIGN(AlternateBase);
	}
	else if (astrcmpi(name, "base") == 0) {
		DEF_PALETTE_ASSIGN(Base);
	}
	else if (astrcmpi(name, "brightText") == 0) {
		DEF_PALETTE_ASSIGN(BrightText);
	}
	else if (astrcmpi(name, "button") == 0) {
		DEF_PALETTE_ASSIGN(Button);
	}
	else if (astrcmpi(name, "buttonText") == 0) {
		DEF_PALETTE_ASSIGN(ButtonText);
	}
	else if (astrcmpi(name, "brightText") == 0) {
		DEF_PALETTE_ASSIGN(BrightText);
	}
	else if (astrcmpi(name, "dark") == 0) {
		DEF_PALETTE_ASSIGN(Dark);
	}
	else if (astrcmpi(name, "highlight") == 0) {
		DEF_PALETTE_ASSIGN(Highlight);
	}
	else if (astrcmpi(name, "highlightedText") == 0) {
		DEF_PALETTE_ASSIGN(HighlightedText);
	}
	else if (astrcmpi(name, "light") == 0) {
		DEF_PALETTE_ASSIGN(Light);
	}
	else if (astrcmpi(name, "link") == 0) {
		DEF_PALETTE_ASSIGN(Link);
	}
	else if (astrcmpi(name, "linkVisited") == 0) {
		DEF_PALETTE_ASSIGN(LinkVisited);
	}
	else if (astrcmpi(name, "mid") == 0) {
		DEF_PALETTE_ASSIGN(Mid);
	}
	else if (astrcmpi(name, "midlight") == 0) {
		DEF_PALETTE_ASSIGN(Midlight);
	}
	else if (astrcmpi(name, "shadow") == 0) {
		DEF_PALETTE_ASSIGN(Shadow);
	}
	else if (astrcmpi(name, "text") == 0 ||
		astrcmpi(name, "foreground") == 0) {
		DEF_PALETTE_ASSIGN(Text);
	}
	else if (astrcmpi(name, "toolTipBase") == 0) {
		DEF_PALETTE_ASSIGN(ToolTipBase);
	}
	else if (astrcmpi(name, "toolTipText") == 0) {
		DEF_PALETTE_ASSIGN(ToolTipText);
	}
	else if (astrcmpi(name, "windowText") == 0) {
		DEF_PALETTE_ASSIGN(WindowText);
	}
	else if (astrcmpi(name, "window") == 0 ||
		astrcmpi(name, "background") == 0) {
		DEF_PALETTE_ASSIGN(Window);
	}
	else {
		return;
	}

#undef DEF_PALETTE_ASSIGN

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

void AFQAppStyling::SetStyle(QWidget* widget)
{
	widget->setStyle(new AFQProxyStyle());
}
