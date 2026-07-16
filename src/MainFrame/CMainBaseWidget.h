#ifndef AFCWIDGET_H
#define AFCWIDGET_H

#include <QWidget>
#include <QDialog>

#if 0 //Rubberband
#include <QRubberBand>
#endif

#include <QPointer>

class AFCQMainBaseWidget : public QWidget
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

public:
    explicit AFCQMainBaseWidget(QWidget* parent = nullptr, Qt::WindowFlags flag = Qt::WindowFlags(),
        bool widthResizable = true, bool heightResizable = true);
    ~AFCQMainBaseWidget();

signals:
    void qsignalBaseWindowMouseRelease();
    void qsignalBaseWindowMaximized(bool max);
    void qsignalCloseTriggered();

public slots:
    void qslotMaximizeWindow();
    void qslotMinimizeWindow();

#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
#if 0 //Rubberband
    enum Edge {
        None = 0x0,
        Left = 0x1,
        Top = 0x2,
        Right = 0x4,
        Bottom = 0x8,
        TopLeft = 0x10,
        TopRight = 0x20,
        BottomLeft = 0x40,
        BottomRight = 0x80
    };
    Q_ENUM(Edge);
    Q_DECLARE_FLAGS(Edges, Edge);

    inline void setBorderWidth(int w) { m_borderWidth = w; }
    inline int borderWidth() const { return m_borderWidth; }
#endif

    void SetWidthResizeEnabled(bool enable) { m_widthResizable = enable; };
    void SetHeightResizeEnabled(bool enable) { m_heightResizable = enable; };

    bool ResizeEnabled() const;
    bool WidthResizeEnabled() const;
    bool HeightResizeEnabled() const;
    bool MoveAllArea() const;
    bool HasTitleBar() const;

    int TitleBarHeight() const;

#pragma region public func

#pragma region protected func
protected:
#if 0 //Rubberband
    void mouseHoverEvent(QHoverEvent* e);
    void mouseLeaveEvent(QEvent* e);
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void closeEvent(QCloseEvent* event) override;
    bool event(QEvent* e) override;
#endif
    virtual void changeWidgetBorder(bool isMaximized) {};
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
#pragma region protected func

#pragma region private func
private:
#if 0 //Rubberband
    void _SetCursorToDefault();
    void _UpdateCursorShape(const QPoint& pos);
    void _CalculateCursorPosition(const QPoint& pos, const QRect& framerect, Edges& _edge);
    void _AdjustWidgetSizeToScreen();
    void _AdjustMaximizeDragPosition(QMouseEvent* event);
#endif

#pragma region private func

#pragma region private member var
private:
#if 0 //Rubberband
    QPoint m_currentPoint = QPoint();
    QPoint m_newPoint = QPoint();
    QPoint m_DragPosition = QPoint();

    QPointer<QRubberBand> m_rubberband = new QRubberBand(QRubberBand::Rectangle);
    Edges m_mousePressedEdge = Edge::None;
    Edges m_mouseMoveEdge = Edge::None;
    int m_borderWidth = 8;

    bool m_cursorChanged = false;
    bool m_leftButtonPressed = false;
    bool m_isFixedWidth = false;
    bool m_isFixedHeight = false;
    bool m_dragStart = false;

    QSize m_maximumSize;
    QSize m_minimumSize;

    bool m_isFixedSizeSet = false;
    int  m_restoreNormalWidth = -1;
#endif

    bool m_widthResizable = true;
    bool m_heightResizable = true;
    bool m_moveAllArea = false;
    bool m_firstShow = true;
    int m_titleBarHeight = 40; // <0: auto, =0: no title bar, >0: manual
#pragma region private member var
};

#endif // AFCWIDGET_H
