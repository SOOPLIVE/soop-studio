#ifndef CSTREAMACCOUNT_H
#define CSTREAMACCOUNT_H

#include <QPushButton>

namespace Ui {
class AFQStreamAccount;
}

class QPixmap;

class AFQStreamAccount : public QPushButton
{
	Q_OBJECT

#pragma region class initializer, destructor
public:
	explicit AFQStreamAccount(QWidget* parent = nullptr);
	~AFQStreamAccount();

	enum AccountStatus {
		Offline = 0,
		LoginWithoutSimulcast,
		SimulcastWithoutLogin,
		SimulcastWithLogin
	};
	Q_ENUM(AccountStatus)

#pragma endregion class initializer, destructor

#pragma region QT Field, CTOR/DTOR
public slots:

signals:
#pragma endregion QT Field

#pragma region public func
public:
	void StreamAccountAreaInit(QString platform, QString channelName, QString channelNickName,
		QString id, QString password, bool onLive, QString server, QString streamKey, QString uuid);
	void SetStreamAccountInfo(QString platform, QString channelName, QString channelNickName,
		QString id, QString password, bool onLive, QString server, QString streamKey, QString uuid);
	
	void SetStreamAccountPlatform(QString platform);
	QString GetStreamAccountPlatform();

	void SetStreamAccountID(QString id);
	QString GetID() { return m_iD; }

	void SetPassword(QString password) { m_password = password; }
	QString GetPassword() { return m_password; }

	void SetStreamKey(QString streamkey) { m_streamKey = streamkey; };
	QString GetStreamKey() { return m_streamKey; }

	void SetServer(QString server) { m_server = server; }
	QString GetServer() { return m_server; }

	void SetChannelName(QString channelName);
	QString GetChannelName() { return m_channelName; }

	void SetChannelNick(QString channelNickName) { m_channelNickName = channelNickName; };
	QString GetChannelNick() { return m_channelNickName; }

	void SetUuid(QString uuid) { m_uuid = uuid; }
	QString GetUuid() { return m_uuid; };

    void SetPixmapProfileImgObj(QPixmap* pObj) { m_pPixmapProfileImg = pObj; }
    QPixmap* GetPixmapProfileImgObj() { return m_pPixmapProfileImg; };
    
    bool GetStateLive() { return m_isLive; }
	void SetOnLive(bool onlive);
	bool GetOnLive();

	void SetModified(bool modified) { m_isModified = modified; }
	bool GetModified() { return m_isModified; }

    void CompleteRegist() { m_isRegistedChannelModelData = true; }
    
	bool IsCustomService() { return m_platform == "+RTMP"; };

	void SetCookie(std::string cookie) { m_cookie = cookie; }
	std::string GetCookie() { return m_cookie; }
#pragma endregion public func

#pragma region protected func
protected:
#pragma endregion protected func

#pragma region private func
private:

#pragma endregion private func


#pragma region public member var
public:
#pragma endregion public member var

#pragma region private member var
private:
	Ui::AFQStreamAccount* ui;
	QString m_server;
	QString m_streamKey;
	QString m_iD;
	QString m_password;
	QString m_platform;
    QPixmap* m_pPixmapProfileImg = nullptr;
    bool m_isLive = false;
	QString m_channelName;
	QString m_channelNickName;
	QString m_uuid;
	bool m_isModified = false;
    bool m_isRegistedChannelModelData = false;
	std::string m_cookie = "";
#pragma endregion private member var
};

#endif // CSTREAMACCOUNT_H
