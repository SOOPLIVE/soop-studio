#ifndef AFMAINACCOUNTBUTTON_H
#define AFMAINACCOUNTBUTTON_H

#include <QPushButton>

namespace Ui {
class AFMainAccountButton;
}

class AFChannelData;
//
class AFMainAccountButton : public QPushButton
{
    Q_OBJECT

public slots:
    void qslotQuitStream();
    void qslotStartStream();
    void qslotHoverPlatformImage(bool hover);
    void qslotPressedPlatformImage(bool pressed);

signals:
    void qsignalAccountButtonMouseMove();
    void qsignalAccountButtonHover(bool enter);
    void qsignalAccountButtonPressed(bool pressed);

public:
    enum ChannelState {
        Disable = 0,
        LoginWithoutSimulcast = 1,
        LoginWithSimulcast = 2,
        Streaming = 3
    };
    Q_ENUM(ChannelState);

    explicit AFMainAccountButton(QWidget *parent = nullptr);
    ~AFMainAccountButton();

    void SetChannelData(AFChannelData* data);
    AFChannelData* GetChannelData() { return m_pChannelData; };

    void SetStreaming(bool streaming, bool setLive = true, bool disable = false);

    bool IsMainAccount() { return m_isMainAccount; };
    void SetMainAccount() { m_isMainAccount = true; };

    bool GetCurrentState() { return m_currentState; };
    void SetCurrentState(ChannelState state) { m_currentState = state; };

    std::string GetCurrentPlatform() { return m_platformStr; };

    void TransparentPlatformImage(bool transparent);
    void SetPlatform(std::string platform, bool hover = true);
    void SetImage(QPixmap* image);
    QSize SetFixedSize(QSize size);
    void SetChecked(bool checked);
    void checkChecked();

    bool IsLive();
protected:
    bool event(QEvent* event) override;

private:
    Ui::AFMainAccountButton *ui;

    AFChannelData* m_pChannelData = nullptr;
    bool m_isMainAccount = false;
    ChannelState m_currentState = ChannelState::Disable;
    std::string m_platformStr;

    bool m_isTransparent = false;
    bool m_isDisable = false;
};

#endif // AFMAINACCOUNTBUTTON_H