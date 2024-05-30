#ifndef CSTREAMSETTINGAREAWIDGET_H
#define CSTREAMSETTINGAREAWIDGET_H

#include <QWidget>
#include <QList>

class QPixmap;
class AFAddStreamWidget;
class AFQStreamAccount;
class AFMainFrame;
struct AFBasicAuth;

namespace Ui {
	class AFQStreamSettingAreaWidget;
}

class AFQStreamSettingAreaWidget : public QWidget
{
	Q_OBJECT;
#pragma region class initializer, destructor
public:
	AFQStreamSettingAreaWidget(QWidget* parent);
	~AFQStreamSettingAreaWidget();

	

#pragma endregion class initializer, destructor

#pragma region QT Field, CTOR/DTOR
public slots:
	void qslotAddAccountTriggered();
	void qslotAuthTriggered();
	void qslotEditTriggred();
	void qslotSimulcastToggled(bool toggled);
	void qslotAccountButtonTriggered(bool selected);
	void qslotPlatformSelectTriggered();
	void qslotReleaseAccount();
	void qslotResetStreamSettingUi();
	void qslotStreamDataChanged();
	void qslotChangeLabel(QString channelName);

signals:
	void qsignalStreamDataChanged();
	void qsignalModified(QString);
#pragma endregion QT Field

#pragma region public func
public:
	void StreamSettingAreaInit();
	void FindAccountButtonWithID(QString id);
	void SetStreamDataChangedVal(bool changed) { m_bStreamDataChanged = changed; }; 
	bool StreamDataChanged() { return m_bStreamDataChanged; };
	void SaveStreamSettings();
	void ToggleOnStreaming(bool streaming);
	void LoadStreamAccountSaved();
	bool LoadMainAccount();
#pragma endregion public func

#pragma region protected func
protected:
#pragma endregion protected func

#pragma region private func
private:
	void                _SetStreamSettings();
	void                _SetStreamLanguage();
	void                _LoadStreamAccountSettings();
	void                _LoadStreamAccountSettings(AFQStreamAccount* account);
	void                _ReleaseAccount();
	void                _ShowSimulcastWithLogin();
	void                _ShowSimulcastWithoutLogin();
	void                _ShowLoginWithoutSimulcast();
	void                _ShowDefaultSetting();
    QPixmap*            _CreateProfileImgObj(AFBasicAuth* pData, AFQStreamAccount*& outRefAccount);
    AFQStreamAccount*   _CreateStreamAccount(AFAddStreamWidget* pUIObjAddStream,
                                             AFBasicAuth* pAuthData,
                                             QString uuid);
    AFQStreamAccount*   _CreateStreamAccount(QString platformName, QString channelName,
                                             QString channelNickName, bool onLive,
                                             QString server, QString streamKey,
                                             QString id, QString password,
                                             QString uuid, bool bFromSavedFile = false);
	void _ModifyStreamAccount(QString server, QString streamkey, QString channelName, QString id, QString password);
    
    //
#pragma endregion private func


#pragma region public member var
public:
#pragma endregion public member var

#pragma region private member var
private:
	Ui::AFQStreamSettingAreaWidget* ui;
	QList<AFQStreamAccount*> m_AccountButtonList;
	AFQStreamAccount* m_CurrentAccountButton = nullptr;
	AFQStreamAccount* m_MainAccountButton = nullptr;

	bool m_bStreamDataChanged = false;
	bool m_bLoading = false;

    
    static bool s_FirstAddAccountUI;

#pragma endregion private member var
};

#endif //CSTREAMSETTINGAREAWIDGET_H
