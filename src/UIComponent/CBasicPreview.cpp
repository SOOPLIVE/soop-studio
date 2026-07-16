#include "CBasicPreview.h"

#include <QMouseEvent>
#include <QShortcut>

#include "display-helpers.hpp"

#include "Application/CApplication.h"
#include "MainFrame/SceneSource/CMainSceneSource.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"

#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Graphics/CGraphicsContext.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Source/CSource.h"

#include "ViewModel/MainWindow/CMainWindowRenderModel.h"

// 
#include "Blocks/SceneSourceDock/CSourceListView.h"

#include "platform/platform.hpp"
/// 
/// </summary>


static inline QColor color_from_int(long long val)
{
    return QColor(val & 0xff, (val >> 8) & 0xff, (val >> 16) & 0xff,
        (val >> 24) & 0xff);
}

CBasicPreview::CBasicPreview(QWidget* parent, Qt::WindowFlags flags)
{
    ResetScrollingOffset();
    
	setMouseTracking(true);

    drawlinePreview.SetModelPreview(&previewModel);

	this->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(this, SIGNAL(customContextMenuRequested(QPoint)),
			this, SLOT(ShowCustomContextMenu(QPoint)));

	setFocusPolicy(Qt::StrongFocus);

    SetSourceBorderColor();
}

CBasicPreview::~CBasicPreview()
{

}

CBasicPreview* CBasicPreview::Get()
{
	return DYNAMIC_COMPOSIT->GetMainPreview();
}

void CBasicPreview::ShowCustomContextMenu(const QPoint& pos)
{
	AFQSourceListView* sourceListView = SCENE_CONTEXT.GetSourceListViewPtr();
	if (!sourceListView)
		return;

	MAIN_SCENESOURCE->CreateSourcePopupMenu(sourceListView->GetTopSelectedSourceItem(), true);
}

void CBasicPreview::keyPressEvent(QKeyEvent *event)
{
    if (IsFixedScaling()/*m_mouseState.GetStateFixedScaling()*/ == false ||
        event->isAutoRepeat())
    {
        AFQTDisplay::keyPressEvent(event);
        return;
    }

    switch (event->key()) 
    {
    case Qt::Key_Space:
        setCursor(Qt::OpenHandCursor);
        mouseState.SetStateScrollMode(true);
        scrollMode = true;
        break;
    }

    AFQTDisplay::keyPressEvent(event);
}

void CBasicPreview::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        AFQTDisplay::keyReleaseEvent(event);
        return;
    }

    switch (event->key()) 
    {
    case Qt::Key_Space:
        scrollMode = false;
        mouseState.SetStateScrollMode(false);
        setCursor(Qt::ArrowCursor);
        break;
    }

    AFQTDisplay::keyReleaseEvent(event);
}

void CBasicPreview::wheelEvent(QWheelEvent *event)
{
    if (/*mouseState.GetStateScrollMode()*/scrollMode == true &&
        /*mouseState.GetStateFixedScaling()*/IsFixedScaling() == true)
    {
        const int delta = event->angleDelta().y();
        if (delta != 0)
        {
            if (delta > 0)
                SetScalingLevel(scalingLevel + 1);
            else
                SetScalingLevel(scalingLevel - 1);
            
            emit qsignalDisplayResized();
        }
    }

    AFQTDisplay::wheelEvent(event);
}

static bool FindSelectedPainterSources(obs_scene_t*, obs_sceneitem_t* item, void* param)
{
    auto sources = reinterpret_cast<std::vector<OBSSource>*>(param);

    if(!item)
        return true;

    if(false == obs_sceneitem_selected(item)) {
        return true;
    }

    if(false == obs_sceneitem_visible(item)) {
        return true;
    }

    obs_source_t* source = obs_sceneitem_get_source(item);
    if(!source) {
        return true;
    }

    std::string id = obs_source_get_id(source);
    if(0 == id.compare("painter_source")) {
        sources->push_back(obs_sceneitem_get_source(item));
    }

    return true;
};

QCursor GetPainterCursor(int toolType, int r, int g, int b, int thickness, float pixelRatio, float previewScale)
{
    const qreal outline = 1.0;
    const qreal penSize = qMax<qreal>(thickness * previewScale, 2.0);

    auto makePenCursor = [&]() -> QCursor {
        int sizePx = (int)qCeil(penSize * pixelRatio + outline * pixelRatio + 2);
        if(sizePx % 2 == 1) sizePx++;

        QPixmap img(sizePx, sizePx);
        img.setDevicePixelRatio(pixelRatio);
        img.fill(Qt::transparent);

        QPainter painter(&img);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QPen pen(QColor(255, 255, 255, 255));
        pen.setWidthF(outline * pixelRatio);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);

        const qreal inset = (outline * pixelRatio) / 2.0;
        const qreal w = sizePx - outline * pixelRatio;
        const qreal h = sizePx - outline * pixelRatio;

        painter.setBrush(QColor(r, g, b, 255));
        painter.drawEllipse(QRectF(inset, inset, w, h));
        painter.end();

        return QCursor(img, sizePx / 2, sizePx / 2);
    };

    auto makeImageCursor = [&](const QString& filePath) -> QCursor {
        QPixmap img(filePath);
        if(img.isNull())
            return QCursor(Qt::ArrowCursor);

        img.setDevicePixelRatio(pixelRatio);

        const QPoint centerPx(img.width() / 2, img.height() / 2);

        const int hx = qRound(centerPx.x() * pixelRatio);
        const int hy = qRound(centerPx.y() * pixelRatio);

        return QCursor(img, hx, hy);
    };

    if(toolType == 0)
        return makePenCursor();

    std::string absPath;
    GetDataFilePath("assets", absPath);
    const QString previewDir = QString::fromUtf8(absPath.c_str()) + "/preview/";

    if(toolType == 1) {
        return makeImageCursor(previewDir + "painter_eraser_cursor.png");
    }

    if(toolType == 2 || toolType == 3) {
        return makeImageCursor(previewDir + "painter_diagram_cursor.png");
    }

    return QCursor(Qt::ArrowCursor);
}

void CBasicPreview::mousePressEvent(QMouseEvent* event)
{
    QPointF pos = event->position();

    if (scrollMode && IsFixedScaling() &&
        event->button() == Qt::LeftButton) 
    {
        setCursor(Qt::ClosedHandCursor);
        scrollingFrom.x = pos.x();
        scrollingFrom.y = pos.y();
        return;
    }
    
    if (event->button() == Qt::RightButton) 
    {
        scrollMode = false;
        setCursor(Qt::ArrowCursor);
    }
    
    if (locked) {
        AFQTDisplay::mousePressEvent(event);
        return;
    }

	int32_t previewX = GRAPHIC_CONTEXT.GetMainPreviewX();
    int32_t previewY = GRAPHIC_CONTEXT.GetMainPreviewY();
	float previewScale = GRAPHIC_CONTEXT.GetMainPreviewScale();
    
//    if (configManager.GetStates()->IsPreviewProgramMode())
//        previewY = 0;
    
    float pixelRatio = devicePixelRatioF();
    
	float x = pos.x() - previewX / pixelRatio;
	float y = pos.y() - previewY / pixelRatio;

	if (event->button() != Qt::LeftButton &&
		event->button() != Qt::RightButton)
		return;


	if (event->button() == Qt::LeftButton)
        mouseState.SetMouseDown();
    
    
    previewModel.ClearSelectedItems();

    
    Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
    bool altDown = (modifiers & Qt::AltModifier);
    bool shiftDown = (modifiers & Qt::ShiftModifier);
    bool ctrlDown = (modifiers & Qt::ControlModifier);
    
    if (altDown)
        mouseState.SetStateCropping(true);

    if (altDown || shiftDown || ctrlDown)
        previewModel.SetSelectedItems();
    
    
	vec2_set(&startPos, x, y);
    previewModel.GetStretchHandleData(startPos, false, mouseState, pixelRatio);

	vec2_divf(&startPos, &startPos, previewScale / pixelRatio);
    startPos.x = std::round(startPos.x);
    startPos.y = std::round(startPos.y);

	if (previewModel.SelectedAtPos(startPos))
        mouseState.SetMouseOverItems();
	else
        mouseState.ResetMouseOverItems();

	vec2_zero(&lastMoveOffset);

	mousePos = startPos;

    std::vector<OBSSource> painter_sources;
    obs_scene_enum_items(SCENE_CONTEXT.GetCurrentScene(), FindSelectedPainterSources, &painter_sources);

    if(1 == painter_sources.size()) {
        OBSSource painter_source = painter_sources.at(0);
        if(painter_source) {
            if(event->buttons() == Qt::LeftButton) {
                struct obs_mouse_event mouseEvent = {};
                mouseEvent.modifiers = INTERACT_MOUSE_LEFT;
                mouseEvent.x = startPos.x;
                mouseEvent.y = startPos.y;

                obs_source_send_mouse_click(painter_source, &mouseEvent, MOUSE_LEFT, false, 1);
            }
        }
    }
    
    wrapper = obs_scene_save_transform_states(SCENE_CONTEXT.GetCurrentScene(), true);
    changed = false;
}

void CBasicPreview::mouseReleaseEvent(QMouseEvent* event)
{
    if (scrollMode)
        setCursor(Qt::OpenHandCursor);

    if (locked) {
        AFQTDisplay::mouseReleaseEvent(event);
        return;
    }
    
	if (mouseState.IsMouseDown())
	{
        float pixelRatio = devicePixelRatioF();

        bool isPainterSeleceted = false;
        std::vector<OBSSource> painter_sources;
        obs_scene_enum_items(SCENE_CONTEXT.GetCurrentScene(), FindSelectedPainterSources, &painter_sources);
        if(1 == painter_sources.size()) {
            isPainterSeleceted = true;
        }

        if(1 == painter_sources.size()) {
            OBSSource painter_source = painter_sources.at(0);
            if(painter_source) {
                struct obs_mouse_event mouseEvent = {};
                mouseEvent.modifiers = INTERACT_MOUSE_LEFT;
                mouseEvent.x = mousePos.x;
                mouseEvent.y = mousePos.y;

                obs_source_send_mouse_click(painter_source, &mouseEvent, MOUSE_LEFT, true, 1);
            }
        }
        
		vec2 pos = GetMouseEventPos(event, pixelRatio);

		if (mouseState.IsMouseMoved() == false)
			ProcessClick(pos);
        
        if (mouseState.GetStateSelectionBox())
        {
            Qt::KeyboardModifiers modifiers =
                QGuiApplication::keyboardModifiers();

            bool altDown = modifiers & Qt::AltModifier;
            bool shiftDown = modifiers & Qt::ShiftModifier;
            bool ctrlDown = modifiers & Qt::ControlModifier;

            previewModel.EnumSelecedHoveredItems(altDown, shiftDown, ctrlDown);
        }

		previewModel.Reset();
		mouseState.ResetMouseDown();
		mouseState.ResetMouseMoved();
        mouseState.SetStateCropping(false);
        mouseState.SetStateSelectionBox(false);

		unsetCursor();
        previewModel.MakeLastHoveredItem(pos);
	}

	AFQTDisplay::mouseReleaseEvent(event);
    
    OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(SCENE_CONTEXT.GetCurrentScene(), true);
    auto undo_redo = [](const std::string& data) {
        OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
        OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(dat, "scene_uuid"));
        DYNAMIC_COMPOSIT->SetCurrentScene(source.Get(), true);

        obs_scene_load_transform_states(data.c_str());
    };

    if(wrapper && rwrapper)
    {
        std::string undo_data(obs_data_get_json(wrapper));
        std::string redo_data(obs_data_get_json(rwrapper));
        if(changed && undo_data.compare(redo_data) != 0) {
            UNDO_STACK.AddAction(QTStr("Undo.Transform").arg(obs_source_get_name(SCENE_CONTEXT.GetCurrentSceneSource())),
                                 undo_redo, undo_redo, undo_data, redo_data);
        }
    }

    wrapper = nullptr;
}

void CBasicPreview::mouseMoveEvent(QMouseEvent* event)
{
    changed = true;
    
	QPointF qtPos = event->position();
    
    if (CheckHoverAreaShowBlock(qtPos) == false)
    {
        enterHoverAreaShowBlock = false;
        if (signaledShowBlock == true)
            signaledShowBlock = false;
    }
    

	bool updateCursor = false;

	ItemHandle stretchHandle = mouseState.GetCurrStateHandle();

    float pixelRatio = devicePixelRatioF();
    
    if (scrollMode && event->buttons() == Qt::LeftButton) {
        scrollingOffset.x += pixelRatio * (qtPos.x() - scrollingFrom.x);
        scrollingOffset.y += pixelRatio * (qtPos.y() - scrollingFrom.y);
        scrollingFrom.x = qtPos.x();
        scrollingFrom.y = qtPos.y();
        emit qsignalDisplayResized();
        return;
    }

    bool isPainterSeleceted = false;
    OBSSource selectedPainterSource;
    std::vector<OBSSource> painter_sources;
    obs_scene_enum_items(SCENE_CONTEXT.GetCurrentScene(), FindSelectedPainterSources, &painter_sources);
    if(1 == painter_sources.size()) {
        selectedPainterSource = painter_sources.at(0);
        isPainterSeleceted = true;
    }

    vec2 pos = GetMouseEventPos(event, pixelRatio);
        
	if (mouseState.IsMouseDown())
	{
        if(isPainterSeleceted && !mouseState.IsMouseMoved())
            previewModel.ClearHoveredItems(mouseState.GetStateSelectionBox());

		if (mouseState.IsMouseMoved() == false &&
			mouseState.IsMouseOverItems() == false &&
            stretchHandle == ItemHandle::None)
        {
			ProcessClick(startPos);

			if (previewModel.SelectedAtPos(startPos))
				mouseState.SetMouseOverItems();
			else
				mouseState.ResetMouseOverItems();
		}

		pos.x = std::round(pos.x);
		pos.y = std::round(pos.y);


        Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
        bool IsShift = (modifiers & Qt::ShiftModifier);
        bool IsCntrl = (modifiers & Qt::ControlModifier);

		if (stretchHandle != ItemHandle::None)
		{
            if (previewModel.IsLocked())
                return;
            
            mouseState.SetStateSelectionBox(false);
            
//            OBSScene scene = main->GetCurrentScene();
//            obs_sceneitem_t *group =
//                obs_sceneitem_get_group(scene, stretchItem);
//            if (group) {
//                vec3 group_pos;
//                vec3_set(&group_pos, pos.x, pos.y, 0.0f);
//                vec3_transform(&group_pos, &group_pos,
//                           &invGroupTransform);
//                pos.x = group_pos.x;
//                pos.y = group_pos.y;
//            }
            //
            
            if (stretchHandle == ItemHandle::Rot) 
            {
                previewModel.RotateItem(pos, IsShift, IsCntrl);
                setCursor(Qt::ClosedHandCursor);
            } 
            else if (mouseState.GetStateCropping())
                previewModel.CropItem(pos, mouseState);
            else
                previewModel.StretchItem(pos, mouseState, IsShift, IsCntrl);
		}
		else if (mouseState.IsMouseOverItems())
		{
            if(!isPainterSeleceted) {
                if (cursor().shape() != Qt::SizeAllCursor)
                    setCursor(Qt::SizeAllCursor);
            }
            
            mouseState.SetStateSelectionBox(false);
			previewModel.MoveItems(pos, lastMoveOffset, startPos, IsCntrl);
		}
		else 
		{
            mouseState.SetStateSelectionBox(true);
            if (mouseState.IsMouseMoved() == false)
                previewModel.DoSelect(startPos);
                        
            OBSScene scene = SCENE_CONTEXT.GetCurrentScene();
            if (scene != nullptr)
            {
                if (cursor().shape() != Qt::CrossCursor)
                    setCursor(Qt::CrossCursor);
                
                previewModel.BoxItems(scene, startPos, pos);
            }
		}

		mouseState.SetMouseMoved();
		mousePos = pos;

        if(isPainterSeleceted && selectedPainterSource) {
            struct obs_mouse_event mouseEvent = {};
            mouseEvent.modifiers = INTERACT_MOUSE_LEFT;
            mouseEvent.x = mousePos.x;
            mouseEvent.y = mousePos.y;

            obs_source_send_mouse_move(selectedPainterSource, &mouseEvent, false);
        }
	}
	else 
	{
		uint32_t cntHover = previewModel.MakeHoveredItem(pos);
        if(mouseState.IsMouseMoved() == false && cntHover > 0)
		{
			mousePos = pos;

			int32_t previewX = GRAPHIC_CONTEXT.GetMainPreviewX();
			int32_t previewY = GRAPHIC_CONTEXT.GetMainPreviewY();
            
//            if (CEFMANAGER.GetStates()->JustCheckPreviewProgramMode())
//                previewY = 0;
            

			float scale = pixelRatio;
			float x = qtPos.x() - previewX / scale;
			float y = qtPos.y() - previewY / scale;
			vec2_set(&startPos, x, y);

			updateCursor = true;
		}
        
        
        if (CheckHoverAreaShowBlock(qtPos))
        {
            if (enterHoverAreaShowBlock == false &&
                signaledShowBlock == false)
            {
                signaledShowBlock = true;
                
                if (STATEAPP.JustCheckPreviewProgramMode() == false)
                    emit DYNAMIC_COMPOSIT->BlockShowRequested();
            }
            
            enterHoverAreaShowBlock = true;
        }        
	}

    if(isPainterSeleceted && selectedPainterSource) {

        float previewScale = GRAPHIC_CONTEXT.GetMainPreviewScale();
        float pixelRatio = devicePixelRatioF();

        OBSDataAutoRelease settings = obs_source_get_settings(selectedPainterSource);
        int r = obs_data_get_int(settings, "line_color_r");
        int g = obs_data_get_int(settings, "line_color_g");
        int b = obs_data_get_int(settings, "line_color_b");
        int thickness = obs_data_get_int(settings, "thickness");
        int toolType = obs_data_get_int(settings, "tool");

        QString paintInfo = QString("paintInfo|%1|%2|%3|%4|%5|%6|%7|")
            .arg(previewScale).arg(pixelRatio)
            .arg(r).arg(g).arg(b).arg(thickness).arg(toolType);

        if(0 != cachedpaintSourceInfo.compare(paintInfo)) {
            cachedPaintSourceCursor = GetPainterCursor(toolType, r, g, b, thickness, pixelRatio, previewScale);
            cachedpaintSourceInfo = paintInfo;
        }

        setCursor(cachedPaintSourceCursor);
        updateCursor = false;
    }

	if (updateCursor)
	{   
		previewModel.GetStretchHandleData(startPos, true, mouseState, pixelRatio);
		uint32_t stretchFlags = (uint32_t)stretchHandle;
		UpdateCursor(stretchFlags);
	}
}
void CBasicPreview::leaveEvent(QEvent* event)
{
    enterHoverAreaShowBlock = false;
    if (signaledShowBlock == true)
        signaledShowBlock = false;
    
    previewModel.ClearHoveredItems(mouseState.GetStateSelectionBox());
}

void CBasicPreview::DrawOverflow()
{
    if (locked)
        return;

    drawlinePreview.DrawOverflow();
}

void CBasicPreview::DrawSceneEditing(float dpiValue/* = 1.f*/)
{
    if (locked)
        return;

    drawlinePreview.DrawSceneEditing(startPos, mousePos,
                                   mouseState.GetStateSelectionBox(), dpiValue);
}

void CBasicPreview::DrawSpacingHelpers(float dpiValue/* = 1.f*/)
{
    if (locked)
        return;

    drawlinePreview.DrawSpacingHelpers(dpiValue);
}

void CBasicPreview::SetSourceBorderColor()
{
    QColor selectColor = color_from_int(config_get_int(USERCONFIG, "Accessibility", "SelectRed"));
    QColor cropColor = color_from_int(config_get_int(USERCONFIG, "Accessibility", "SelectGreen"));
    QColor hoverColor = color_from_int(config_get_int(USERCONFIG, "Accessibility", "SelectBlue"));

    drawlinePreview.SetSelectColor(selectColor);
    drawlinePreview.SetCropColor(cropColor);
    drawlinePreview.SetHoverColor(hoverColor);
}

void CBasicPreview::InitSetNugeEventAction()
{
    auto addNudge = [this](const QKeySequence &seq, MoveDir direction,
                       int distance) {
        QAction* nudge = new QAction(this);
        nudge->setShortcut(seq);
        nudge->setShortcutContext(Qt::WidgetShortcut);
        addAction(nudge);
        connect(nudge, &QAction::triggered,
            [this, distance, direction]() {
                Nudge(distance, direction);
            });
    };

    addNudge(Qt::Key_Up, MoveDir::Up, 1);
    addNudge(Qt::Key_Down, MoveDir::Down, 1);
    addNudge(Qt::Key_Left, MoveDir::Left, 1);
    addNudge(Qt::Key_Right, MoveDir::Right, 1);
    addNudge(Qt::SHIFT | Qt::Key_Up, MoveDir::Up, 10);
    addNudge(Qt::SHIFT | Qt::Key_Down, MoveDir::Down, 10);
    addNudge(Qt::SHIFT | Qt::Key_Left, MoveDir::Left, 10);
    addNudge(Qt::SHIFT | Qt::Key_Right, MoveDir::Right, 10);
}

void CBasicPreview::SetScalingLevel(int32_t newScalingLevelVal)
{
    newScalingLevelVal = std::clamp(newScalingLevelVal, -MAX_SCALING_LEVEL,
                    MAX_SCALING_LEVEL);
    float newScalingAmountVal =
        pow(ZOOM_SENSITIVITY, float(newScalingLevelVal));
    scalingLevel = newScalingLevelVal;
    SetScalingAmount(newScalingAmountVal);
}

void CBasicPreview::SetScalingAmount(float newScalingAmountVal)
{
    scrollingOffset.x *= newScalingAmountVal / scalingAmount;
    scrollingOffset.y *= newScalingAmountVal / scalingAmount;
    scalingAmount = newScalingAmountVal;
}

void CBasicPreview::ResetScrollingOffset()
{
    vec2_zero(&scrollingOffset);
}

void CBasicPreview::ClampScrollingOffsets()
{
    obs_video_info ovi;
    obs_get_video_info(&ovi);

    QSize targetSize = GetPixelSize(this);

    vec3 target, offset;
    vec3_set(&target, (float)targetSize.width(), (float)targetSize.height(),
         1.0f);

    vec3_set(&offset, (float)ovi.base_width, (float)ovi.base_height, 1.0f);
    vec3_mulf(&offset, &offset, scalingAmount);

    vec3_sub(&offset, &offset, &target);

    vec3_mulf(&offset, &offset, 0.5f);
    vec3_maxf(&offset, &offset, 0.0f);

    scrollingOffset.x = std::clamp(scrollingOffset.x, -offset.x, offset.x);
    scrollingOffset.y = std::clamp(scrollingOffset.y, -offset.y, offset.y);
}

void CBasicPreview::UpdateCursor(uint32_t& flags)
{
	if (previewModel.IsLocked())
	{
		unsetCursor();
		return;
	}

	if (!flags && (cursor().shape() != Qt::OpenHandCursor || !scrollMode) )
		unsetCursor();
	if (cursor().shape() != Qt::ArrowCursor)
		return;

	if ((flags & ITEM_LEFT && flags & ITEM_TOP) ||
		(flags & ITEM_RIGHT && flags & ITEM_BOTTOM))
		setCursor(Qt::SizeFDiagCursor);
	else if ((flags & ITEM_LEFT && flags & ITEM_BOTTOM) ||
		(flags & ITEM_RIGHT && flags & ITEM_TOP))
		setCursor(Qt::SizeBDiagCursor);
	else if (flags & ITEM_LEFT || flags & ITEM_RIGHT)
		setCursor(Qt::SizeHorCursor);
	else if (flags & ITEM_TOP || flags & ITEM_BOTTOM)
		setCursor(Qt::SizeVerCursor);
	else if (flags & ITEM_ROT)
		setCursor(Qt::OpenHandCursor);
}

vec2 CBasicPreview::GetMouseEventPos(QMouseEvent* event, float dpiValue/* = 1.f*/)
{
	int32_t previewX = GRAPHIC_CONTEXT.GetMainPreviewX();
    int32_t previewY = GRAPHIC_CONTEXT.GetMainPreviewY();
	float previewScale = GRAPHIC_CONTEXT.GetMainPreviewScale();

//    if (STATEAPP.JustCheckPreviewProgramMode())
//        previewY = 0;
    
	float pixelRatio = dpiValue;
	float scale = pixelRatio / previewScale;
	QPoint qtPos = event->pos();
	vec2 pos;
	vec2_set(&pos, (qtPos.x() - previewX / pixelRatio) * scale,
		(qtPos.y() - previewY / pixelRatio) * scale);

	return pos;
}

void CBasicPreview::ProcessClick(const vec2& pos)
{
	Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();

	if (modifiers & Qt::ControlModifier)
        previewModel.DoCtrlSelect(pos);
	else
        previewModel.DoSelect(pos);
}

void CBasicPreview::Nudge(int dist, MoveDir dir)
{
    if (mouseState.GetStateLocked())
        return;

    struct vec2 offset;
    vec2_set(&offset, 0.0f, 0.0f);

    switch (dir) {
    case MoveDir::Up:
        offset.y = (float)-dist;
        break;
    case MoveDir::Down:
        offset.y = (float)dist;
        break;
    case MoveDir::Left:
        offset.x = (float)-dist;
        break;
    case MoveDir::Right:
        offset.x = (float)dist;
        break;
    }

    if (!recent_nudge) {
        recent_nudge = true;
        OBSDataAutoRelease wrapper = obs_scene_save_transform_states(SCENE_CONTEXT.GetCurrentScene(), true);
        std::string undo_data(obs_data_get_json(wrapper));

        nudge_timer = new QTimer;
        QObject::connect(
            nudge_timer, &QTimer::timeout,
            [this, &recent_nudge = recent_nudge, undo_data]() {
            OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(SCENE_CONTEXT.GetCurrentScene(), true);
                std::string redo_data(obs_data_get_json(rwrapper));

                UNDO_STACK.AddAction(QTStr("Undo.Transform").
                                     arg(obs_source_get_name(SCENE_CONTEXT.GetCurrentSceneSource())),
                                     undo_redo, undo_redo, undo_data, redo_data);

                recent_nudge = false;
            });
        connect(nudge_timer, &QTimer::timeout, nudge_timer, &QTimer::deleteLater);
        nudge_timer->setSingleShot(true);
    }

    if (nudge_timer) {
        nudge_timer->stop();
        nudge_timer->start(1000);
    } else {
        blog(LOG_ERROR, "No nudge timer!");
    }

    obs_scene_enum_items(SCENE_CONTEXT.GetCurrentScene(), CModelPreview::NudgeCallBack, &offset);
}
