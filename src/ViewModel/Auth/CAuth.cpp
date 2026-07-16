
#include "CAuth.h"
#include <vector>

#include "Application/CApplication.h"


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
//
void AFAuth::RegisterAuth(const Def& d, create_cb create)
{
	AuthInfo info = {d, create};
	g_authDefs.push_back(info);
}