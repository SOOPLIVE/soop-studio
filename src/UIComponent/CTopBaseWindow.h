#ifndef AFTTOPBASEWINDOW_H
#define AFTTOPBASEWINDOW_H

#include <QWidget>
#include <QDialog>
#include <QEvent>

#include <type_traits>

// AFTTopBaseWindow : Provides WindowOS Caption

// Caption : Provides Move
// Client  : Interaction With Components

// Default : Client area (No Move) -> Need to set Caption Area for Move
// static void AFQBlockManager::ApplyMoveInAllArea(QObject* applyWidget) : Apply Caption Area
// Resizable to handle resize
// Need To Set on Constructor


class AFQBaseWindowController : public QObject
{
    Q_OBJECT
public:
    AFQBaseWindowController(QWidget *parent = nullptr) : QObject(parent)
    {
        m_pWindow = parent;
    };

    void setWindow(QWidget* w)
    {
        m_pWindow = w;
    };

    void ExitSizeMoveSignalTrigger();
    void CallSignalMaximized(bool maximized);

signals:
    void qsignalExitSizeMove();
    void qsignalMaximized(bool maximized);

public slots:
    void minimizeWindow() {
        if (m_pWindow) {
            m_pWindow->showMinimized();
        }
    }

    void maximizeWindow() {
        if (m_pWindow) {
            if (m_pWindow->isMaximized()) {
                m_pWindow->showNormal();
            }
            else {
                m_pWindow->showMaximized();
            }
        }
    }

private:
    QWidget* m_pWindow;
};


template <typename T, typename = std::enable_if_t<std::is_base_of<QWidget, T>::value>>
class AFTTopBaseWindow : public T
{
public:
    AFTTopBaseWindow(QWidget* parent = nullptr, Qt::WindowFlags flag = Qt::WindowFlags()) : T(parent, flag)
    {
        m_pController = new AFQBaseWindowController(this);
    };

    ~AFTTopBaseWindow() {};

    void SetWidthResizeEnabled(bool enable) { m_widthResizable = enable; };
    void SetHeightResizeEnabled(bool enable) { m_heightResizable = enable; };
    void SetTitleBarHeight(int titleBar) { m_titleBarHeight = titleBar; };

    bool ResizeEnabled() const { return m_widthResizable && m_heightResizable; };
    bool WidthResizeEnabled() const { return m_widthResizable; };
    bool HeightResizeEnabled() const { return m_heightResizable; };
    bool MoveInAllArea() const { return m_moveInAllArea; };
    bool HasTitleBar() const { return TitleBarHeight(); };

    int TitleBarHeight() const { return m_titleBarHeight; };
    AFQBaseWindowController* GetController() { return m_pController; };

    bool GetAboutToMax() { return m_aboutToMaximize; };
    void FinishAboutToMax() { m_aboutToMaximize = false; };

protected:
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
    void changeEvent(QEvent* event) override;

private:
    bool m_widthResizable = true;
    bool m_heightResizable = true;
    bool m_moveInAllArea = true;
    bool m_firstShow = true;
    bool m_noMove = false;
    int m_titleBarHeight = -1; // <0: auto, =0: no title bar, >0: manual

    bool m_aboutToMaximize = false;   
    bool m_wasMinimize = false;

    AFQBaseWindowController* m_pController;
};

using AFTTopBaseWidget = AFTTopBaseWindow<QWidget>;
using AFTTopBaseDialog = AFTTopBaseWindow<QDialog>;


#endif // AFTTOPBASEWINDOW_H
