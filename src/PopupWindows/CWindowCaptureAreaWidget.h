#pragma once

#include <functional>
#include <memory>
#include <string>
#include <optional>

#include <QObject>
#include <QEvent>
#include <QKeyEvent>
#include <QWidget>
#include <QApplication>
#include <QPainter>
#include <QScreen>
#include <QMouseEvent>
#include <QRect>
#include <QPoint>
#include <QShowEvent>
#include <QHideEvent>
#include <QRegion>

#include "obs.hpp"

// ONLY Windows OS
#include <Windows.h>

// ============================================================================
// ESC Key Filter
// ============================================================================
class EscKeyFilter : public QObject
{
    Q_OBJECT

public:
    explicit EscKeyFilter(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

signals:
    void escapeKeyPressed();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::KeyPress) {

            QKeyEvent* keyEvent =
                static_cast<QKeyEvent*>(event);

            if (keyEvent->key() == Qt::Key_Escape) {
                emit escapeKeyPressed();
                return true;
            }
        }

        return QObject::eventFilter(watched, event);
    }
};

struct window_area_info
{
    HWND window = nullptr;

    std::string id;
    std::string name;

    std::string className;
    std::string executableName;
};

struct WindowCaptureAreaResult
{
    bool desktopMonitor = false;

    std::string windowId;

    int monitor = 0;

    bool useSubregion = false;

    int subregionX = 0;
    int subregionY = 0;

    int subregionWidth = 0;
    int subregionHeight = 0;
};


using WindowSelectedCallback =
std::function<void(
    const std::optional<WindowCaptureAreaResult>&)>;

class WindowCaptureAreaWidget : public QWidget
{
    Q_OBJECT

        using properties_delete_t = decltype(&obs_properties_destroy);
    using properties_t = std::unique_ptr<obs_properties_t, properties_delete_t>;

public:
    static void launch(
        obs_source_t* source,
        QWidget* parent = nullptr,
        WindowSelectedCallback callback = nullptr);

    static void ApplyResult(
        obs_data_t* settings,
        const WindowCaptureAreaResult& result);

    ~WindowCaptureAreaWidget() override;

signals:
    void selectionOperationFinished();

private:
    explicit WindowCaptureAreaWidget(
        QScreen* screen,
        const QVector<window_area_info>& allWindowsInfo,
        obs_source_t* source,
        QWidget* parent = nullptr,
        WindowSelectedCallback callback = nullptr);

private:
    void RaiseSelectionWindow();
    int  GetCurrentMonitorIndex();

    void UpdateDimOverlay();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    static QList<QPointer<WindowCaptureAreaWidget>> activeInstances;

private:
    OBSWeakSourceAutoRelease weakSource;

    std::unique_ptr<obs_properties_t, decltype(&obs_properties_destroy)> props;

    QPointer<QScreen> screen = nullptr;

    QVector<window_area_info> windowAreaInfos;

    QWidget* dimOverlay = nullptr;

    // Global logical coordinates
    QRect highlightedRect;
    QRect dragTargetRect;

    int currentTargetIdx = -1;
    int dragTargetIdx = -1;

    bool dragging = false;

    // Global logical coordinates
    QPoint dragStartPosition;
    QRect currentDragRect;
    QString dragRectSizeString;
    bool mouseInside = false;

    // Global logical coordinates
    QPoint mousePos;
    WindowSelectedCallback selectionCallback;
};