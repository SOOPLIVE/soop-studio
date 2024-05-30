#ifndef AFCWIDGET_H
#define AFCWIDGET_H

#include <QWidget>
#include <QRubberBand>
#include <QPointer>

class AFCQMainBaseWidget : public QWidget
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

public:
    explicit AFCQMainBaseWidget(QWidget* parent = nullptr, Qt::WindowFlags flag = Qt::WindowFlags());
    ~AFCQMainBaseWidget();

signals:
    void qsignalBaseWindowMouseRelease();
    void qsignalBaseWindowMaximized(bool max);

public slots:
    void qslotMaximizeWindow();
    void qslotMinimizeWindow();

#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
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

    inline void setBorderWidth(int w) { m_iBorderWidth = w; }
    inline int borderWidth() const { return m_iBorderWidth; }
    void SetHeightFixed(bool fixed) { m_bIsFixedHeight = fixed; };
    void SetWidthFixed(bool fixed) { m_bIsFixedWidth = fixed; };
    void RestoreNormalWidth(int w) { m_RestoreNormalWidth = w; }

#pragma region public func

#pragma region protected func
protected:
    void mouseHoverEvent(QHoverEvent* e);
    void mouseLeaveEvent(QEvent* e);
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    bool event(QEvent* e) override;
    virtual void changeWidgetBorder(bool isMaximized) {};
#pragma region protected func

#pragma region private func
private:
    void _SetCursorToDefault();
    void _UpdateCursorShape(const QPoint& pos);
    void _CalculateCursorPosition(const QPoint& pos, const QRect& framerect, Edges& _edge);
    void _AdjustWidgetSizeToScreen();
    void _AdjustMaximizeDragPosition(QMouseEvent* event);
#pragma region private func

#pragma region private member var
private:
    QPoint m_CurrentPoint = QPoint();
    QPoint m_NewPoint = QPoint();
    QPoint m_DragPosition = QPoint();

    QPointer<QRubberBand> m_Rubberband = new QRubberBand(QRubberBand::Rectangle);
    Edges m_MousePressedEdge = Edge::None;
    Edges m_MouseMoveEdge = Edge::None;
    int m_iBorderWidth = 8;

    bool m_bCursorChanged = false;
    bool m_bLeftButtonPressed = false;
    bool m_bIsFixedWidth = false;
    bool m_bIsFixedHeight = false;
    bool m_bDragStart = false;

    QSize m_maximumSize;
    QSize m_minimumSize;

    bool m_IsFixedSizeSet = false;
    int  m_RestoreNormalWidth = -1;
#pragma region private member var
};

#endif // AFCWIDGET_H
