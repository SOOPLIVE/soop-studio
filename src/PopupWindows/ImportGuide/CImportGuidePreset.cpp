#include "CImportGuidePreset.h"

#include <QMouseEvent>
#include <QLabel>

#include "qt-wrappers.hpp"
#include "Application/CApplication.h"

#define EDGE_MARGIN 10
#define LINE_MARGIN 5
#define MINIMUM_WIDTH 70
#define MINIMUM_HEIGHT 40

// Top Left Edge Check
bool IsOnTopLeftEdge(const QPoint& pos, const QWidget* widget)
{
    if (widget->pos().x() + EDGE_MARGIN >= pos.x() &&
        widget->pos().x() - EDGE_MARGIN <= pos.x() &&
        widget->pos().y() + EDGE_MARGIN >= pos.y() &&
        widget->pos().y() - EDGE_MARGIN <= pos.y())
        return true;
    return false;
}

// Top Right Edge Check
bool IsOnTopRightEdge(const QPoint& pos, const QWidget* widget) {
    if (widget->pos().x() + widget->width() + EDGE_MARGIN >= pos.x() &&
        widget->pos().x() + widget->width() - EDGE_MARGIN <= pos.x() &&
        widget->pos().y() + EDGE_MARGIN >= pos.y() &&
        widget->pos().y() - EDGE_MARGIN <= pos.y())
        return true;
    return false;
}

// Bottom Left Edge Check
bool IsOnBottomLeftEdge(const QPoint& pos, const QWidget* widget) {
    if (widget->pos().x() + EDGE_MARGIN >= pos.x() &&
        widget->pos().x() - EDGE_MARGIN <= pos.x() &&
        widget->pos().y() + widget->height() + EDGE_MARGIN >= pos.y() &&
        widget->pos().y() + widget->height() - EDGE_MARGIN <= pos.y())
        return true;
    return false;
}

// Bottom Right Edge Check
bool IsOnBottomRightEdge(const QPoint& pos, const QWidget* widget) {
    if (widget->pos().x() + widget->width() + EDGE_MARGIN >= pos.x() &&
        widget->pos().x() + widget->width() - EDGE_MARGIN <= pos.x() &&
        widget->pos().y() + widget->height() + EDGE_MARGIN >= pos.y() &&
        widget->pos().y() + widget->height() - EDGE_MARGIN <= pos.y())
        return true;
    return false;
}

double PointToLineDistance(const QPoint& point, const QPoint& linePointA, const QPoint& linePointB) {
    double dx = linePointB.x() - linePointA.x();
    double dy = linePointB.y() - linePointA.y();

    // Line Length
    double lineLen = std::sqrt(dx * dx + dy * dy);
    if (lineLen < 1e-6) // B == C
        // Return Distance point <-> linePointA
        return std::sqrt(std::pow(point.x() - linePointA.x(), 2) + std::pow(point.y() - linePointA.y(), 2));

    double t = ((point.x() - linePointA.x()) * dx + (point.y() - linePointA.y()) * dy) / (lineLen * lineLen);
    t = std::max(0.0, std::min(1.0, t));

    double px = linePointA.x() + t * dx;
    double py = linePointA.y() + t * dy;

    return std::sqrt(std::pow(point.x() - px, 2) + std::pow(point.y() - py, 2));
}

// Top Side Check
bool IsOnTopSide(const QPoint& pos, const QWidget* widget) 
{
    QPoint topLeft(widget->pos());
    QPoint topRight(widget->pos().x() + widget->width(), widget->pos().y());

    double distance = PointToLineDistance(pos, topLeft, topRight);
    return distance <= LINE_MARGIN ? true : false;
}

// Bottom Side Check
bool IsOnBottomSide(const QPoint& pos, const QWidget* widget)
{
    QPoint bottomLeft(widget->pos().x(), widget->pos().y() + widget->height());
    QPoint bottomRight(widget->pos().x() + widget->width(), widget->pos().y() + widget->height());

    double distance = PointToLineDistance(pos, bottomLeft, bottomRight);
    return distance <= LINE_MARGIN ? true : false;
}

// Left Side Check
bool IsOnLeftSide(const QPoint& pos, const QWidget* widget)
{
    QPoint leftTop(widget->pos());
    QPoint leftBottom(widget->pos().x(), widget->pos().y() + widget->height());

    double distance = PointToLineDistance(pos, leftTop, leftBottom);
    return distance <= LINE_MARGIN ? true : false;
}

// Right Side Check
bool IsOnRightSide(const QPoint& pos, const QWidget* widget)
{
    QPoint rightTop(widget->pos().x() + widget->width(), widget->pos().y());
    QPoint rightBottom(widget->pos().x() + widget->width(), widget->pos().y() + widget->height());

    double distance = PointToLineDistance(pos, rightTop, rightBottom);
    return distance <= LINE_MARGIN ? true : false;
}

AFQImportGuidePreset::AFQImportGuidePreset(QWidget *parent) :
    QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setMouseTracking(true);
}

AFQImportGuidePreset::~AFQImportGuidePreset()
{

}

void AFQImportGuidePreset::Init(BroadPresetType type)
{
    if (type == BroadPresetType::Game)
    {
        _AddPresetGameCaptureLabel();
    }
    else if (type == BroadPresetType::VisibleRadio)
    {
        _AddPresetVisibleRadioLabel();
    }
    else if (type == BroadPresetType::RelayBroadcast)
    {
        _AddPresetRelayBroadLabel();
    }
    else
        return;
}

const QVector<QPair<QString, QRectF>> AFQImportGuidePreset::GetSourcesGeometry()
{
    QVector<QPair<QString, QRectF>> sourcesGeometry;

    for (QLabel* label : m_sourceLabelList)
    {
        if (label)
        {
            QString sourceType = label->property("sourceType").toString();
            bool sourceMovable = label->property("isMovable").toBool();
            if (!sourceType.isEmpty()) 
            {
                float relativeWidth;
                float relativeHeight;
                float relativePosX;
                float relativePosY;

                if (!sourceMovable)
                {
                    relativeWidth = 1.f;
                    relativeHeight = 1.f;
                    relativePosX = 0.f;
                    relativePosY = 0.f;
                }
                else 
                {
                    relativeWidth = (float)label->width() / this->width();
                    relativeHeight = (float)label->height() / this->height();
                    relativePosX = (float)label->x() / this->width();
                    relativePosY = (float)label->y() / this->height();
                }

                QRectF relativeRect(relativePosX, relativePosY, relativeWidth, relativeHeight);
                sourcesGeometry.append(qMakePair(sourceType, relativeRect));
            }
        }
    }

    return sourcesGeometry;
}

void AFQImportGuidePreset::mousePressEvent(QMouseEvent* event) 
{
    QPoint clickPos = event->pos();

    // Set To Default Style
    _ResetStyle();

    m_pResizedWidget = nullptr;
    m_pDraggedWidget = nullptr;
    m_resizeEdge = EdgeType::None;

    // Find Resizable Child
    QVector<QPair<EdgeType, QWidget*>> resizeChildWidget = _GetResizableWidgetAtPoint(clickPos);
    if (!resizeChildWidget.isEmpty())
    {
        m_resizeEdge = resizeChildWidget[0].first;
        m_pResizedWidget = resizeChildWidget[0].second;
    }

    // Find Movable Child
    if (!m_pResizedWidget)
    {
        m_pDraggedWidget = _GetMovableWidgetAtPoint(clickPos);
        if (m_pDraggedWidget)
            m_dragOffset = m_pDraggedWidget->pos() - clickPos;
    }

    // Set To Selected Style
    if (m_pResizedWidget) {
        m_pResizedWidget->setProperty("isSelected", true);
        PolishStyleSheet(m_pResizedWidget);
    }

    if (m_pDraggedWidget) {
        m_pDraggedWidget->setProperty("isSelected", true);
        PolishStyleSheet(m_pDraggedWidget);
    }
}

void AFQImportGuidePreset::mouseMoveEvent(QMouseEvent* event) 
{
    QPoint mousePos = event->pos();

    // Resize Widget
    if (m_pResizedWidget) {
        _ResizeWidget(mousePos);
        return;
    }
    // Move Widget
    else if (m_pDraggedWidget) {
        _MoveWidget(mousePos);
        return;
    }

    // Hover Check
    QWidget* resizeableWidget = nullptr;
    QWidget* movableWidget = nullptr;
    auto resizableData = _GetResizableWidgetAtPoint(mousePos);

    // Set Resizable Cursor
    if (!resizableData.isEmpty() && resizableData[0].first != EdgeType::None) {
        resizeableWidget = resizableData[0].second;
        _SetResizableCursor(resizableData[0].first);
    }

    // Set Movable Cursor
    if (!resizeableWidget) {
        movableWidget = _GetMovableWidgetAtPoint(mousePos);
        if (movableWidget)
            setCursor(Qt::SizeAllCursor);
    }

    if (!resizeableWidget && !movableWidget)
        setCursor(Qt::ArrowCursor);

    event->accept();
}

void AFQImportGuidePreset::mouseReleaseEvent(QMouseEvent* event) 
{
    // Set To Default Style
    _ResetStyle();

    m_pDraggedWidget = nullptr;
    m_pResizedWidget = nullptr;
    
    m_resizeEdge = EdgeType::None;
    m_dragOffset = QPoint(0, 0);

    setCursor(Qt::ArrowCursor);
}

void AFQImportGuidePreset::_ResizeWidget(QPoint mousePos) 
{
    if (m_pResizedWidget == nullptr)
        return;

    int newWidth = 0;
    int newHeight = 0;
    int newPosX = 0;
    int newPosY = 0;

    // Check Mouse X Pos
    if (mousePos.x() < 0)
        mousePos.setX(0);
    else if (mousePos.x() > this->width())
        mousePos.setX(this->width());
    // Check Mouse Y Pos
    if (mousePos.y() < 0)
        mousePos.setY(0);
    else if (mousePos.y() > this->height())
        mousePos.setY(this->height());

    // Set Souce Label Size, Position
    if (m_resizeEdge == EdgeType::TopLeft)
    {
        newWidth = m_pResizedWidget->width() + (m_pResizedWidget->x() - mousePos.x());
        newHeight = m_pResizedWidget->height() + (m_pResizedWidget->y() - mousePos.y());
        newPosX = mousePos.x();
        newPosY = mousePos.y();
    }
    else if (m_resizeEdge == EdgeType::TopRight)
    {
        newWidth = mousePos.x() - m_pResizedWidget->x();
        newHeight = m_pResizedWidget->height() + (m_pResizedWidget->y() - mousePos.y());
        newPosX = m_pResizedWidget->x();
        newPosY = mousePos.y();
    }
    else if (m_resizeEdge == EdgeType::BottomLeft)
    {
        newWidth = m_pResizedWidget->width() + (m_pResizedWidget->x() - mousePos.x());
        newHeight = mousePos.y() - m_pResizedWidget->y();
        newPosX = mousePos.x();
        newPosY = m_pResizedWidget->y();
    }
    else if (m_resizeEdge == EdgeType::BottomRight)
    {
        newWidth = mousePos.x() - m_pResizedWidget->x();
        newHeight = mousePos.y() - m_pResizedWidget->y();
        newPosX = m_pResizedWidget->x();
        newPosY = m_pResizedWidget->y();
    }
    else if (m_resizeEdge == EdgeType::Top)
    {
        newWidth = m_pResizedWidget->width();
        newHeight = m_pResizedWidget->height() + (m_pResizedWidget->y() - mousePos.y());
        newPosX = m_pResizedWidget->x();
        newPosY = mousePos.y();
    }
    else if (m_resizeEdge == EdgeType::Bottom)
    {
        newWidth = m_pResizedWidget->width();
        newHeight = mousePos.y() - m_pResizedWidget->y();
        newPosX = m_pResizedWidget->x();
        newPosY = m_pResizedWidget->y();
    }
    else if (m_resizeEdge == EdgeType::Left)
    {
        newWidth = m_pResizedWidget->width() + (m_pResizedWidget->x() - mousePos.x());
        newHeight = m_pResizedWidget->height();
        newPosX = mousePos.x();
        newPosY = m_pResizedWidget->y();
    }
    else if (m_resizeEdge == EdgeType::Right)
    {
        newWidth = mousePos.x() - m_pResizedWidget->x();
        newHeight = m_pResizedWidget->height();
        newPosX = m_pResizedWidget->x();
        newPosY = m_pResizedWidget->y();
    }
    else
        return;

    // Check Size
    if (newWidth <= MINIMUM_WIDTH) {
        newPosX = m_pResizedWidget->x();
        newWidth = m_pResizedWidget->width();
    }
    if (newHeight <= MINIMUM_HEIGHT) {
        newPosY = m_pResizedWidget->y();
        newHeight = m_pResizedWidget->height();
    }
    //

    m_pResizedWidget->setGeometry(newPosX, newPosY, newWidth, newHeight);
}

void AFQImportGuidePreset::_MoveWidget(QPoint mousePos)
{
    QPoint newPos = mousePos + m_dragOffset;

    // Check X Pos
    if (newPos.x() < 0)
        newPos.setX(0);
    else if (newPos.x() + m_pDraggedWidget->width() > this->width())
        newPos.setX(this->width() - m_pDraggedWidget->width());
    // Check Y Pos
    if (newPos.y() < 0)
        newPos.setY(0);
    else if (newPos.y() + m_pDraggedWidget->height() > this->height())
        newPos.setY(this->height() - m_pDraggedWidget->height());

    m_pDraggedWidget->move(newPos);
}

void AFQImportGuidePreset::_SetResizableCursor(EdgeType edge)
{
    if (edge == EdgeType::TopLeft || edge == EdgeType::BottomRight)
        setCursor(Qt::SizeFDiagCursor);
    else if (edge == EdgeType::TopRight || edge == EdgeType::BottomLeft)
        setCursor(Qt::SizeBDiagCursor);
    else if (edge == EdgeType::Top || edge == EdgeType::Bottom)
        setCursor(Qt::SizeVerCursor);
    else if (edge == EdgeType::Left || edge == EdgeType::Right)
        setCursor(Qt::SizeHorCursor);
    else
        setCursor(Qt::ArrowCursor);
}

void AFQImportGuidePreset::_ResetStyle()
{
    if (m_pResizedWidget) {
        m_pResizedWidget->setProperty("isSelected", false);
        PolishStyleSheet(m_pResizedWidget);
    }

    if (m_pDraggedWidget) {
        m_pDraggedWidget->setProperty("isSelected", false);
        PolishStyleSheet(m_pDraggedWidget);
    }
}

QLabel* AFQImportGuidePreset::_CreateLabel(const QString& text, QWidget* parent) 
{
    QLabel* label = new QLabel(text, parent);
    label->setAlignment(Qt::AlignCenter);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    return label;
}

QVector<QPair<AFQImportGuidePreset::EdgeType, QWidget*>> AFQImportGuidePreset::_GetResizableWidgetAtPoint(QPoint point)
{
    QVector<QPair<EdgeType, QWidget*>> resizeWidgetData;
    resizeWidgetData.clear();

    QWidget* resizeChildWidget = _FindChildNear(point);
    EdgeType edgeType = EdgeType::None;

    // Find Resizable Child
    if (resizeChildWidget)
    {
        if (IsOnTopLeftEdge(point, resizeChildWidget)) {
            edgeType = EdgeType::TopLeft;
        }
        else if (IsOnTopRightEdge(point, resizeChildWidget)) {
            edgeType = EdgeType::TopRight;
        }
        else if (IsOnBottomLeftEdge(point, resizeChildWidget)) {
            edgeType = EdgeType::BottomLeft;
        }
        else if (IsOnBottomRightEdge(point, resizeChildWidget)) {
            edgeType = EdgeType::BottomRight;
        }
        else if (IsOnTopSide(point, resizeChildWidget)) {
            edgeType = EdgeType::Top;
        }
        else if (IsOnBottomSide(point, resizeChildWidget)) {
            edgeType = EdgeType::Bottom;
        }
        else if (IsOnLeftSide(point, resizeChildWidget)) {
            edgeType = EdgeType::Left;
        }
        else if (IsOnRightSide(point, resizeChildWidget)) {
            edgeType = EdgeType::Right;
        }
    }
    
    if (edgeType == EdgeType::None)
        resizeChildWidget = nullptr;

    resizeWidgetData.append(qMakePair(edgeType, resizeChildWidget));
    return resizeWidgetData;
}

QWidget* AFQImportGuidePreset::_GetMovableWidgetAtPoint(QPoint point)
{
    //QWidget* childWidget = this->childAt(point);
    QWidget* childWidget = _FindChildAtPoint(point);
    if (childWidget && childWidget != this)
    {
        QVariant isMovable = childWidget->property("isMovable");
        if (isMovable.toBool() == false)
            childWidget = nullptr;
    }
    return childWidget;
}

QWidget* AFQImportGuidePreset::_FindChildNear(const QPoint& pos) 
{
    for (int dx = -EDGE_MARGIN; dx <= EDGE_MARGIN; ++dx) 
    {
        for (int dy = -EDGE_MARGIN; dy <= EDGE_MARGIN; ++dy)
        {
            QPoint adjustedPos = pos + QPoint(dx, dy);
           // QWidget* child = this->childAt(adjustedPos);
            QWidget* child = _FindChildAtPoint(adjustedPos);

            if (child && child != this)
            {
                QVariant isMovable = child->property("isMovable");
                if (isMovable.toBool()) {
                    return child;
                }
            }
        }
    }
    return nullptr;
}

QWidget* AFQImportGuidePreset::_FindChildAtPoint(const QPoint pos)
{
    QWidget* childWidget = nullptr;

    QList<QWidget*> allChildren = findChildren<QWidget*>();
    for (QWidget* child : allChildren) {
        if (child->geometry().contains(pos)) {
            childWidget = child;
            break;
        }
    }

    return childWidget;
}

void AFQImportGuidePreset::_AddPresetGameCaptureLabel()
{
    /******************* Movable Source *******************/
    // Alert
    QLabel* alertLabel = _CreateLabel(QTStr(PRESET_SOURCETYPE_ALERT), this);
    alertLabel->setGeometry(14, 14, 148, 79);

    // Chatting
    QLabel* chattingLabel = _CreateLabel(QTStr(PRESET_SOURCETYPE_CHATTING), this);
    chattingLabel->setGeometry(404, 14, 168, 180);

    // Video Capture
    QLabel* videoCaptureLabel = _CreateLabel(QTStr(PRESET_SOURCETYPE_VIDEOCAPTURE), this);
    videoCaptureLabel->setGeometry(404, 204, 168, 112);

    // Set Movable Property
    alertLabel->setProperty("isMovable", true);
    chattingLabel->setProperty("isMovable", true);
    videoCaptureLabel->setProperty("isMovable", true);
    /******************************************************/

    /***************** Non-Movable Source *****************/
    // Game Capture
    QLabel* gameCaptureLabel = _CreateLabel(QTStr(PRESET_SOURCETYPE_GAMECAPTURE), this);
    gameCaptureLabel->setGeometry(243, 115, 100, 100);
    /******************************************************/

    // Add To Source List
    gameCaptureLabel->setProperty("sourceType", PRESET_SOURCETYPE_GAMECAPTURE);
    alertLabel->setProperty("sourceType", PRESET_SOURCETYPE_ALERT);
    chattingLabel->setProperty("sourceType", PRESET_SOURCETYPE_CHATTING);
    videoCaptureLabel->setProperty("sourceType", PRESET_SOURCETYPE_VIDEOCAPTURE);

    m_sourceLabelList.clear();
    m_sourceLabelList.append(gameCaptureLabel);
    m_sourceLabelList.append(alertLabel);
    m_sourceLabelList.append(chattingLabel);
    m_sourceLabelList.append(videoCaptureLabel);

    // Set Style Property
    gameCaptureLabel->setProperty("labelType", "labelPresetFullScreenSource");
    alertLabel->setProperty("labelType", "labelPresetSource");
    chattingLabel->setProperty("labelType", "labelPresetSource");
    videoCaptureLabel->setProperty("labelType", "labelPresetSource");

    /*************** Preset Name, Img Label ***************/
    // Preset Label Img
    QLabel* presetImgLabel = _CreateLabel("", this);
    presetImgLabel->setGeometry(20, 285, 24, 24);
    presetImgLabel->setObjectName("label_PresetGameImg");

    // Preset Label Text
    QLabel* presetTextLabel = _CreateLabel(QTStr("Game"), this);
    presetTextLabel->move(50, 285);
    presetTextLabel->setObjectName("label_PresetTxt");
    /******************************************************/
}

void AFQImportGuidePreset::_AddPresetVisibleRadioLabel()
{
    /******************* Movable Source *******************/
    // Alert
    QLabel* alertLabel = _CreateLabel(QTStr(PRESET_SOURCETYPE_ALERT), this);
    alertLabel->setGeometry(14, 14, 148, 79);

    // Target Graph
    QLabel* targetGraphLabel = _CreateLabel(QTStr(PRESET_SOURCETYPE_TARGETGRAPH), this);
    targetGraphLabel->setGeometry(400, 14, 172, 40);

    // Gift Subtitle
    QLabel* giftSubtitleLabel = _CreateLabel(QTStr(PRESET_SOURCETYPE_GIFTSUBTITLE), this);
    giftSubtitleLabel->setGeometry(400, 64, 172, 40);

    // Chatting
    QLabel* chattingLabel = _CreateLabel(QTStr(PRESET_SOURCETYPE_CHATTING), this);
    chattingLabel->setGeometry(400, 114, 172, 202);

    // Set Movable Property
    alertLabel->setProperty("isMovable", true);
    targetGraphLabel->setProperty("isMovable", true);
    giftSubtitleLabel->setProperty("isMovable", true);
    chattingLabel->setProperty("isMovable", true);
    /******************************************************/

    /***************** Non-Movable Source *****************/
    // Video Capture
    QLabel* videoCaptureLabel = _CreateLabel(QTStr(PRESET_SOURCETYPE_VIDEOCAPTURE), this);
    videoCaptureLabel->setGeometry(243, 115, 100, 100);
    /******************************************************/

    // Add To Source List
    videoCaptureLabel->setProperty("sourceType", PRESET_SOURCETYPE_VIDEOCAPTURE);
    alertLabel->setProperty("sourceType", PRESET_SOURCETYPE_ALERT);
    targetGraphLabel->setProperty("sourceType", PRESET_SOURCETYPE_TARGETGRAPH);
    giftSubtitleLabel->setProperty("sourceType", PRESET_SOURCETYPE_GIFTSUBTITLE);
    chattingLabel->setProperty("sourceType", PRESET_SOURCETYPE_CHATTING);

    m_sourceLabelList.clear();
    m_sourceLabelList.append(videoCaptureLabel);
    m_sourceLabelList.append(alertLabel);
    m_sourceLabelList.append(targetGraphLabel);
    m_sourceLabelList.append(giftSubtitleLabel);
    m_sourceLabelList.append(chattingLabel);

    // Set Style Property
    videoCaptureLabel->setProperty("labelType", "labelPresetFullScreenSource");
    alertLabel->setProperty("labelType", "labelPresetSource");
    targetGraphLabel->setProperty("labelType", "labelPresetSource");
    giftSubtitleLabel->setProperty("labelType", "labelPresetSource");
    chattingLabel->setProperty("labelType", "labelPresetSource");

    /*************** Preset Name, Img Label ***************/
    // Preset Label Img
    QLabel* presetImgLabel = _CreateLabel("", this);
    presetImgLabel->setGeometry(20, 285, 24, 24);
    presetImgLabel->setObjectName("label_PresetVisibleRadioImg");

    // Preset Label Text
    QLabel* presetTextLabel = _CreateLabel(QTStr("VisibleRadio"), this);
    presetTextLabel->move(50, 285);
    presetTextLabel->setObjectName("label_PresetTxt");
    /******************************************************/
}

void AFQImportGuidePreset::_AddPresetRelayBroadLabel()
{
    /******************* Movable Source *******************/
    // Alert
    QLabel* alertLabel = _CreateLabel(QTStr(PRESET_SOURCETYPE_ALERT), this);
    alertLabel->setGeometry(14, 193, 148, 79);

    // Chatting
    QLabel* chattingLabel = _CreateLabel(QTStr(PRESET_SOURCETYPE_CHATTING), this);
    chattingLabel->setGeometry(404, 14, 168, 180);

    // Video Capture
    QLabel* videoCaptureLabel = _CreateLabel(QTStr(PRESET_SOURCETYPE_VIDEOCAPTURE), this);
    videoCaptureLabel->setGeometry(404, 204, 168, 112);

    // Set Movable Property
    alertLabel->setProperty("isMovable", true);
    chattingLabel->setProperty("isMovable", true);
    videoCaptureLabel->setProperty("isMovable", true);
    /******************************************************/

    /***************** Non-Movable Source *****************/
    // Direct Broad
    QLabel* directBroadLabel = _CreateLabel(QTStr(PRESET_SOURCETYPE_DIRECTBROAD), this);
    directBroadLabel->setGeometry(243, 115, 100, 100);
    /******************************************************/

    // Add To Source List
    directBroadLabel->setProperty("sourceType", PRESET_SOURCETYPE_DIRECTBROAD);
    alertLabel->setProperty("sourceType", PRESET_SOURCETYPE_ALERT);
    chattingLabel->setProperty("sourceType", PRESET_SOURCETYPE_CHATTING);
    videoCaptureLabel->setProperty("sourceType", PRESET_SOURCETYPE_VIDEOCAPTURE);

    m_sourceLabelList.clear();
    m_sourceLabelList.append(directBroadLabel);
    m_sourceLabelList.append(alertLabel);
    m_sourceLabelList.append(chattingLabel);
    m_sourceLabelList.append(videoCaptureLabel);

    // Set Style Property
    directBroadLabel->setProperty("labelType", "labelPresetFullScreenSource");
    alertLabel->setProperty("labelType", "labelPresetSource");
    chattingLabel->setProperty("labelType", "labelPresetSource");
    videoCaptureLabel->setProperty("labelType", "labelPresetSource");

    /*************** Preset Name, Img Label ***************/
    // Preset Label Img
    QLabel* presetImgLabel = _CreateLabel("", this);
    presetImgLabel->setGeometry(20, 285, 24, 24);
    presetImgLabel->setObjectName("label_PresetRelayBroadImg");

    // Preset Label Text
    QLabel* presetTextLabel = _CreateLabel(QTStr("RelayBroad"), this);
    presetTextLabel->move(50, 285);
    presetTextLabel->setObjectName("label_PresetTxt");
    /******************************************************/
}
