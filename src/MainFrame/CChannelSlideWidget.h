#ifndef CCHANNELSLIDEWIDGET_H
#define CCHANNELSLIDEWIDGET_H

#include <QWidget>
#include <QFrame>
#include <QPushButton>

#include "icon-label.hpp"

//
class AFQMustRaiseMainFrameEventFilter;
class QResizeEvent;
class AFQToggleButton;
class AFMainAccountButton;

class AFQFreecshotUnInstallButton;
class QLabel;
class QMovie;

#define SERVICE_DASHBOARD       "dashboard"
#define SERVICE_BROADINFO       "broadinfo"
#define SERVICE_CHAT            "chat"
#define SERVICE_OVERLAY         "overlay"
#define SERVICE_SUBTITLE        "subtitle"
#define SERVICE_MISSION         "mission"
#define SERVICE_VOTE            "vote"
#define SERVICE_EXTENSIONS      "extensions"
#define SERVICE_SAVEVOD         "savevod"
#define SERVICE_AQUA_CONTROL    "aquaControl"
#define SERVICE_BREAKTIME       "breaktime"
#define SERVICE_NEWSFEED        "newsfeed"


class ChannelServiceButton : public QFrame
{
    Q_OBJECT
public:
    explicit ChannelServiceButton(QWidget* parent = nullptr);
    ~ChannelServiceButton();

    void SetMenuId(const QString& menuId);
    void SetMenuIcon(const QString& iconDefaultPath, const QString& iconActivePath);
    void SetDisabledMenuIcon(const QString& iconDisabledPath);

    void SetFavoriteState(bool isFavorite);
    void SetActive(bool active);
    void SetDisabled(bool disabled);

private slots:
    void FavoriteButtonClicked(bool checked);

signals:
    void ServiceButtonClicked(QString& menuId);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event);

private:
    IconLabel* icon = nullptr;
    QLabel* buttonInfo = nullptr;
    QPushButton* favoriteButton = nullptr;

    QString menuId;
    QString iconDefaultPath;
    QString iconActivePath;
    QString iconDisabledPath;

    bool isFavorite = false;
    bool active = false;
    bool disabled = false;

    bool mousePressed = false;

};

namespace Ui {
    class AFQChannelSlideWidget;
}

class AFQChannelSlideWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AFQChannelSlideWidget(QWidget *parent = nullptr);
    ~AFQChannelSlideWidget();

public slots:
    void qslotSlideStreamChanged();

private slots:
    void qslotTransmissionToggle(bool checked);
    void qslotOpenDockWithPropertyInSlide();
    void qslotOpenPopupWithPropertyInSlide();
    void qslotOpenChatInSlide();
    void qslotSlideNicknameClicked();
    void qslotPopupVisible(bool visible, int type, bool enableFavoriteMenu);
    void qslotSetOpacity();
    void qslotRemoveOpacity();

signals:
    void qsignalChannelSlideClosed();

public:
    void ChangeSlideInfo(AFMainAccountButton* button);
    void SetChannelSlideGeometry(QRect sliderect);
    QPushButton* CreateServiceButton(const char* locale,
                                     const char* serviceType,
                                     int popupType = -1,     //ENUM_WINDOW_TYPE::None : -1
                                     bool useOpacity = true);


    void ToggleServiceButton(QString type, bool available);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void _SetPlatformButtonArea();
    void _SetTransmissionArea();
    void _NoServiceButtons();
    bool _CheckBlockIsVisible(QString type);
    int  _GetWindowType(QString serviceType);

    QString _AddSpaceToString(QString text);

private:
    Ui::AFQChannelSlideWidget *ui = nullptr;
    AFMainAccountButton* m_pCurrentAccountButton = nullptr;
    AFQToggleButton* m_pTransmissionButton = nullptr;

    QMap<QString, QPushButton*> m_platformServiceButtons;
    QMap<QString, ChannelServiceButton*> serviceButtons; // for Favorite Menu

    // Freecshot Uninstall
    AFQFreecshotUnInstallButton* m_uninstallFreecshotBtn = nullptr;
};

//
class AFQFreecshotUnInstallButton : public QPushButton
{
    Q_OBJECT

public:
    explicit AFQFreecshotUnInstallButton(QWidget* parent = nullptr);
    ~AFQFreecshotUnInstallButton();

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void qslotUnInstallFreecShot();

private:
    QLabel* m_labelGIF = nullptr;
    QLabel* m_labelIcon = nullptr;
    QLabel* m_labelText = nullptr;
    QMovie* m_movieGIF = nullptr;

    QPixmap m_normalIcon;
    QPixmap m_pressedIcon;
};

#endif // CCHANNELSLIDEWIDGET_H