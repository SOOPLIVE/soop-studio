#ifndef CIMPORTGUIDEPRESET_H
#define CIMPORTGUIDEPRESET_H

#include <QWidget>
#include <QLabel>

#define PRESET_SOURCETYPE_GAMECAPTURE "GameCapture"
#define PRESET_SOURCETYPE_VIDEOCAPTURE "VideoCapture"
#define PRESET_SOURCETYPE_CHATTING "Chatting"
#define PRESET_SOURCETYPE_TARGETGRAPH "TargetGraph"
#define PRESET_SOURCETYPE_GIFTSUBTITLE "GiftSubtitle"
#define PRESET_SOURCETYPE_ALERT "Alert"
#define PRESET_SOURCETYPE_DIRECTBROAD "DirectBroad"

enum class BroadPresetType {
    New,
    Game,
    VisibleRadio,
    RelayBroadcast
};


class AFQImportGuidePreset : public QWidget
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

    enum class EdgeType 
    {
        None,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
        Top,
        Bottom,
        Left,
        Right
    };

public:
    explicit AFQImportGuidePreset(QWidget *parent = nullptr);
    ~AFQImportGuidePreset();

#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void Init(BroadPresetType type);
    const QVector<QPair<QString, QRectF>> GetSourcesGeometry();
#pragma endregion public func

#pragma region protected func
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
#pragma endregion protected func

#pragma region private func
private:
    void _ResizeWidget(QPoint mousePos);
    void _MoveWidget(QPoint mousePos);
    void _SetResizableCursor(EdgeType edge);
    void _ResetStyle();

    QLabel* _CreateLabel(const QString& text, QWidget* parent);

    QVector<QPair<EdgeType, QWidget*>> _GetResizableWidgetAtPoint(QPoint point);
    QWidget* _GetMovableWidgetAtPoint(QPoint point);
    QWidget* _FindChildNear(const QPoint& pos);
    QWidget* _FindChildAtPoint(const QPoint pos);

    void _AddPresetGameCaptureLabel();
    void _AddPresetVisibleRadioLabel();
    void _AddPresetRelayBroadLabel();
#pragma endregion private func

#pragma region private member var
private:
    QWidget* m_pResizedWidget = nullptr;
    EdgeType m_resizeEdge = EdgeType::None;

    QWidget* m_pDraggedWidget = nullptr;
    QPoint m_dragOffset;

    QVector<QLabel*> m_sourceLabelList;
#pragma endregion private member var

};

#endif // CIMPORTGUIDEPRESET_H
