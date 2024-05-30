#pragma once

#include <QObject>
#include <QString>
#include <random>
#include <string>

#include "ViewModel/Auth/COAuthLogin.hpp"



#define YOUTUBE_CLIENTID            ""
#define YOUTUBE_SECRETID            ""



class AFAddStreamWidget;
class AFAuthListener;

inline const std::vector<AFAuth::Def> youtubeServices = {
	{"YouTube - RTMP", AFAuth::Type::OAuth_LinkedAccount, true, true},
	{"YouTube - RTMPS", AFAuth::Type::OAuth_LinkedAccount, true, true},
	{"YouTube - HLS", AFAuth::Type::OAuth_LinkedAccount, true, true}};

class YoutubeAuth : public AFOAuthStreamKey {
	Q_OBJECT

public:
	YoutubeAuth(const Def &d, AFAddStreamWidget* widget);
	~YoutubeAuth();

	virtual bool Login() override;
	virtual void DeleteCookies() override;

	virtual void GetAuthInfo(AFAddStreamWidget* widget) {}

	void SetChatId(const QString &chat_id, const std::string &api_chat_id);
	void ResetChat();
    
    QString GenerateState();

private:
	virtual bool RetryLogin() override;
	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;
	virtual void LoadUI() override;

private slots:
	void qslotRedirect(QString code);
	void qslotClose();

private:
	AFAddStreamWidget* m_widget = nullptr;
	bool uiLoaded = false;
	std::string section;

	QString m_redirect_uri;

	AFAuthListener* m_authListner = nullptr;

	//#ifdef BROWSER_AVAILABLE
	//	YoutubeChatDock *chat = nullptr;
	//#endif
};
