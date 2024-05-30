#pragma once

#include <qobject.h>
#include <functional>
#include <memory>


class AFAuth : public QObject
{
#pragma region QT Field, CTOR/DTOR
	Q_OBJECT

public:
	enum class Type {
		None,
		OAuth_StreamKey,
		OAuth_LinkedAccount,
	};
	struct Def {
		std::string service;
		Type type = Type::None;
		bool externalOAuth = false;
		bool usesBroadcastFlow = false;
	};

	inline AFAuth(const Def& d) : m_def(d) {}
	virtual ~AFAuth() {}
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
	typedef std::function<std::shared_ptr<AFAuth>()> create_cb;

	inline Type type() const { return m_def.type; }
	inline const char* service() const { return m_def.service.c_str(); }
	inline bool external() const { return m_def.externalOAuth; }
	inline bool broadcastFlow() const { return m_def.usesBroadcastFlow; }

	virtual void LoadUI() {}
	virtual void OnStreamConfig() {}

	static std::shared_ptr<AFAuth> Create(const std::string& service);
	static Type AuthType(const std::string& service);
	static bool External(const std::string& service);
	static void Load();
	static void Save();
#pragma endregion public func

#pragma region private func
private:
#pragma endregion private func

#pragma region protected func
protected:
	virtual void SaveInternal() = 0;
	virtual bool LoadInternal() = 0;

	struct ErrorInfo {
		std::string message;
		std::string error;

		ErrorInfo(std::string message_, std::string error_)
			:message(message_)
			,error(error_)
		{}
	};

	static void RegisterAuth(const Def& d, create_cb create);

#pragma endregion protected func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
private:
	Def m_def;
#pragma endregion private member var

#pragma region protected member var
protected:
	bool m_firstLoad = true;
#pragma endregion protected member var
};