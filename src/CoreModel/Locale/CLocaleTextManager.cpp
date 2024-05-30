#include "CLocaleTextManager.h"


#include <sstream>


#include <util/dstr.hpp>
#include <util/profiler.hpp>


#include "CoreModel/Log/CLogManager.h"
#include "CoreModel/Config/CConfigManager.h"
#include "include/qt-wrapper.h"
#include "platform/platform.hpp"


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

	config_t* globalConfig = AFConfigManager::GetSingletonInstance().GetGlobal();


	const char* lang =
		config_get_string(globalConfig, "General", "Language");
	bool userLocale =
		config_has_user_value(globalConfig, "General", "Language");
	bool foundLang = true;
	if (!userLocale || !lang || lang[0] == '\0')
	{
		lang = DEFAULT_LANG;
		foundLang = false;
	}

	m_strCurrLocale = lang;

	std::string englishPath;
	if (!GetDataFilePath("locale/" DEFAULT_LANG ".ini", englishPath)) 
	{
		AFErrorBox(NULL, "Failed to find locale/" DEFAULT_LANG ".ini");

		return false;
	}

	m_BaseLookup = text_lookup_create(englishPath.c_str());
	if (!m_BaseLookup) 
	{
		AFErrorBox(NULL, "Failed to create locale from file '%s'",
				   englishPath.c_str());

		return false;
	}

	bool defaultLang = astrcmpi(lang, DEFAULT_LANG) == 0;

	auto& logManager = AFLogManager::GetSingletonInstance();

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

			if (!text_lookup_add(m_BaseLookup, path.c_str()))
				continue;

			logManager.OBSBaseLog(LOG_INFO, "Using preferred locale '%s'",
								  locale_.c_str());
			m_strCurrLocale = locale_;


			if (!foundLang)
			{
				config_set_string(globalConfig, "General", "Language", m_strCurrLocale.c_str());
				config_set_string(globalConfig, "General", "LanguageBase", m_strCurrLocale.c_str());
				config_save_safe(globalConfig, "tmp", nullptr);
			}
			return true;
		}

		if (!foundLang)
		{
			config_set_string(globalConfig, "General", "Language", m_strCurrLocale.c_str());
			config_set_string(globalConfig, "General", "LanguageBase", m_strCurrLocale.c_str());
			config_save_safe(globalConfig, "tmp", nullptr);
		}
		return true;
	}



	std::stringstream file;
	file << "locale/" << lang << ".ini";

	std::string path;
	if (GetDataFilePath(file.str().c_str(), path))
	{
		if (!text_lookup_add(m_BaseLookup, path.c_str()))
			logManager.OBSBaseLog(LOG_ERROR, "Failed to add locale file '%s'",
							      path.c_str());
	}
	else
	{
		logManager.OBSBaseLog(LOG_ERROR, "Could not find locale file '%s'",
							  file.str().c_str());
	}



	return true;
}

bool AFLocaleTextManager::TranslateString(const char* lookupVal, const char** out) const
{
	for (obs_frontend_translate_ui_cb cb : m_queTranslatorHooks)
		if (cb(lookupVal, out))
			return true;
	

	return text_lookup_getstr(m_BaseLookup, lookupVal, out);
}
