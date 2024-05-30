#ifndef CSTREAMACCOUNT_H
#define CSTREAMACCOUNT_H

#include <QPushButton>

namespace Ui {
class AFQStreamAccount;
}

class QPixmap;

class AFQStreamAccount : public QPushButton
{
	Q_OBJECT;
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
	QString GetID() { return m_sID; }

	void SetPassword(QString password) { m_sPassword = password; }
	QString GetPassword() { return m_sPassword; }

	void SetStreamKey(QString streamkey) { m_sStreamKey = streamkey; };
	QString GetStreamKey() { return m_sStreamKey; }

	void SetServer(QString server) { m_sServer = server; }
	QString GetServer() { return m_sServer; }

	void SetChannelName(QString channelName);
	QString GetChannelName() { return m_sChannelName; }

	void SetChannelNick(QString channelNickName) { m_sChannelNickName = channelNickName; };
	QString GetChannelNick() { return m_sChannelNickName; }

	void SetUuid(QString uuid) { m_sUuid = uuid; }
	QString GetUuid() { return m_sUuid; };

    void SetPixmapProfileImgObj(QPixmap* pObj) { m_pPixmapProfileImg = pObj; }
    QPixmap* GetPixmapProfileImgObj() { return m_pPixmapProfileImg; };
    
    bool GetStateLive() { return m_IsLive; }
	void SetOnLive(bool onlive);
	bool GetOnLive();

	void SetModified(bool modified) { m_IsModified = modified; }
	bool GetModified() { return m_IsModified; }

    void CompleteRegist() { m_IsRegistedChannelModelData = true; }
    
	bool IsCustomService() { return m_sPlatform == "+RTMP"; };
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
	QString m_sServer;
	QString m_sStreamKey;
	QString m_sID;
	QString m_sPassword;
	QString m_sPlatform;
    QPixmap* m_pPixmapProfileImg = nullptr;
    bool m_IsLive = false;
	QString m_sChannelName;
	QString m_sChannelNickName;
	QString m_sUuid;
	bool m_IsModified = false;
    bool m_IsRegistedChannelModelData = false;
#pragma endregion private member var
};

#endif // CSTREAMACCOUNT_H
