#include "CLocaleTextManager.h"


#include <sstream>

#include <util/dstr.hpp>
#include <util/profiler.hpp>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#include "Application/CApplication.h"

#include "CoreModel/Log/CLogManager.h"


#define DEFAULT_LANG "en-US"


tLOCALE_NAME AFLocaleTextManager::GetLocaleNames()
{
	std::string path;
	if (!GetDataFilePath("locale.ini", path))
		throw "Could not find locale.ini path";

	ConfigFile ini;
	if (ini.Open(path.c_str(), CONFIG_OPEN_EXISTING) != 0)
		throw "Could not open locale.ini";

	size_t sections = config_num_sections(ini);

	tLOCALE_NAME names;
	names.reserve(sections);
	for (size_t i = 0; i < sections; i++) {
		const char* tag = config_get_section(ini, i);
		const char* name = config_get_string(ini, tag, "Name");
		names.emplace_back(tag, name);
	}

	return names;
}

bool AFLocaleTextManager::InitLocale()
{
	ProfileScope("AFLocaleTextManager::InitLocale");

	config_t* userConfig = USERCONFIG;
	//
	const char* lang = config_get_string(userConfig, "General", "Language");
	bool userLocale = config_has_user_value(userConfig, "General", "Language");
	bool foundLang = true;
	if (!userLocale || !lang || lang[0] == '\0')
	{
		lang = DEFAULT_LANG;
		foundLang = false;
	}

	m_currLocale = lang;

	std::string englishPath;
	if (!GetDataFilePath("locale/" DEFAULT_LANG ".ini", englishPath)) {
		OBSErrorBox(NULL, "Failed to find locale/" DEFAULT_LANG ".ini");
		return false;
	}

	m_baseLookup = text_lookup_create(englishPath.c_str());
	if (!m_baseLookup) {
		OBSErrorBox(NULL, "Failed to create locale from file '%s'", englishPath.c_str());
		return false;
	}

	bool defaultLang = astrcmpi(lang, DEFAULT_LANG) == 0;
	if (userLocale && defaultLang)
		return true;

	if (!userLocale && defaultLang)
	{
		for (auto& locale_ : GetPreferredLocales()) 
		{
			if (locale_ == lang)
				return true;

			std::stringstream file;
			file << "locale/" << locale_ << ".ini";

			std::string path;
			if (!GetDataFilePath(file.str().c_str(), path))
				continue;

			if (!text_lookup_add(m_baseLookup, path.c_str()))
				continue;

			blog(LOG_INFO, "Using preferred locale '%s'", locale_.c_str());
			m_currLocale = locale_;

			if (!foundLang)
			{
				config_set_string(userConfig, "General", "Language", m_currLocale.c_str());
				config_set_string(userConfig, "General", "LanguageBase", m_currLocale.c_str());
				config_save_safe(userConfig, "tmp", nullptr);
			}
			return true;
		}

		if (!foundLang)
		{
			config_set_string(userConfig, "General", "Language", m_currLocale.c_str());
			config_set_string(userConfig, "General", "LanguageBase", m_currLocale.c_str());
			config_save_safe(userConfig, "tmp", nullptr);
		}
		return true;
	}

	std::stringstream file;
	file << "locale/" << lang << ".ini";

	std::string path;
	if (GetDataFilePath(file.str().c_str(), path)) {
		if (!text_lookup_add(m_baseLookup, path.c_str()))
			blog(LOG_ERROR, "Failed to add locale file '%s'", path.c_str());
	} else {
		blog(LOG_ERROR, "Could not find locale file '%s'", file.str().c_str());
	}

	return true;
}

bool AFLocaleTextManager::TranslateString(const char* lookupVal, const char** out) const
{
	for (obs_frontend_translate_ui_cb cb : m_translatorHooks)
		if (cb(lookupVal, out))
			return true;

	return text_lookup_getstr(m_baseLookup, lookupVal, out);
}
//
const char* AFLocaleTextManager::getString(const char* lookupValue) const
{
	return m_baseLookup.GetString(lookupValue);
};