#pragma once

#include <QEvent>
#include <QWidget>
#include <QPointer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

#define SCENE_CONTROL_MIN_SIZE_WIDTH 718
#define SCENE_CONTROL_MIN_SIZE_HEIGTH 444


class AFSimpleHoverEventFilter : public QObject
{
    Q_OBJECT

signals:
    void qsignalHoverEventOccurred(QEvent::Type type);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override 
    {
        if (event->type() == QEvent::HoverEnter ||
            event->type() == QEvent::HoverLeave ||
            event->type() == QEvent::HoverMove)
        {
            emit qsignalHoverEventOccurred(event->type());
            return true;
        }
        return QObject::eventFilter(watched, event);
    };
};

class AFSceneControlWidget final : public QWidget
{
	Q_OBJECT

public:
    explicit AFSceneControlWidget(QWidget* parent = nullptr);
    ~AFSceneControlWidget();

signals:
    void qSignalTransitionButtonClicked();

private slots:
    void qslotStudioModeToggled();

public:
    void ChangeLayoutStudioMode();

private:
    void _InitLayout();
    void _ReleaseLayoutObj();

private:
    QWidget* m_pTransitionAreaContents = nullptr;
    QHBoxLayout* m_pTransitionAreaLayout = nullptr;
    QPushButton* m_pTransitionButton = nullptr;
    //QLabel* m_pTransitionImg = nullptr;
                 
    QWidget* m_pExpandedAreaContents = nullptr;
    QHBoxLayout* m_pExpandedAreaLayout = nullptr;
                 
                 
                 
    QWidget m_contents;
    QVBoxLayout m_mainLayout;
    QPointer<QWidget> m_multiView = nullptr;
};