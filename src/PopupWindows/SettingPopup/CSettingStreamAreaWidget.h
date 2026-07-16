#ifndef CSTREAMSETTINGAREAWIDGET_H
#define CSTREAMSETTINGAREAWIDGET_H

#include <QWidget>
#include <QList>
#include <QPointer>

#include "CSettingBroadInfoWidget.h"

class QPixmap;
class AFAddStreamWidget;
class AFQStreamAccount;
class AFMainFrame;
struct AFBasicAuth;
struct AFChannelData;

namespace Ui {
	class AFQStreamSettingAreaWidget;
}

class AFQStreamSettingAreaWidget : public QWidget
{
	Q_OBJECT

#pragma region class initializer, destructor
public:
	AFQStreamSettingAreaWidget(QWidget* parent);
	~AFQStreamSettingAreaWidget();

#pragma endregion class initializer, destructor

#pragma region QT Field, CTOR/DTOR
public slots:
	void qslotShowAddAccount();
	void qslotAuthTriggered();
	void qslotEditTriggred();
	void qslotSimulcastToggled(bool toggled);
	void qslotAccountButtonTriggered(bool selected);
	void qslotReleaseAccount();
	void qslotResetStreamSettingUi();
	void qslotStreamDataChanged();
	void qslotChangeLabel(QString channelName);
	void qslotProfilePictureClicked();
	void qslotGetSoopStreamerInfo(const QByteArray& responseData);
	void qslotRefreshProfilePicture(int code, QString message);
	void qslotClickMainAccount(bool checked);

	void qslotScrollEnter();
	void qslotScrollLeave();

	void qslotAddAccountEnter();
	void qslotAddAccountLeave();

signals:
	void qsignalStreamDataChanged();
	void qsignalModified(QString);
	void qsignalApplyTriggered();
#pragma endregion QT Field

#pragma region public func
public:
	void StreamSettingAreaInit();
	void FindAccountButtonWithID(QString id, QString platform);
	void SetStreamDataChangedVal(bool changed) { m_streamDataChanged = changed; };
	void SetBroadDataChangedVal(bool changed);
	bool StreamDataChanged() { return m_streamDataChanged; };
	bool BroadDataChanged();
	void SaveStreamSettings();
	bool SaveBroadInfoSettings();
	void ToggleOnStreaming(bool streaming);
	void LoadStreamAccountSaved();
	bool LoadMainAccount();
	void RefreshAccountButtons();

	void ReloadBroadInfo();

	QMap<QString, AFQStreamAccount*> GetLiveChannels();
#pragma endregion public func

#pragma region protected func
protected:
#pragma endregion protected func

#pragma region private func
private:
	void                _SetStreamSettings();
	void				_MakeBroadInfoSetting();
	void                _LoadStreamAccountSettings();
	void                _ReleaseAccount();
	void                _ShowSimulcastWithLogin();
	void                _ShowSimulcastWithoutLogin();
	void                _ShowLoginWithoutSimulcast();
	void                _ShowDefaultSetting();
    QPixmap*            _CreateProfileImgObj(AFBasicAuth* pData, AFQStreamAccount*& outRefAccount);
	QPixmap*			_CreateProfileImgObj(AFChannelData* data, AFQStreamAccount*& outRefAccount);
    AFQStreamAccount*   _CreateStreamAccount(AFAddStreamWidget* pUIObjAddStream,
                                             AFBasicAuth* pAuthData,
                                             QString uuid);
    AFQStreamAccount*   _CreateStreamAccount(QString platformName, QString channelName,
                                             QString channelNickName, bool onLive,
                                             QString server, QString streamKey,
                                             QString id, QString password,
                                             QString uuid, bool bFromSavedFile = false);
	void _ModifyStreamAccount(QString server, QString streamkey, QString channelName, QString id, QString password);
    
	bool _FindAccountButtonWithPlatform(QString platform, AFQStreamAccount*& outButton);
	void _ReorderAccountList();
    //
#pragma endregion private func


#pragma region public member var
public:
#pragma endregion public member var

#pragma region private member var
private:
	Ui::AFQStreamSettingAreaWidget* ui;
	QList<QPointer<AFQStreamAccount>> m_accountButtonList;
	QList<QPointer<AFQStreamAccount>> m_releasedAccountButtonList;
	AFQStreamAccount* m_pCurrentAccountButton = nullptr;
	AFQStreamAccount* m_pMainAccountButton = nullptr;

	QPointer<AFQSettingBroadInfoWidget> m_broadInfoWidget;

	bool m_streamDataChanged = false;
	bool m_loading = false;
    static bool s_firstAddAccountUI;

#pragma endregion private member var
};

#endif //CSTREAMSETTINGAREAWIDGET_H
