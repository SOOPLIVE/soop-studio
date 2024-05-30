#include "CBasicPreview.h"


#include <QMouseEvent>


#include "Application/CApplication.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Graphics/CGraphicsContext.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Source/CSource.h"


#include "ViewModel/MainWindow/CMainWindowAccesser.h"

// 
#include "Blocks/SceneSourceDock/CSourceListView.h"

/// 
/// </summary>

#include <QShortcut>

static inline QColor color_from_int(long long val)
{
    return QColor(val & 0xff, (val >> 8) & 0xff, (val >> 16) & 0xff,
        (val >> 24) & 0xff);
}

AFBasicPreview::AFBasicPreview(QWidget* parent, Qt::WindowFlags flags)
{
    ResetScrollingOffset();
    
	setMouseTracking(true);

	m_pInitedContextGraphics = &AFGraphicsContext::GetSingletonInstance();
	AFSceneContext* tmpSceneContext = &AFSceneContext::GetSingletonInstance();

	m_DrawPreview.SetUnsafeAccessContext(m_pInitedContextGraphics, tmpSceneContext);
	m_PreviewModel.SetUnsafeAccessContext(m_pInitedContextGraphics, tmpSceneContext);
    m_DrawPreview.SetModelPreview(&m_PreviewModel);


	this->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(this, SIGNAL(customContextMenuRequested(QPoint)),
			this, SLOT(qSlotShowContextMenu(QPoint)));

	setFocusPolicy(Qt::StrongFocus);

    SetSourceBorderColor();
}

AFBasicPreview::~AFBasicPreview()
{

}

AFBasicPreview* AFBasicPreview::Get()
{
	return App()->GetMainView()->GetMainWindow()->GetMainPreview();
}

void AFBasicPreview::qSlotShowContextMenu(const QPoint& pos)
{
	AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
	AFQSourceListView* sourceListView = sceneContext.GetSourceListViewPtr();
	if (!sourceListView)
		return;

	App()->GetMainView()->CreateSourcePopupMenu(sourceListView->GetTopSelectedSourceItem(), true);
}

void AFBasicPreview::keyPressEvent(QKeyEvent *event)
{
    if (IsFixedScaling()/*m_MouseState.GetStateFixedScaling()*/ == false ||
        event->isAutoRepeat())
    {
        AFQTDisplay::keyPressEvent(event);
        return;
    }

    switch (event->key()) 
    {
    case Qt::Key_Space:
        setCursor(Qt::OpenHandCursor);
        m_MouseState.SetStateScrollMode(true);
        m_isScrollMode = true; 
        break;
    }

    AFQTDisplay::keyPressEvent(event);
}

void AFBasicPreview::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        AFQTDisplay::keyReleaseEvent(event);
        return;
    }

    switch (event->key()) 
    {
    case Qt::Key_Space:
        m_isScrollMode = false;
        m_MouseState.SetStateScrollMode(false);
        setCursor(Qt::ArrowCursor);
        break;
    }

    AFQTDisplay::keyReleaseEvent(event);
}

void AFBasicPreview::wheelEvent(QWheelEvent *event)
{
    if (/*m_MouseState.GetStateScrollMode()*/m_isScrollMode == true &&
        /*m_MouseState.GetStateFixedScaling()*/IsFixedScaling() == true)
    {
        const int delta = event->angleDelta().y();
        if (delta != 0)
        {
            if (delta > 0)
                SetScalingLevel(m_scalingLevel + 1);
            else
                SetScalingLevel(m_scalingLevel - 1);
            
            emit qsignalDisplayResized();
        }
    }

    AFQTDisplay::wheelEvent(event);
}

void AFBasicPreview::mousePressEvent(QMouseEvent* event)
{
	auto& contextDisplay = AFGraphicsContext::GetSingletonInstance();
    //auto& configManager = AFConfigManager::GetSingletonInstance();

	QPointF pos = event->position();

    if (m_isScrollMode && IsFixedScaling() &&
        event->button() == Qt::LeftButton) 
    {
        setCursor(Qt::ClosedHandCursor);
        m_vec2scrollingFrom.x = pos.x();
        m_vec2scrollingFrom.y = pos.y();
        return;
    }
    
    if (event->button() == Qt::RightButton) 
    {
        m_isScrollMode = false;
        setCursor(Qt::ArrowCursor);
    }
    
    if (m_isLocked) {
        AFQTDisplay::mousePressEvent(event);
        return;
    }

	int32_t previewX = contextDisplay.GetMainPreviewX();
    int32_t previewY = contextDisplay.GetMainPreviewY();
	float previewScale = contextDisplay.GetMainPreviewScale();
    
//    if (configManager.GetStates()->IsPreviewProgramMode())
//        previewY = 0;
    
    auto* mainWindowViewModel = g_ViewModelsDynamic.UnSafeGetInstace();
    float pixelRatio = mainWindowViewModel->m_RenderModel.GetDPIValue();
    
	float x = pos.x() - previewX / pixelRatio;
	float y = pos.y() - previewY / pixelRatio;

    AFQTDisplay::mousePressEvent(event);

	if (event->button() != Qt::LeftButton &&
		event->button() != Qt::RightButton)
		return;


	if (event->button() == Qt::LeftButton)
		m_MouseState.SetMouseDown();
    
    
    m_PreviewModel.ClearSelectedItems();

    
    Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
    bool altDown = (modifiers & Qt::AltModifier);
    bool shiftDown = (modifiers & Qt::ShiftModifier);
    bool ctrlDown = (modifiers & Qt::ControlModifier);
    
    if (altDown)
        m_MouseState.SetStateCropping(true);

    if (altDown || shiftDown || ctrlDown)
        m_PreviewModel.SetSelectedItems();
    
    
	vec2_set(&m_vec2StartPos, x, y);
	m_PreviewModel.GetStretchHandleData(m_vec2StartPos, false, m_MouseState, pixelRatio);

	vec2_divf(&m_vec2StartPos, &m_vec2StartPos, previewScale / pixelRatio);
	m_vec2StartPos.x = std::round(m_vec2StartPos.x);
	m_vec2StartPos.y = std::round(m_vec2StartPos.y);

	if (m_PreviewModel.SelectedAtPos(m_vec2StartPos))
		m_MouseState.SetMouseOverItems();
	else
		m_MouseState.ResetMouseOverItems();

	vec2_zero(&m_vec2LastMoveOffset);

	m_vec2MousePos = m_vec2StartPos;
    
    m_wrapper = obs_scene_save_transform_states(AFSourceUtil::GetCurrentScene(), true);
    m_changed = false;
}

void AFBasicPreview::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_isScrollMode)
        setCursor(Qt::OpenHandCursor);

    if (m_isLocked) {
        AFQTDisplay::mouseReleaseEvent(event);
        return;
    }
    
	if (m_MouseState.IsMouseDown())
	{
        auto* mainWindowViewModel = g_ViewModelsDynamic.UnSafeGetInstace();
        float pixelRatio = mainWindowViewModel->m_RenderModel.GetDPIValue();
        
		vec2 pos = _GetMouseEventPos(event, pixelRatio);

		if (m_MouseState.IsMouseMoved() == false)
			_ProcessClick(pos);
        
        if (m_MouseState.GetStateSelectionBox())
        {
            Qt::KeyboardModifiers modifiers =
                QGuiApplication::keyboardModifiers();

            bool altDown = modifiers & Qt::AltModifier;
            bool shiftDown = modifiers & Qt::ShiftModifier;
            bool ctrlDown = modifiers & Qt::ControlModifier;

            m_PreviewModel.EnumSelecedHoveredItems(altDown, shiftDown, ctrlDown);
        }

		m_PreviewModel.Reset();
		m_MouseState.ResetMouseDown();
		m_MouseState.ResetMouseMoved();
        m_MouseState.SetStateCropping(false);
        m_MouseState.SetStateSelectionBox(false);

		unsetCursor();
        
        m_PreviewModel.MakeLastHoveredItem(pos);
	}

	AFQTDisplay::mouseReleaseEvent(event);
    
    AFMainFrame* main = App()->GetMainView();
    AFMainDynamicComposit* maincomposit = main->GetMainWindow();
    OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(AFSourceUtil::GetCurrentScene(), true);
    auto undo_redo = [maincomposit](const std::string& data) {
        OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
        OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(dat, "scene_uuid"));
        maincomposit->SetCurrentScene(source.Get(), true);

        obs_scene_load_transform_states(data.c_str());
    };

    if(m_wrapper && rwrapper)
    {
        std::string undo_data(obs_data_get_json(m_wrapper));
        std::string redo_data(obs_data_get_json(rwrapper));
        if(m_changed && undo_data.compare(redo_data) != 0) {
            main->m_undo_s.AddAction(QTStr("Undo.Transform").arg(obs_source_get_name(AFSourceUtil::GetCurrentSource())),
                                     undo_redo, undo_redo, undo_data, redo_data);
        }
    }

    m_wrapper = nullptr;
}

void AFBasicPreview::mouseMoveEvent(QMouseEvent* event)
{

    m_changed = true;
    
	QPointF qtPos = event->position();
    
    if (_CheckHoverAreaShowBlock(qtPos) == false)
    {
        m_IsEnterHoverAreaShowBlock = false;
        if (m_bSignaledShowBlock == true)
            m_bSignaledShowBlock = false;
    }
    

	bool updateCursor = false;

	ItemHandle stretchHandle = m_MouseState.GetCurrStateHandle();

    auto* mainWindowViewModel = g_ViewModelsDynamic.UnSafeGetInstace();
    float pixelRatio = mainWindowViewModel->m_RenderModel.GetDPIValue();
    
    if (m_isScrollMode && event->buttons() == Qt::LeftButton) {
        m_vec2ScrollingOffset.x += pixelRatio * (qtPos.x() - m_vec2scrollingFrom.x);
        m_vec2ScrollingOffset.y += pixelRatio * (qtPos.y() - m_vec2scrollingFrom.y);
        m_vec2scrollingFrom.x = qtPos.x();
        m_vec2scrollingFrom.y = qtPos.y();
        emit qsignalDisplayResized();
        return;
    }
    
    
	if (m_MouseState.IsMouseDown())
	{
		vec2 pos = _GetMouseEventPos(event, pixelRatio);

		if (m_MouseState.IsMouseMoved() == false &&
			m_MouseState.IsMouseOverItems() == false &&
            stretchHandle == ItemHandle::None)
        {
			_ProcessClick(m_vec2StartPos);

			if (m_PreviewModel.SelectedAtPos(m_vec2StartPos))
				m_MouseState.SetMouseOverItems();
			else
				m_MouseState.ResetMouseOverItems();
		}

		pos.x = std::round(pos.x);
		pos.y = std::round(pos.y);


        Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
        bool IsShift = (modifiers & Qt::ShiftModifier);
        bool IsCntrl = (modifiers & Qt::ControlModifier);

		if (stretchHandle != ItemHandle::None)
		{
            if (m_PreviewModel.IsLocked())
                return;
            
            m_MouseState.SetStateSelectionBox(false);
            
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
                m_PreviewModel.RotateItem(pos, IsShift, IsCntrl);
                setCursor(Qt::ClosedHandCursor);
            } 
            else if (m_MouseState.GetStateCropping())
                m_PreviewModel.CropItem(pos, m_MouseState);
            else
                m_PreviewModel.StretchItem(pos, m_MouseState, IsShift, IsCntrl);
		}
		else if (m_MouseState.IsMouseOverItems())
		{
            if (cursor().shape() != Qt::SizeAllCursor)
                setCursor(Qt::SizeAllCursor);
            
            m_MouseState.SetStateSelectionBox(false);
			m_PreviewModel.MoveItems(pos, m_vec2LastMoveOffset, m_vec2StartPos, IsCntrl);
		}
		else 
		{
            m_MouseState.SetStateSelectionBox(true);
            if (m_MouseState.IsMouseMoved() == false)
                m_PreviewModel.DoSelect(m_vec2StartPos);
            
            
            OBSScene scene = AFSceneContext::GetSingletonInstance().
                                GetCurrOBSScene();
            if (scene != nullptr)
            {
                if (cursor().shape() != Qt::CrossCursor)
                    setCursor(Qt::CrossCursor);
                
                m_PreviewModel.BoxItems(scene, m_vec2StartPos, pos);
            }
		}

		m_MouseState.SetMouseMoved();
		m_vec2MousePos = pos;
	}
	else 
	{
		vec2 pos = _GetMouseEventPos(event, pixelRatio);
		uint32_t cntHover = m_PreviewModel.MakeHoveredItem(pos);

		if (m_MouseState.IsMouseMoved() == false &&
            cntHover > 0)
		{
			m_vec2MousePos = pos;

            auto& contextDisplay = AFGraphicsContext::GetSingletonInstance();
//            auto& configManager = AFConfigManager::GetSingletonInstance();

			int32_t previewX = contextDisplay.GetMainPreviewX();
			int32_t previewY = contextDisplay.GetMainPreviewY();
            
//            if (configManager.GetStates()->JustCheckPreviewProgramMode())
//                previewY = 0;
            

			float scale = pixelRatio;
			float x = qtPos.x() - previewX / scale;
			float y = qtPos.y() - previewY / scale;
			vec2_set(&m_vec2StartPos, x, y);

			updateCursor = true;
		}
        
        
        if (_CheckHoverAreaShowBlock(qtPos))
        {
            if (m_IsEnterHoverAreaShowBlock == false &&
                m_bSignaledShowBlock == false)
            {
                m_bSignaledShowBlock = true;
                
                auto& configManager = AFConfigManager::GetSingletonInstance();
                
                if (configManager.GetStates()->JustCheckPreviewProgramMode() == false)
                    emit App()->GetMainView()->GetMainWindow()->qsignalShowBlock();
            }
            
            m_IsEnterHoverAreaShowBlock = true;
        }
        
	}

	if (updateCursor)
	{
        auto* mainWindowViewModel = g_ViewModelsDynamic.UnSafeGetInstace();
        float pixelRatio = mainWindowViewModel->m_RenderModel.GetDPIValue();
        
		m_PreviewModel.GetStretchHandleData(m_vec2StartPos, true, m_MouseState, pixelRatio);
		uint32_t stretchFlags = (uint32_t)stretchHandle;
		UpdateCursor(stretchFlags);
	}
}
void AFBasicPreview::leaveEvent(QEvent* event)
{
    m_IsEnterHoverAreaShowBlock = false;
    if (m_bSignaledShowBlock == true)
        m_bSignaledShowBlock = false;
    
    m_PreviewModel.ClearHoveredItems(m_MouseState.GetStateSelectionBox());
}

void AFBasicPreview::DrawOverflow()
{
    if (m_isLocked)
        return;

	m_DrawPreview.DrawOverflow();
}

void AFBasicPreview::DrawSceneEditing(float dpiValue/* = 1.f*/)
{
    if (m_isLocked)
        return;

	m_DrawPreview.DrawSceneEditing(m_vec2StartPos, m_vec2MousePos, 
                                   m_MouseState.GetStateSelectionBox(), dpiValue);
}

void AFBasicPreview::DrawSpacingHelpers(float dpiValue/* = 1.f*/)
{
    if (m_isLocked)
        return;

	m_DrawPreview.DrawSpacingHelpers(dpiValue);
}

void AFBasicPreview::SetSourceBorderColor()
{
    config_t* globalConfig = AFConfigManager::GetSingletonInstance().GetGlobal();

    QColor selectColor = color_from_int(config_get_int(globalConfig, "Accessibility", "SelectRed"));
    QColor cropColor = color_from_int(config_get_int(globalConfig, "Accessibility", "SelectGreen"));
    QColor hoverColor = color_from_int(config_get_int(globalConfig, "Accessibility", "SelectBlue"));

    m_DrawPreview.SetSelectColor(selectColor);
    m_DrawPreview.SetCropColor(cropColor);
    m_DrawPreview.SetHoverColor(hoverColor);
}

void AFBasicPreview::RegisterShortCutAction(QAction* actionShortcut)
{
	if (actionShortcut)
		addAction(actionShortcut);
}

void AFBasicPreview::InitSetNugeEventAction()
{
    auto addNudge = [this](const QKeySequence &seq, MoveDir direction,
                       int distance) {
        QAction* nudge = new QAction(this);
        nudge->setShortcut(seq);
        nudge->setShortcutContext(Qt::WidgetShortcut);
        addAction(nudge);
        connect(nudge, &QAction::triggered,
            [this, distance, direction]() {
                _Nudge(distance, direction);
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

void AFBasicPreview::SetScalingLevel(int32_t newScalingLevelVal)
{
    newScalingLevelVal = std::clamp(newScalingLevelVal, -MAX_SCALING_LEVEL,
                    MAX_SCALING_LEVEL);
    float newScalingAmountVal =
        pow(ZOOM_SENSITIVITY, float(newScalingLevelVal));
    m_scalingLevel = newScalingLevelVal;
    SetScalingAmount(newScalingAmountVal);
}

void AFBasicPreview::SetScalingAmount(float newScalingAmountVal)
{
    m_vec2ScrollingOffset.x *= newScalingAmountVal / m_scalingAmount;
    m_vec2ScrollingOffset.y *= newScalingAmountVal / m_scalingAmount;
    m_scalingAmount = newScalingAmountVal;
}

void AFBasicPreview::ResetScrollingOffset()
{
    vec2_zero(&m_vec2ScrollingOffset);
}

void AFBasicPreview::ClampScrollingOffsets()
{
    obs_video_info ovi;
    obs_get_video_info(&ovi);

    QSize targetSize = GetPixelSize(this);

    vec3 target, offset;
    vec3_set(&target, (float)targetSize.width(), (float)targetSize.height(),
         1.0f);

    vec3_set(&offset, (float)ovi.base_width, (float)ovi.base_height, 1.0f);
    vec3_mulf(&offset, &offset, m_scalingAmount);

    vec3_sub(&offset, &offset, &target);

    vec3_mulf(&offset, &offset, 0.5f);
    vec3_maxf(&offset, &offset, 0.0f);

    m_vec2ScrollingOffset.x = std::clamp(m_vec2ScrollingOffset.x, -offset.x, offset.x);
    m_vec2ScrollingOffset.y = std::clamp(m_vec2ScrollingOffset.y, -offset.y, offset.y);
}

void AFBasicPreview::UpdateCursor(uint32_t& flags)
{
	if (m_PreviewModel.IsLocked())
	{
		unsetCursor();
		return;
	}

	if (!flags && (cursor().shape() != Qt::OpenHandCursor || !m_isScrollMode) )
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

vec2 AFBasicPreview::_GetMouseEventPos(QMouseEvent* event, float dpiValue/* = 1.f*/)
{
    auto& contextDisplay = AFGraphicsContext::GetSingletonInstance();
    //auto& configManager = AFConfigManager::GetSingletonInstance();

	int32_t previewX = contextDisplay.GetMainPreviewX();
    int32_t previewY = contextDisplay.GetMainPreviewY();
	float previewScale = contextDisplay.GetMainPreviewScale();

//    if (configManager.GetStates()->JustCheckPreviewProgramMode())
//        previewY = 0;
    
	float pixelRatio = dpiValue;
	float scale = pixelRatio / previewScale;
	QPoint qtPos = event->pos();
	vec2 pos;
	vec2_set(&pos, (qtPos.x() - previewX / pixelRatio) * scale,
		(qtPos.y() - previewY / pixelRatio) * scale);

	return pos;
}

void AFBasicPreview::_ProcessClick(const vec2& pos)
{
	Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();

	if (modifiers & Qt::ControlModifier)
        m_PreviewModel.DoCtrlSelect(pos);
	else
        m_PreviewModel.DoSelect(pos);
}

void AFBasicPreview::_Nudge(int dist, MoveDir dir)
{
    if (m_MouseState.GetStateLocked())
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
        OBSDataAutoRelease wrapper = obs_scene_save_transform_states(AFSourceUtil::GetCurrentScene(), true);
        std::string undo_data(obs_data_get_json(wrapper));

        nudge_timer = new QTimer;
        QObject::connect(
            nudge_timer, &QTimer::timeout,
            [this, &recent_nudge = recent_nudge, undo_data]() {
            OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(AFSourceUtil::GetCurrentScene(), true);
                std::string redo_data(obs_data_get_json(rwrapper));

                AFMainFrame* main = App()->GetMainView();
                main->m_undo_s.AddAction(QTStr("Undo.Transform").
                                         arg(obs_source_get_name(AFSourceUtil::GetCurrentSource())),
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

    auto& tmpSceneContext = AFSceneContext::GetSingletonInstance();
    obs_scene_enum_items(tmpSceneContext.GetCurrOBSScene(), AFModelPreview::NudgeCallBack, &offset);
}
