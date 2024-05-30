#ifndef AFQROUNDEDDIALOGBASE_H
#define AFQROUNDEDDIALOGBASE_H

#include <QDialog>
#include <QRubberBand>
#include <QPointer>

class AFQRoundedDialogBase : public QDialog
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

public:
    explicit AFQRoundedDialogBase(QWidget* parent = nullptr, Qt::WindowFlags flag = Qt::WindowFlags(), bool bModal = true);
    ~AFQRoundedDialogBase();

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
    void SetDragTitleOnly(bool onlyTop) { m_bDragTitleOnly = onlyTop; }
    void SetTitleHeight(float titleHeight) { m_fTitleHeight = titleHeight; }
#pragma endregion public func

#pragma region protected func
protected:
    void mouseHoverEvent(QHoverEvent* e);
    void mouseLeaveEvent(QEvent* e);
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    bool event(QEvent* e) override;
    void paintEvent(QPaintEvent* ev) override;
#pragma endregion protected func

#pragma region private func
private:
    void _SetCursorToDefault();
    void _UpdateCursorShape(const QPoint& pos);
    void _CalculateCursorPosition(const QPoint& pos, const QRect& framerect, Edges& _edge);
#pragma endregion private func

#pragma region private member var
private:
    QPoint m_CurrentPoint = QPoint();
    QPoint m_NewPoint = QPoint();
    QPoint m_DragPosition = QPoint();

    QPointer<QRubberBand> m_Rubberband = new QRubberBand(QRubberBand::Rectangle);
    Edges m_MousePressedEdge = Edge::None;
    Edges m_MouseMoveEdge = Edge::None;
    int m_iBorderWidth = 8;
    float m_fTitleHeight = 38.8f;

    bool m_bCursorChanged = false;
    bool m_bLeftButtonPressed = false;
    bool m_bDragStart = false;
    bool m_bIsFixedWidth = false;
    bool m_bIsFixedHeight = false;
    bool m_bDragTitleOnly = false;

    qreal m_radius = 10.0;        // desired radius in absolute pixels
    qreal m_borderWidth = 0.0;  // -1 : use style hint frame width;  0 : no border;  > 0 : use this width.

    QSize m_maximumSize;
    QSize m_minimumSize;
#pragma endregion private member var
};

#endif // AFQROUNDEDDIALOGBASE_H
