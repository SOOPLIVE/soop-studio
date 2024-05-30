#ifndef AFMAINACCOUNTBUTTON_H
#define AFMAINACCOUNTBUTTON_H

#include <QPushButton>
#include "CoreModel/Auth/CAuthManager.h"

namespace Ui {
class AFMainAccountButton;
}

class AFMainAccountButton : public QPushButton
{
    Q_OBJECT

public slots:
    void qslotQuitStream();
    void qslotStartStream();

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
    AFChannelData* GetChannelData() { return m_dChannelData; };

    void SetStreaming(bool streaming, bool setLive = true, bool disable = false);

    bool IsMainAccount() { return m_bIsMainAccount; };
    void SetMainAccount() { m_bIsMainAccount = true; };

    bool GetCurrentState() { return m_eCurrentState; };
    void SetCurrentState(ChannelState state) { m_eCurrentState = state; };

    void SetPlatformImage(std::string platform, bool disable = false);
private:


    Ui::AFMainAccountButton *ui;

    AFChannelData* m_dChannelData = nullptr;
    bool m_bIsMainAccount = false;
    ChannelState m_eCurrentState = ChannelState::Disable;
};

#endif // AFMAINACCOUNTBUTTON_H
