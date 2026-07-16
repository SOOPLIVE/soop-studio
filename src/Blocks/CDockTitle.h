#ifndef AFCDOCKTITLE_H
#define AFCDOCKTITLE_H

#include <QFrame>
#include <QMouseEvent>
#include <QAbstractButton>

#include "MainFrame/CTransparentMouseEvents.h"

#include "UIComponent/CCustomMenu.h"

class AFQHoverWidget;
class QPushButton;
class QDockWidget;

namespace Ui {
    class AFDockTitle;
}

class AFDockTitle : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(bool moving READ IsMoving WRITE SetMoving)
    Q_PROPERTY(bool moveandfloat READ IsMoveAndFloat)

public:
    explicit AFDockTitle(QWidget* parent = nullptr);
    ~AFDockTitle();

public slots:
    void qslotShowMenu();
    void qslotShowSceneSourceMenu();
    void qslotHideSceneSourceMenu();
    void qslotToggleDock();
    void qslotMaximumPopupTriggered();
    void qslotMinimumPopupTriggered();
    void qslotRefreshButtonTriggered();
    void qslotCloseButtonTriggered();

    void qslotTransitionScenePopup();
    void qslotAdvAudioMixerPopup();

    void qslotChangeMaximizeIcon(bool maximize);
    void qslotBreaktimeTicked(int remainingSec, int totalSec);
    void qslotBreaktimeFinished();
    void qslotUpdateWindowTitle(QString title);

signals:
    void qsignalMaximumWithType(int);
    void qsignalMinimumWithType(int);
    void qsignalCloseWithType(int);
    void qsignalCloseWithUuid(QString);
    void qsignalMaximizeWithUuid(QString);
    void qsignalMinimizeWithUuid(QString);
    void qsignalToggleDock(bool, int);
    void qsignalAddScene();
    void qsignalAddSource();
    void qsignalTransitionScenePopup();
    void qsignalAdvAudioMixerPopup();
    void qsignalRefreshButton(int);
    void qsignalToggleCustomDock(bool, QString);

public:
    void Initialize(bool onlyPopup, QString text, int BlockType, bool needQuestionMark, const QString& questionMarkToolTip);
    void InitializeCustom(QString customName, QString customUuid, bool minmax, bool threeDots = false, bool closeButton = false);
    void ShowRefreshButton();
    void AddButton(QAbstractButton* button);
    void AddButton(QList<QAbstractButton*> buttons);
    void UpdateTitleLabel();
    QString GetLabelText();
    void SetToggleWindowToDockButton(bool checked); //false: dock Button
    void MinMaxButton(bool visible);
    void DeleteTreeDotsButton();
    void DeleteQuestionMarkButton();
    void DeleteTitleIcon();
    int GetBlockType() { return m_blockType; };

    bool IsMoving() const;
    void SetMoving(bool moving);
    bool IsFloating() const;
    void SetFloating(bool floating);

    void SetHidePopup(bool hidePopup) { m_hidePopup = hidePopup; }
    void SetIsPopup(bool popup) { m_popup = popup; }

    bool IsMoveAndFloat() const;
    void ChangeMaximizedIcon(bool isMaximized);

    void TitleChangePage(int nPage);

    void ChangeLabelFontSize(int fontSize);
    QFont GetFont();

protected:
    void resizeEvent(QResizeEvent* event);

    QSize sizeHint() const override { return minimumSizeHint(); }
    QSize minimumSizeHint() const override;

private:
    void _ToggleWindowToDock(bool popup);
    void _MakeCustomMenu();
    void _ConnectMaxIconChanged();
    QDockWidget* _CheckDock();

private:
    Ui::AFDockTitle* ui = nullptr;
        
    AFQCustomMenu* m_menu = nullptr;
    AFQCustomMenu* m_addSceneSourceMenu = nullptr;
    
    QAction* m_broadInfoSettingAction = nullptr;
    QAction* m_toggleDockAction = nullptr;
    QAction* m_closeAction = nullptr;
    QAction* m_transitionScene = nullptr;
    QAction* m_advAudioMixerShow = nullptr;
    QAction* m_changeAudioMixerLayout = nullptr;
             
    QAction* m_addSceneAction = nullptr;
    QAction* m_addSourceAction = nullptr;
             
    QPoint m_dragPosition;
    
    int m_normalModeWidth = 0;
    bool m_dragInitiated = false;
         
    int  m_blockType = -1;
    bool m_popup = true;
    bool m_hidePopup = false;
         
    bool m_moving = false;
    bool m_floating = false;
    bool m_isMaximized = false;

    static const inline std::unordered_map<int, QString> windowTypesMap = {
        //{ -1, "None" },
        { 0, "SceneSource" },       { 1, "AudioMixer" },    //{ 2, "Null_3" },
        { 3, "SoopChat" },          { 4, "TwitchChat" },    { 5, "YoutubeChat" },       //{ 6, "Null_7" },
        { 7, "BroadInfo" },         { 8, "BLOCKITER" },     { 9, "AdvanceControls" },
        { 10, "SceneControl" },     { 11, "CustomBrowserCollection" },  { 12, "StatPage" },
        { 13, "Mission" },          { 14, "Vote" },         { 15, "Extensions" },
        { 16, "SAVVYReaction" },    { 17, "AquaControl" },  { 18, "SoopOverlay" },
        { 19, "Breaktime" },        { 20, "EventBannerImage" },        { 21, "SubTitle" },
        //{ 22, "ENDOFINDE" }
    };
};

#endif // AFCDOCKTITLE_H