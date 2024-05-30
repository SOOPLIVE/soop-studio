
#include "CAuth.h"
#include <vector>

#include "Application/CApplication.h"
#include "CoreModel/Config/CConfigManager.h"


struct AuthInfo {
    AFAuth::Def def;
    AFAuth::create_cb create;
};

static std::vector<AuthInfo> g_authDefs;

//
std::shared_ptr<AFAuth> AFAuth::Create(const std::string& service)
{
	for(auto& a : g_authDefs) {
		if(service.find(a.def.service) != std::string::npos) {
			return a.create();
		}
	}
	return nullptr;
}
//
AFAuth::Type AFAuth::AuthType(const std::string& service)
{
	for(auto& a : g_authDefs) {
		if(service.find(a.def.service) != std::string::npos) {
			return a.def.type;
		}
	}

	return Type::None;
}
bool AFAuth::External(const std::string& service)
{
	for(auto& a : g_authDefs) {
		if(service.find(a.def.service) != std::string::npos) {
			return a.def.externalOAuth;
		}
	}

	return false;
}
void AFAuth::Load()
{
	AFMainFrame* main = App()->GetMainView();
	const char* typeStr = config_get_string(GetBasicConfig(), "Auth", "Type");
	if(!typeStr)
		typeStr = "";

	main->m_auth = Create(typeStr);
	if(main->m_auth)
	{
		if(main->m_auth->LoadInternal()) {
			main->m_auth->LoadUI();
		}
	}
}
void AFAuth::Save()
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	//
	AFMainFrame* main = App()->GetMainView();
	AFAuth* auth = main->m_auth.get();
	if(!auth) {
		if(config_has_user_value(GetBasicConfig(), "Auth", "Type")) {
			config_remove_value(GetBasicConfig(), "Auth", "Type");
			config_save_safe(GetBasicConfig(), "tmp", nullptr);
		}
		return;
	}

	config_set_string(GetBasicConfig(), "Auth", "Type", auth->service());
	auth->SaveInternal();
	config_save_safe(GetBasicConfig(), "tmp", nullptr);
}
//
void AFAuth::RegisterAuth(const Def& d, create_cb create)
{
	AuthInfo info = {d, create};
	g_authDefs.push_back(info);
}