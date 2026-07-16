#ifndef CADDSTREAMWIDGET_H
#define CADDSTREAMWIDGET_H

#include <QWidget>
#include "UIComponent/CTopBaseWindow.h"
#include "Application/CApplication.h"

#include "CoreModel/Auth/SBaseAuth.h"

namespace Ui {
class AFAddStreamWidget;
}

class QCefWidget;
class QLineEdit;
class AFAuth;

class AFQResizeDialog : public QDialog
{
#pragma region class initializer, destructor
    Q_OBJECT
public:
    explicit AFQResizeDialog(QWidget* parent = nullptr) : QDialog(parent) {};
    ~AFQResizeDialog() {};


    virtual void showEvent(QShowEvent* event)
    {
        resize(width() + 1, height());
        resize(width() - 1, height());

        QDialog::showEvent(event);
    };

#pragma endregion class initializer, destructor
};

class AFAddStreamWidget : public AFTTopBaseDialog
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
    void qslotCloseTriggered();

    void qslotGetMessageFromLogin(const QString& msg);
    void qslotGetChildPopup(const QString& url);
    void qslotLoginRecieved(const QCefQuery& query);
    void qslotLoginUrlChanged(const QString& url);
signals:

#pragma endregion QT Field

#pragma region public func
public:
	void SetAddStreamButtons();
	void AddStreamWidgetInit(QString platform = "");
	void EditStreamWidgetInit(QString server, QString streamkey, 
                              QString channelName, QString id, QString password);

    QCefWidget* GetLoginCefWidget(QWidget* parent, const std::string &url);
    void SetAuthData(std::string accessToken,
                    std::string refreshToken,
                    uint64_t expireTime,
                    std::string channelID,
                    std::string channelNick,
                    std::string streamKey = std::string(),
                    std::string streamUrl = std::string(),
                    bool loginRetain = false, 
                    std::string clientID = std::string());

    void SetAuthCookie(std::string cookie, std::string channelID);

	QString GetStreamKey();
	QString GetUrl();
	QString GetID();
	QString GetChannelName();
    QString GetChannelNick() { return m_channelNick; };
	QString GetPassword();
	QString GetPlatform() { return m_platform; };
    std::string GetCookie() { return m_tempCookie; };
    AFBasicAuth& GetRawAuth() { return m_rawAuthData; };
    std::string GetThumbnailPath() { return m_thumbnailImgPath; };

    bool CheckChannel(const char* uuid);

    inline bool LoadFail() const { return m_fail; }

    virtual int exec() override;
    virtual void reject() override;
    virtual void accept() override;

    bool IsGlobaltoKrLink() { return m_GlobaltoKrLink; };
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
                    std::string customPassword = std::string(),
                    bool loginRetain = false,
                    std::string clientID = std::string());

    /*bool _ChangeAuth(std::string channelID,
        std::string streamKey = std::string(),
        std::string streamUrl = std::string(),
        std::string customID = std::string(),
        std::string customPassword = std::string());*/
    void _ConvertAuthToStreamData();
    
    void _InitPage();
    
    //
    void _InitSoopGlobalLoginPage();
    void _InitSoopLoginPage();
    void _InitTwitchLoginPage();
    void _InitYoutubeLoginPage();
    void _InitConnectedIconToolTip();

    bool _IsCefPlatform();

    QSize _SnsSize(const QString& url);
    QString _SnsCode(const QString& url);
#pragma endregion private func


#pragma region public member var
public:
#pragma endregion public member var

#pragma region private member var
private:
	Ui::AFAddStreamWidget* ui;

    AFBasicAuth                                 m_rawAuthData;

    std::shared_ptr<AFAuth>                     m_auth = nullptr;
    QCefWidget*                                 m_pCefWidget = nullptr;

    QLineEdit*                                  m_pIDEditorTestGlobalSoop = nullptr;
    QLineEdit*                                  m_pPWEditorTestGlobalSoop = nullptr;
    //
	QString                                     m_platform ="";
    bool                                        m_editMode = false;
    bool                                        m_fail = false;
    bool                                        m_loginRetain = false;
    bool                                        m_saveId = false;
    QString                                     m_channelNick = "";

    AFQResizeDialog*                            m_pSnsDialog = nullptr;
    std::string                                 m_tempCookie = "";
    std::string                                 m_thumbnailImgPath = "";
    bool                                        m_startWithoutPlatform = true;

    bool                                        m_GlobaltoKrLink = false;
#pragma endregion private member var
};

#endif // CADDSTREAMWIDGET_H
