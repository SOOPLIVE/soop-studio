#ifndef CLEFTNAVIGATIONBAR_H
#define CLEFTNAVIGATIONBAR_H

#include <QWidget>
#include <QPointer>
#include <QToolTip>
#include <QPushButton>
#include <QGraphicsOpacityEffect>

#include "MainFrame/CMainFrame.h"

#include "UIComponent/CCustomPushbutton.h"
#include "CChannelSlideWidget.h"

class AFMainAccountButton;
class AFChannelData;
class BalloonTooltip;

namespace Ui {
class AFQLeftNavigationBar;
}

class AFQLeftNavigationBar : public QWidget
{

#pragma region QT Field
    Q_OBJECT

public:
    explicit AFQLeftNavigationBar(QWidget *parent = nullptr);
    ~AFQLeftNavigationBar();

public slots:
    void qslotVirtualCamClicked();
    void qslotVirtualCamConfig();
    void showContextMenu(const QPoint& pos);
    void RecieveBroadState(bool stream);
    void CertainMinuteBroadToggled(bool available);

private slots:
    void qslotChannelSelected(bool clicked);

    void qslotShowAddAccountInNavigationBar();
    void qslotShowToolTip();
    void qslotOnNavigationBarScreenChanged();
    void qslotAccountButtonUIReset();
    void qslotStudioModeClicked();
    void qslotBlockClicked();
    void qslotBlockPressed();
    void qslotBlockReleased();
    void qslotSavvyClicked();
    void qslotButtonPressed();
    void qslotButtonReleased();
    void qslotAddButtonOpacity(bool opacity);
    void qslotSavvyResponse(const QByteArray& responseData);

    void qslotOpenSignature(bool reactionAble);
    void qslotOpenReaction();
    void qslotChangeSavvyStyle(bool opened);

    void qslotChangeVirtualCamStyle(bool opened);

    void qslotCheckSubscribeLive();

    void InitMainFrameAfter();
    void moveBalloonTooltip();

    void ShowMainChannelLNB();


public:
    void qslotBlockStatusChanged(bool visible, int type, bool enableFavoriteMenu);
    void qslotCloseSignature(bool opened);
    void qslotStudioModeStatusChanged(bool studioMode);

signals:
    void qsignalBlockButtonTriggered(bool, int);
    void qsignalBlockPopupButtonTriggered(bool, int);
    void qsignalAddButtonOpacity(bool opacity);
#pragma endregion QT Field


#pragma region public func
public:
    bool LeftNavigationBarInit();
    void ConnectNavigationBarScreen();
    void MoveChannelSlide();
    bool LoadNavigationAccounts();
    
    void HideChannelSlide();
    void ToggleAddChannelButton(bool isStream);

    bool FindChannelButton(std::string platform, AFMainAccountButton*& outbutton);

    void SelectChannelSlide(std::string platform);
    void SelectChannleSlide(AFMainAccountButton* button);
    void ResetChannelSlide();

    QPushButton* CreateFavoriteLnbMenuButton(const LnbMenuItem& item);
    void SetFavoriteLnbMenuEmptyState(bool empty);
    void ClearFavoriteLnbMenuButtons();
    void RefreshFavoriteLnbMenuButtons();
    void UpdateFavoriteLnbMenuButtons(const QString& menuId);
    void UpdateFavoriteLnbMenuAreaSize();

    void UpdateVirtualCamIconState(bool active);

#pragma endregion public func

#pragma region private func
private:
    bool _LoadMultiStreamAccounts();
    void _MakeAccountButton(AFChannelData* tmpChannel, AFMainAccountButton*& outButton);
    void _LoadBlocks();
    bool _SetBaseAccounts();

    void _InitScrollBarHoverEvent();
#pragma endregion private func


protected:
    bool eventFilter(QObject* watched, QEvent* event);
    
    void HandleExtraMenuEvent(QObject* watched, QEvent* event);
    void HandleScrollAreaHoverEvent(QObject* watched, QEvent* event, 
                                    QScrollArea* scrollArea, QWidget* contentsWidget);

#pragma region private var
private:
    Ui::AFQLeftNavigationBar *ui;

    //QPointer<AFQChannelSlide> m_ChannelSlide;  ->  AFQChannelSlide -> AFQChannelSlideWidget ??

    AFMainAccountButton* m_pSelectedChannelNum = nullptr;
    QPointer<AFQChannelSlideWidget> m_channelSlide;
    QPointer<QPushButton> m_accountAddButton = nullptr;
    QPointer<AFQCustomPushbutton> m_eventButton = nullptr;
    QPointer<AFQCustomPushbutton> m_sceneSourceButton = nullptr;
    QPointer<AFQCustomPushbutton> m_audioMixerButton = nullptr;

    QPointer<BalloonTooltip> balloonTooltip = nullptr;

    QPointer<AFQCustomPushbutton> savygButton = nullptr;
    QPointer<AFQCustomPushbutton> virtualCamButton = nullptr;
    QPointer<AFQCustomPushbutton> studioModeButton = nullptr;

    QPointer<QFrame> extraMenu = nullptr;
    QPointer<QPushButton> extraMenuButton = nullptr;


    QVector<QPointer<QPushButton>> favoriteLnbMenuButtons;

    bool ignoreExtraMenuButtonClicked = false;
    bool m_slideAnimation = false;

#pragma region private var
};

#endif // CLEFTNAVIGATIONBAR_H
