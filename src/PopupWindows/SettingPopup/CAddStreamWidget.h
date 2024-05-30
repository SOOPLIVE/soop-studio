#ifndef CADDSTREAMWIDGET_H
#define CADDSTREAMWIDGET_H

#include <QWidget>
#include "UIComponent/CRoundedDialogBase.h"


#include "CoreModel/Auth/SBaseAuth.h"

namespace Ui {
class AFAddStreamWidget;
}

class QCefWidget;
class QLineEdit;
class AFAuth;

class AFAddStreamWidget : public AFQRoundedDialogBase
{

#pragma region class initializer, destructor
    Q_OBJECT
public:
	explicit AFAddStreamWidget(QWidget* parent = nullptr);
	~AFAddStreamWidget();

#pragma endregion class initializer, destructor

#pragma region QT Field, CTOR/DTOR
public slots:
	void qslotOkTriggered();
	void qslotCustomRtmpTriggered();
	void qslotAuthButtonTriggered();
    void qslotAuthUsage(bool use);
    void qslotToggleStreamKeyHidden(bool show);
    void qslotTogglePasswordHidden(bool show);

signals:

#pragma endregion QT Field

#pragma region public func
public:
	void SetAddStreamButtons();
	void AddStreamWidgetInit();
	void AddStreamWidgetInit(QString platform);
	void EditStreamWidgetInit(QString server, QString streamkey, 
                              QString channelName, QString id, QString password);

    QCefWidget* GetLoginCefWidget(QWidget* parent, const std::string &url);
    void SetAuthData(std::string accessToken,
                     std::string refreshToken,
                     uint64_t expireTime,
                     std::string channelID,
                     std::string channelNick,
                     std::string streamKey = std::string(),
                     std::string streamUrl = std::string());

	QString GetStreamKey();
	QString GetUrl();
	QString GetID();
	QString GetChannelName();
    QString GetChannelNick() { return m_ChannelNick; };
	QString GetPassword();
	QString GetPlatform() { return m_sPlatform; };
    AFBasicAuth& GetRawAuth() { return m_RawAuthData; };

    bool CheckChannel(const char* uuid);

    inline bool LoadFail() const { return m_fail; }

    virtual int exec() override;
    virtual void reject() override;
    virtual void accept() override;

#pragma endregion public func

#pragma region protected func
protected:
#pragma endregion protected func

#pragma region private func
private:
    bool _CacheAuth(std::string accessToken,
                    std::string refreshToken,
                    uint64_t expireTime,
                    std::string channelID,
                    std::string channelNick,
                    std::string streamKey = std::string(),
                    std::string streamUrl = std::string(),
                    std::string uuid = std::string(),
                    std::string customID = std::string(),
                    std::string customPassword = std::string());

    /*bool _ChangeAuth(std::string channelID,
        std::string streamKey = std::string(),
        std::string streamUrl = std::string(),
        std::string customID = std::string(),
        std::string customPassword = std::string());*/
    void _ConvertAuthToStreamData();
    
    void _InitPage();
    
    //void _GetDataUseOAuth();
    //
    void _InitSoopGlobalLoginPage();
    void _InitSoopLoginPage();
    void _InitTwitchLoginPage();
    void _InitYoutubeLoginPage();

    bool _IsCefPlatform();
#pragma endregion private func


#pragma region public member var
public:
#pragma endregion public member var

#pragma region private member var
private:
	Ui::AFAddStreamWidget* ui;

    AFBasicAuth                                 m_RawAuthData;

    std::shared_ptr<AFAuth>                     m_auth = nullptr;
    QCefWidget*                                 m_cefWidget = nullptr;
    
    QLineEdit*                                  m_pIDEditorTestGlobalSoop = nullptr;
    QLineEdit*                                  m_pPWEditorTestGlobalSoop = nullptr;
    //
	QString                                     m_sPlatform ="";
    bool                                        m_bEditMode = false;
    bool                                        m_fail = false;
    QString                                     m_ChannelNick = "";
#pragma endregion private member var
};

#endif // CADDSTREAMWIDGET_H
