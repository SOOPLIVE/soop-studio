#include "CMainSceneSource.h"

#include <fstream>
#include <sstream>

#include "ui_aneta-main-frame.h"

#include "qt-wrappers.hpp"

#include "Common/StudioDefine.h"

#include "Utils/ScreenshotObj.h"

#pragma region _SOOP_BREAKTIME
#include "Utils/BreaktimeManager.h"
#pragma endregion

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Auth/SBroadInfo.h"
#include "CoreModel/Video/CVideo.h"
#include "CoreModel/Audio/CAudio.h"
#include "CoreModel/OBSData/CLoadSaveManager.h"
#include "CoreModel/Icon/CIconContext.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/SOOPSource/CSoopMediaSourceManager.h"

#include "UIComponent/CBasicPreview.h"
#include "UIComponent/CColorSelect.h"
#include "UIComponent/CNameDialog.h"

#include "PopupWindows/CCustomColorDialog.h"
#include "PopupWindows/SourceDialog/CSelectSourceDialog.h"
#include "PopupWindows/SourceDialog/CSelectSourceButton.h"

#include "Blocks/SceneSourceDock/CSourceListView.h"

#include "MainFrame/Output/COutput.h"

CMainSceneSource::CMainSceneSource(QObject* parent) :
    QObject(parent)
{

}

CMainSceneSource::~CMainSceneSource()
{
	if (m_screenshotData) delete m_screenshotData;

    if (m_widgetActionColor) delete m_widgetActionColor;
    if (m_widgetColorSelect) delete m_widgetColorSelect;
    if (m_menuScaleFiltering) delete m_menuScaleFiltering;
    if (m_menuBlendingMode) delete m_menuBlendingMode;
    if (m_menuBlendingMethodMode) delete m_menuBlendingMethodMode;
    if (m_menuDeinterlace) delete m_menuDeinterlace;
}

void CMainSceneSource::qslotShowSelectSourcePopup()
{
    AFQSelectSourceDialog sourceSelect(nullptr);
    setCenterPositionNotUseParent(&sourceSelect, MAINFRAME);

    if (MAINFRAME->IsSmallResolution())
    {
        sourceSelect.setMinimumHeight(550);
        sourceSelect.resize(sourceSelect.width(), 550);
    }

    int sourceX = MAINFRAME->x() + (MAINFRAME->width() / 2) - (sourceSelect.width() / 2);
    int sourceY = MAINFRAME->y() + (MAINFRAME->height() / 2) - (sourceSelect.height() / 2);

    QRect adjust;
    QRect originRect = QRect(sourceX, sourceY, sourceSelect.width(), sourceSelect.height());
    MAIN_BLOCKMANAGER->AdjustPositionOutSideFullScreen(originRect, adjust);
    sourceSelect.setGeometry(adjust);

    if (sourceSelect.exec() != QDialog::DialogCode::Accepted)
        return;

    AddNewSource(sourceSelect.m_sourceId);
}

void CMainSceneSource::qslotAddSource(QString sourceID)
{
    AddNewSource(sourceID);
}

void CMainSceneSource::qslotActionCopySource()
{
    m_clipboard.clear();

    AFQSourceListView* sourceListView = SCENE_CONTEXT.GetSourceListViewPtr();

    for (auto& selectedSource : sourceListView->selectionModel()->selectedIndexes()) {
        OBSSceneItem item = sourceListView->Get(selectedSource.row());
        if (!item)
            continue;

        OBSSource source = obs_sceneitem_get_source(item);


        std::string id = obs_source_get_id(source);
        if (AFSourceUtil::IsSoopKBOSource(source) ||
            AFSourceUtil::IsSoopFootballSource(source) ||
            AFSourceUtil::IsSoopMediaSource(source) ||
            0 == id.compare("soop_videoballoon_source")) {
            continue;
        }


        SourceCopyInfo copyInfo;
        copyInfo.weak_source = OBSGetWeakRef(source);
        obs_sceneitem_get_info(item, &copyInfo.transform);
        obs_sceneitem_get_crop(item, &copyInfo.crop);
        copyInfo.blend_method = obs_sceneitem_get_blending_method(item);
        copyInfo.blend_mode = obs_sceneitem_get_blending_mode(item);
        copyInfo.visible = obs_sceneitem_visible(item);
        copyInfo.can_duplicate = !(obs_source_get_output_flags(source) & OBS_SOURCE_DO_NOT_DUPLICATE);
        
        if (AFSourceUtil::IsForceDuplicateSource(id.c_str()))
            copyInfo.can_duplicate = true;

        m_clipboard.push_back(copyInfo);
    }

    UpdateEditMenu();
}

void CMainSceneSource::qslotActionPasteSource()
{
    OBSScene scene = SCENE_CONTEXT.GetCurrentScene();
    OBSSource scene_source = OBSSource(obs_scene_get_source(scene));

    OBSData undo_data = MAINFRAME->BackupScene(scene_source);
    UNDO_STACK.PushDisabled();

    for (size_t i = m_clipboard.size(); i > 0; i--) {
        SourceCopyInfo& copyInfo = m_clipboard[i - 1];

        OBSSource source = OBSGetStrongRef(copyInfo.weak_source);
        if (!source)
            continue;

        bool dup = copyInfo.can_duplicate;
        const char* name = obs_source_get_name(source);

        if (!!obs_scene_get_group(scene, name)) {
            dup = true;
        }

        AFSourceUtil::SourcePaste(copyInfo, dup);
    }

    UNDO_STACK.PopDisabled();

    QString action_name = QTStr("Undo.PasteSource");
    const char* scene_name = obs_source_get_name(scene_source);

    OBSData redo_data = MAINFRAME->BackupScene(scene_source);
    MAINFRAME->CreateSceneUndoRedoAction(action_name.arg(scene_name), undo_data, redo_data);
}

void CMainSceneSource::qslotActionPasteRefSource()
{
    OBSScene scene = SCENE_CONTEXT.GetCurrentScene();
    OBSSource scene_source = OBSSource(obs_scene_get_source(scene));

    OBSData undo_data = MAINFRAME->BackupScene(scene_source);
    UNDO_STACK.PushDisabled();

    for (size_t i = m_clipboard.size(); i > 0; i--) {
        SourceCopyInfo& copyInfo = m_clipboard[i - 1];

        OBSSource source = OBSGetStrongRef(copyInfo.weak_source);
        if (!source)
            continue;

        const char* name = obs_source_get_name(source);

        /* do not allow duplicate refs of the same group in the same
         * scene */
        if (!!obs_scene_get_group(scene, name)) {
            continue;
        }

        AFSourceUtil::SourcePaste(copyInfo, false);
    }

    UNDO_STACK.PopDisabled();

    QString action_name = QTStr("Undo.PasteSourceRef");
    const char* scene_name = obs_source_get_name(scene_source);

    OBSData redo_data = MAINFRAME->BackupScene(scene_source);
    MAINFRAME->CreateSceneUndoRedoAction(action_name.arg(scene_name), undo_data, redo_data);
}

void CMainSceneSource::qslotActionRenameSource()
{
    AFQSourceListView* sourceListView = SCENE_CONTEXT.GetSourceListViewPtr();
    if (!sourceListView)
        return;

    int idx = sourceListView->GetTopSelectedSourceItem();
    sourceListView->Edit(idx);
}

void CMainSceneSource::qslotActionRemoveSource()
{
    AFSourceUtil::RemoveSourceItems(SCENE_CONTEXT.GetCurrentScene());
}

void CMainSceneSource::qslotActionPasteDupSource()
{
    OBSScene scene = SCENE_CONTEXT.GetCurrentScene();
    OBSSource scene_source = OBSSource(obs_scene_get_source(scene));

    OBSData undo_data = MAINFRAME->BackupScene(scene_source);
    UNDO_STACK.PushDisabled();


    for (size_t i = m_clipboard.size(); i > 0; i--) {
        SourceCopyInfo& copyInfo = m_clipboard[i - 1];
        AFSourceUtil::SourcePaste(copyInfo, true);
    }

    UNDO_STACK.PopDisabled();

    QString action_name = QTStr("Undo.PasteSource");
    const char* scene_name = obs_source_get_name(scene_source);

    OBSData redo_data = MAINFRAME->BackupScene(scene_source);
    MAINFRAME->CreateSceneUndoRedoAction(action_name.arg(scene_name), undo_data, redo_data);
}


void CMainSceneSource::qslotActionCopyTransform()
{
    const auto item = SCENE_CONTEXT.GetCurrentOBSSceneItem();
    if (!item)
        return;

    obs_sceneitem_get_info(item, &SCENE_CONTEXT.GetTransformInfo());
    obs_sceneitem_get_crop(item, &SCENE_CONTEXT.GetCropInfo());

    MAINFRAME_UI->action_PasteTransform->setEnabled(true);
    m_hasCopiedTransform = true;
}

void CMainSceneSource::qslotActionPasteTransform()
{
    OBSScene curScene = SCENE_CONTEXT.GetCurrentScene();

    OBSDataAutoRelease wrapper = obs_scene_save_transform_states(curScene, false);
    auto func = [](obs_scene_t*, obs_sceneitem_t* item, void* data) {
        if (!obs_sceneitem_selected(item))
            return true;

        obs_sceneitem_defer_update_begin(item);
        obs_sceneitem_set_info(item, &SCENE_CONTEXT.GetTransformInfo());
        obs_sceneitem_set_crop(item, &SCENE_CONTEXT.GetCropInfo());
        obs_sceneitem_defer_update_end(item);

        return true;
    };

    obs_scene_enum_items(curScene, func, this);

    OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(curScene, false);
    std::string undo_data(obs_data_get_json(wrapper));
    std::string redo_data(obs_data_get_json(rwrapper));
    OBSSource curSource = SCENE_CONTEXT.GetCurrentSceneSource();
    UNDO_STACK.AddAction(QTStr("Undo.Transform.Paste").arg(obs_source_get_name(curSource)),
                         undo_redo, undo_redo, undo_data, redo_data);
}

void CMainSceneSource::qslotActionResetTransform()
{
    AFSourceUtil::ResetTransform();
}

void CMainSceneSource::qslotActionRotate90CW()
{
    AFSourceUtil::RotateSourceFromMenu(90.0f);
}

void CMainSceneSource::qslotActionRotate90CCW()
{
    AFSourceUtil::RotateSourceFromMenu(-90.0f);
}

void CMainSceneSource::qslotActionRotate180()
{
    AFSourceUtil::RotateSourceFromMenu(180.0f);
}

void CMainSceneSource::qslotFlipHorizontal()
{
    AFSourceUtil::FlipSourceFromMenu(-1.0f, 1.0f);
}

void CMainSceneSource::qslotFlipVertical()
{
    AFSourceUtil::FlipSourceFromMenu(1.0f, -1.0f);
}

void CMainSceneSource::qslotFitToScreen()
{
    AFSourceUtil::FitSourceToScreenFromMenu(OBS_BOUNDS_SCALE_INNER);
}

void CMainSceneSource::qslotStretchToScreen()
{
    AFSourceUtil::FitSourceToScreenFromMenu(OBS_BOUNDS_STRETCH);
}

void CMainSceneSource::qslotCenterToScreen()
{
    AFSourceUtil::SetCenterToScreenFromMenu(CenterType::Scene);
}

void CMainSceneSource::qslotVerticalCenter()
{
    AFSourceUtil::SetCenterToScreenFromMenu(CenterType::Vertical);
}

void CMainSceneSource::qslotHorizontalCenter()
{
    AFSourceUtil::SetCenterToScreenFromMenu(CenterType::Horizontal);
}

void CMainSceneSource::qslotActionEditTransform()
{
    const auto item = SCENE_CONTEXT.GetCurrentOBSSceneItem();
    if (!item)
        return;

    const char* id = obs_source_get_id(obs_sceneitem_get_source(item));
    if(0 == strcmp("painter_source", id))
        return;

    MAINFRAME->CreateEditTransformPopup(item);
}

void CMainSceneSource::qslotActionShowInteractionPopup()
{
    const auto item = SCENE_CONTEXT.GetCurrentOBSSceneItem();
    if (!item)
        return;

    OBSSource source = obs_sceneitem_get_source(item);

    if (source)
        MAINFRAME->ShowBrowserInteractionPopup(source);
}

void CMainSceneSource::qslotActionShowProperties()
{
    const auto item = SCENE_CONTEXT.GetCurrentOBSSceneItem();
    if (!item)
        return;

    obs_source_t* source = obs_sceneitem_get_source(item);
    if (obs_source_configurable(source)) {
        MAINFRAME->CreateSourceProperties(source, true);
    }
}

void CMainSceneSource::qslotOpenSourceFilters()
{
    OBSSceneItem item = SCENE_CONTEXT.GetCurrentOBSSceneItem();
    OBSSource source = obs_sceneitem_get_source(item);

    MAINFRAME->CreateFiltersWindow(source);
}

void CMainSceneSource::qslotCopySourceFilters()
{
    OBSSceneItem item = SCENE_CONTEXT.GetCurrentOBSSceneItem();
    if (!item)
        return;

    OBSSource source = obs_sceneitem_get_source(item);

    SCENE_CONTEXT.m_obsCopyFiltersSource = obs_source_get_weak_source(source);

    MAINFRAME_UI->action_PasteFilters->setEnabled(true);
}

void CMainSceneSource::qslotPasteSourceFilters()
{
    OBSSourceAutoRelease source =
        obs_weak_source_get_source(SCENE_CONTEXT.m_obsCopyFiltersSource);

    OBSSceneItem sceneItem = SCENE_CONTEXT.GetCurrentOBSSceneItem();
    if (!sceneItem)
        return;

    OBSSource dstSource = obs_sceneitem_get_source(sceneItem);

    if (source == dstSource)
        return;

    OBSDataArrayAutoRelease undo_array = obs_source_backup_filters(dstSource);
    obs_source_copy_filters(dstSource, source);
    OBSDataArrayAutoRelease redo_array = obs_source_backup_filters(dstSource);

    const char* srcName = obs_source_get_name(source);
    const char* dstName = obs_source_get_name(dstSource);
    QString text = Str("Undo.Filters.Paste.Multiple");
    text = text.arg(srcName, dstName);

    MAINFRAME->CreateFilterPasteUndoRedoAction(text, dstSource, undo_array, redo_array);
}

void CMainSceneSource::qslotSetSplitEffectFilter()
{
    OBSSceneItem item = SCENE_CONTEXT.GetCurrentOBSSceneItem();
    OBSSource source = obs_sceneitem_get_source(item);
    
    MAINFRAME->CreateSplitEffectPopup(source);
}

void CMainSceneSource::qslotSetScaleFilter()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    obs_scale_type mode = (obs_scale_type)action->property("mode").toInt();
    OBSSceneItem sceneItem = SCENE_CONTEXT.GetCurrentOBSSceneItem();

    obs_sceneitem_set_scale_filter(sceneItem, mode);
}

void CMainSceneSource::qslotBlendingMethod()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    obs_blending_method method =
        (obs_blending_method)action->property("method").toInt();
    OBSSceneItem sceneItem = SCENE_CONTEXT.GetCurrentOBSSceneItem();

    obs_sceneitem_set_blending_method(sceneItem, method);
}

void CMainSceneSource::qslotBlendingMode()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    obs_blending_type mode =
        (obs_blending_type)action->property("mode").toInt();
    OBSSceneItem sceneItem = SCENE_CONTEXT.GetCurrentOBSSceneItem();

    obs_sceneitem_set_blending_mode(sceneItem, mode);
}

void CMainSceneSource::qslotSetDeinterlaceingMode()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    obs_deinterlace_mode mode =
        (obs_deinterlace_mode)action->property("mode").toInt();
    OBSSceneItem sceneItem = SCENE_CONTEXT.GetCurrentOBSSceneItem();
    obs_source_t* source = obs_sceneitem_get_source(sceneItem);

    obs_source_set_deinterlace_mode(source, mode);
}

void CMainSceneSource::qslotSetDeinterlacingOrder()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    obs_deinterlace_field_order order =
        (obs_deinterlace_field_order)action->property("order").toInt();
    OBSSceneItem sceneItem = SCENE_CONTEXT.GetCurrentOBSSceneItem();
    obs_source_t* source = obs_sceneitem_get_source(sceneItem);

    obs_source_set_deinterlace_field_order(source, order);
}

static void ConfirmColor(AFQSourceListView* sourceList, const QColor& color,
    QModelIndexList selectedItems)
{
    for (int x = 0; x < selectedItems.count(); x++) {
        AFQSourceViewItem* sourceItem =
            sourceList->GetItemWidget(selectedItems[x].row());

        sourceItem->SetBackgroundColor(color);

        OBSSceneItem sceneItem = sourceList->Get(selectedItems[x].row());
        OBSDataAutoRelease privData =
            obs_sceneitem_get_private_settings(sceneItem);
        obs_data_set_int(privData, "color-preset", 1);
        obs_data_set_string(privData, "color",
            QT_TO_UTF8(color.name(QColor::HexArgb)));
    }
}

void CMainSceneSource::qslotSourceListItemColorChange()
{
    AFQSourceListView* sourceList = SCENE_CONTEXT.GetSourceListViewPtr();
    QModelIndexList selectedItems =
        sourceList->selectionModel()->selectedIndexes();

    QAction* action = qobject_cast<QAction*>(sender());
    QPushButton* colorButton = qobject_cast<QPushButton*>(sender());

    if (selectedItems.count() == 0)
        return;

    if (colorButton) {
        QFrame* colorButtonFrame = qobject_cast<QFrame*>(colorButton->parentWidget());

        int preset = colorButton->property("bgColor").value<int>();
        for (int x = 0; x < selectedItems.count(); x++) {
            AFQSourceViewItem* sourceItem = sourceList->GetItemWidget(selectedItems[x].row());

            QColor color = _GetSourceListBackgroundColor(preset);
            sourceItem->SetBackgroundColor(color);

            OBSSceneItem sceneItem =
                sourceList->Get(selectedItems[x].row());
            OBSDataAutoRelease privData =
                obs_sceneitem_get_private_settings(sceneItem);
            obs_data_set_int(privData, "color-preset", preset + 1);
            obs_data_set_string(privData, "color", "");
        }

        for (int i = 1; i < 9; i++) {
            std::stringstream button;
            button << "framePreset" << i;
            QFrame* cButtonFrame =
                colorButton->parentWidget()->parentWidget()
                ->findChild<QFrame*>(
                    button.str().c_str());
            cButtonFrame->setStyleSheet("QFrame { border: 1px solid black; border-radius:6px; }");
        }
        colorButtonFrame->setStyleSheet("QFrame { border: 1px solid #D9D9D9; border-radius:6px; }");
    }
    else if (action) {
        int preset = action->property("bgColor").value<int>();
        if (1 == preset) {
            OBSSceneItem sceneItem = SCENE_CONTEXT.GetCurrentOBSSceneItem();
            if (!sceneItem)
                return;

            AFQSourceViewItem* sourceItem =
                sourceList->GetItemWidgetFromSceneItem(sceneItem);

            OBSDataAutoRelease curPrivData =
                obs_sceneitem_get_private_settings(sceneItem);

            int oldPreset =
                obs_data_get_int(curPrivData, "color-preset");

            const QString oldSheet = sourceItem->styleSheet();
            const QColor  oldColor = sourceItem->GetBackgroundColor();

            auto liveChangeColor = [=](const QColor& color) {
                if (color.isValid()) {
                    sourceItem->SetBackgroundColor(color);
                }
            };

            auto changedColor = [=](const QColor& color) {
                if (color.isValid()) {
                    ConfirmColor(sourceList, color,
                        selectedItems);
                }
            };

            auto rejected = [=]() {
                if (oldPreset == 1) {
                    sourceItem->SetBackgroundColor(oldColor);
                }
                else if (oldPreset == 0) {
                    sourceItem->SetBackgroundColor(QColor(24, 27, 32, 84));
                }
                else {
                    QColor color = _GetSourceListBackgroundColor(oldPreset - 1);
                    sourceItem->SetBackgroundColor(color);
                }
            };

            QColorDialog::ColorDialogOptions options =
                QColorDialog::ShowAlphaChannel;

#ifdef _WIN32
            AFQCustomColorDialog* colorDialog = new AFQCustomColorDialog(oldColor,
                                                Str("CustomColorDialog.title"), MAINFRAME);
            colorDialog->setOptions(options);

            connect(colorDialog, &AFQCustomColorDialog::qsignalCurrentColorChanged,
                liveChangeColor);

            connect(colorDialog, &AFQCustomColorDialog::qsignalColorSelected,
                changedColor);

            connect(colorDialog, &AFQCustomColorDialog::rejected, rejected);

            colorDialog->open();
#else
            QColorDialog* colorDialog = new QColorDialog(MAINFRAME);
            colorDialog->setOptions(options);
            colorDialog->setCurrentColor(oldColor);
            connect(colorDialog, &QColorDialog::currentColorChanged,
                liveChangeColor);
            connect(colorDialog, &QColorDialog::colorSelected,
                changedColor);
            connect(colorDialog, &QColorDialog::rejected, rejected);
            colorDialog->open();
#endif
        }
        else {
            for (int x = 0; x < selectedItems.count(); x++) {
                AFQSourceViewItem* sourceItem
                    = sourceList->GetItemWidget(selectedItems[x].row());

                QColor color(24, 27, 32, 84);
                sourceItem->SetBackgroundColor(color);

                OBSSceneItem sceneItem = sourceList->Get(selectedItems[x].row());
                OBSDataAutoRelease privData =
                    obs_sceneitem_get_private_settings(
                        sceneItem);
                obs_data_set_int(privData, "color-preset",
                    preset);
                obs_data_set_string(privData, "color", "");
            }
        }
    }
}


void CMainSceneSource::qslotResizeOutputSizeOfSource()
{
    if (obs_video_active())
        return;

    int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        MAINFRAME, "",
        QString(Str("ResizeOutputSizeOfSource.Text")) + "\n\n" +
        QString(Str("ResizeOutputSizeOfSource.Continue")));

    if (!result)
        return;

    QWidget* outBlock = nullptr;
    MAIN_BLOCKMANAGER->FindBlock(ENUM_WINDOW_TYPE::SceneSource, outBlock);
    AFSceneSourceWidget* scenesource = reinterpret_cast<AFSceneSourceWidget*>(outBlock);

    OBSSceneItem sceneItem = scenesource->GetCurrentSceneItem();
    OBSSource source = obs_sceneitem_get_source(sceneItem);

    int width = obs_source_get_width(source);
    int height = obs_source_get_height(source);

    auto activeConfig = ACTIVECONFIG;
    //
    config_set_uint(activeConfig, "Video", "BaseCX", width);
    config_set_uint(activeConfig, "Video", "BaseCY", height);
    config_set_uint(activeConfig, "Video", "OutputCX", width);
    config_set_uint(activeConfig, "Video", "OutputCY", height);

    AFVideoUtil::ResetVideo();
    MAIN_OUTPUT->ResetOutputs();

    config_save_safe(activeConfig, "tmp", nullptr);
    qslotFitToScreen();
}

void CMainSceneSource::qslotActionScaleWindow()
{
    MAIN_PREVIEW->SetFixedScaling(false);
    MAIN_PREVIEW->ResetScrollingOffset();
    emit MAIN_PREVIEW->qsignalDisplayResized();
}

void CMainSceneSource::qslotActionScaleCanvas()
{
    MAIN_PREVIEW->SetFixedScaling(true);
    MAIN_PREVIEW->SetScalingLevel(0);
    emit MAIN_PREVIEW->qsignalDisplayResized();
}

void CMainSceneSource::qslotActionScaleOutput()
{
    obs_video_info ovi;
    obs_get_video_info(&ovi);

    MAIN_PREVIEW->SetFixedScaling(true);
    float scalingAmount = float(ovi.output_width) / float(ovi.base_width);
    // log base ZOOM_SENSITIVITY of x = log(x) / log(ZOOM_SENSITIVITY)
    int32_t approxScalingLevel =
        int32_t(round(log(scalingAmount) / log(ZOOM_SENSITIVITY)));
    MAIN_PREVIEW->SetScalingLevel(approxScalingLevel);
    MAIN_PREVIEW->SetScalingAmount(scalingAmount);
    emit MAIN_PREVIEW->qsignalDisplayResized();
}

void CMainSceneSource::qslotActionRemoveScene()
{
    AFQSceneListItem* selectedSceneItem = SCENE_CONTEXT.GetCurSelectedSceneItem();
    if (!selectedSceneItem)
        return;

    OBSScene scene = selectedSceneItem->GetScene();
    obs_source_t* source = obs_scene_get_source(scene);

    if (!source || !AFSourceUtil::QueryRemoveSource(source)) {
        return;
    }

    /* ------------------------------ */
    /* save all sources in scene      */

    OBSDataArrayAutoRelease sources_in_deleted_scene = obs_data_array_create();
    obs_scene_enum_items(scene, save_undo_source_enum, sources_in_deleted_scene);

    OBSDataAutoRelease scene_data = obs_save_source(source);
    obs_data_array_push_back(sources_in_deleted_scene, scene_data);

    OBSDataArrayAutoRelease scene_used_in_other_scenes = obs_data_array_create();

    struct other_scenes_cb_data {
        obs_source_t* oldScene;
        obs_data_array_t* scene_used_in_other_scenes;
    } other_scenes_cb_data;
    other_scenes_cb_data.oldScene = source;
    other_scenes_cb_data.scene_used_in_other_scenes = scene_used_in_other_scenes;

    auto other_scenes_cb = [](void* data_ptr, obs_source_t* scene) {
        struct other_scenes_cb_data* data = (struct other_scenes_cb_data*)data_ptr;
        if (strcmp(obs_source_get_name(scene), obs_source_get_name(data->oldScene)) == 0)
            return true;
        obs_sceneitem_t* item = obs_scene_find_source(obs_group_or_scene_from_source(scene),
                                                      obs_source_get_name(data->oldScene));
        if (item) {
            OBSDataAutoRelease scene_data = obs_save_source(obs_scene_get_source(obs_sceneitem_get_scene(item)));
            obs_data_array_push_back(data->scene_used_in_other_scenes, scene_data);
        }
        return true;
    };
    obs_enum_scenes(other_scenes_cb, &other_scenes_cb_data);

    /* --------------------------- */
    /* undo/redo                   */

	auto undo = [this](const std::string& json) {
		OBSDataAutoRelease base = obs_data_create_from_json(json.c_str());
		OBSDataArrayAutoRelease sources_in_deleted_scene = obs_data_get_array(base, "sources_in_deleted_scene");
		OBSDataArrayAutoRelease scene_used_in_other_scenes = obs_data_get_array(base, "scene_used_in_other_scenes");
		int savedIndex = (int)obs_data_get_int(base, "index");
		std::vector<OBSSource> sources;

		/* create missing sources */
		size_t count = obs_data_array_count(sources_in_deleted_scene);
		sources.reserve(count);

		for (size_t i = 0; i < count; i++)
		{
			OBSDataAutoRelease data = obs_data_array_item(sources_in_deleted_scene, i);
			const char* name = obs_data_get_string(data, "name");

			OBSSourceAutoRelease source = obs_get_source_by_name(name);
			if (!source) {
				source = obs_load_source(data);
				sources.push_back(source.Get());
			}
		}

		/* actually load sources now */
		for (obs_source_t* source : sources)
			obs_source_load2(source);

		/* Add scene to scenes and groups it was nested in */
		for (size_t i = 0; i < obs_data_array_count(scene_used_in_other_scenes); i++)
		{
			OBSDataAutoRelease data = obs_data_array_item(scene_used_in_other_scenes, i);
			const char* name = obs_data_get_string(data, "name");
			OBSSourceAutoRelease source = obs_get_source_by_name(name);
			OBSDataAutoRelease settings = obs_data_get_obj(data, "settings");
			OBSDataArrayAutoRelease items = obs_data_get_array(settings, "items");

			/* Clear scene, but keep a reference to all sources in the scene to make sure they don't get destroyed */
			std::vector<OBSSource> existing_sources;
			auto cb = [](obs_scene_t*, obs_sceneitem_t* item, void* data) {
				std::vector<OBSSource>* existing = (std::vector<OBSSource> *)data;
				OBSSource source = obs_sceneitem_get_source(item);
				obs_sceneitem_remove(item);
				existing->push_back(source);
				return true;
			};
			obs_scene_enum_items(obs_group_or_scene_from_source(source), cb, (void*)&existing_sources);

			/* Re-add sources to the scene */
			obs_sceneitems_add(obs_group_or_scene_from_source(source), items);
		}

		if (0 != sources.size())
		{
			obs_source_t* scene_source = sources.back();
			OBSScene scene = obs_scene_from_source(scene_source);
			App()->GetMainView()->GetMainWindow()->SetCurrentScene(scene, true);

			/* set original index in list box */
			QWidget* outBlock = nullptr;
			if (MAIN_BLOCKMANAGER->FindBlock(ENUM_WINDOW_TYPE::SceneSource, outBlock))
			{
				AFSceneSourceWidget* scenesourceBlock = reinterpret_cast<AFSceneSourceWidget*>(outBlock);
				if (scenesourceBlock)
				{
					const int from = SCENE_CONTEXT.GetSceneItemSize() - 1;
					const int dest = savedIndex;
					QMetaObject::invokeMethod(scenesourceBlock, "qslotSwapItem", Qt::QueuedConnection,
                                              Q_ARG(int, from), Q_ARG(int, dest));
				}
			}
		}
	};

    auto redo = [](const std::string& name) {
        OBSSourceAutoRelease source = obs_get_source_by_name(name.c_str());
        RemoveSceneAndReleaseNested(source);
    };

    OBSDataAutoRelease data = obs_data_create();
    obs_data_set_array(data, "sources_in_deleted_scene", sources_in_deleted_scene);
    obs_data_set_array(data, "scene_used_in_other_scenes", scene_used_in_other_scenes);
    obs_data_set_int(data, "index", selectedSceneItem->GetSceneIndex());

    const char* scene_name = obs_source_get_name(source);
    AFMainFrame* main = App()->GetMainView();
    UNDO_STACK.AddAction(QTStr("Undo.Delete").arg(scene_name), undo, redo, obs_data_get_json(data), scene_name);

    /* --------------------------- */
    /* remove                      */

    RemoveSceneAndReleaseNested(source);
}

void CMainSceneSource::qslotActionRenameScene()
{
    AFQSceneListItem* selectedSceneItem = SCENE_CONTEXT.GetCurSelectedSceneItem();
    if (!selectedSceneItem)
        return;

    selectedSceneItem->ShowRenameSceneUI();
}

void CMainSceneSource::qslotAddSceneTriggered()
{
    std::string name;
    QString format{ Str("Basic.Main.DefaultSceneName.Text") };

    int i = 2;
    QString placeHolderText = format.arg(i);
    OBSSourceAutoRelease findSrc = nullptr;
    while ((findSrc = obs_get_source_by_name(QT_TO_UTF8(placeHolderText)))) {
        placeHolderText = format.arg(++i);
    }

    bool accept = AFQNameDialog::AskForName(MAINFRAME,
        Str("Basic.Main.AddSceneDlg.Title"),
        Str("Basic.Main.AddSceneDlg.Text"),
        name, placeHolderText, 170, true);
    if (accept) {

        if (name.empty()) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
                MAINFRAME,
                Str("NoNameEntered.Title"),
                Str("NoNameEntered.Text"),
                true, true);
            qslotAddSceneTriggered();
            return;
        }

        OBSSourceAutoRelease source = obs_get_source_by_name(name.c_str());
        if (source) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
                MAINFRAME,
                Str("NameExists.Title"),
                Str("NameExists.Text"),
                true, true);
            qslotAddSceneTriggered();
            return;
        }

        auto undo_fn = [](const std::string& data) {
            obs_source_t* t = obs_get_source_by_name(data.c_str());
            if (t) {
                obs_source_remove(t);
                obs_source_release(t);
            }
        };
        auto redo_fn = [this](const std::string& data) {
            OBSSceneAutoRelease scene = obs_scene_create(data.c_str());
            obs_source_t* source = obs_scene_get_source(scene);
            MAINFRAME->SetCurrentScene(source, true);
        };
        UNDO_STACK.AddAction(QTStr("Undo.Add").arg(QString(name.c_str())),
            undo_fn, redo_fn, name, name);

        obs_source_t* scene_source = AFSceneUtil::CreateOBSScene(name.c_str());
        MAINFRAME->SetCurrentScene(scene_source);
    }
}

void CMainSceneSource::qslotSceneButtonClicked(OBSScene scene)
{
    if (!scene)
        return;

    obs_source_t* source = obs_scene_get_source(scene);

    MAINFRAME->SetCurrentScene(source);
}

void CMainSceneSource::qslotSceneButtonDoubleClicked(OBSScene scene)
{
    if (!scene)
        return;

    UNUSED_PARAMETER(scene);

    DYNAMIC_COMPOSIT->ChangeSceneOnDoubleClick();
}

void CMainSceneSource::qslotSceneButtonDotClicked()
{
    SceneItemVector& sceneItems = SCENE_CONTEXT.GetSceneItemVector();
    const OBSScene curObsScene = SCENE_CONTEXT.GetCurrentScene();

    if (sceneItems.empty())
        return;

    AFQCustomMenu popup(MAINFRAME);

    QAction* actionSelectSourcePopup = new QAction(Str("AddSource"), this);
    connect(actionSelectSourcePopup, &QAction::triggered,
            this, &CMainSceneSource::qslotShowSelectSourcePopup);

    popup.addAction(actionSelectSourcePopup);
    popup.addSeparator();

    QAction* actionPopupSceneDock = new QAction(Str("Basic.BottomSceneMenu.ShowSceneSourceDock"), this);
    auto popupSceneDock = [this] {
        AFQBorderPopupBaseWidget* pupup = nullptr;
        MAINFRAME->GetBlockManager()->MakePopup(ENUM_WINDOW_TYPE::SceneSource, pupup);
    };
    connect(actionPopupSceneDock, &QAction::triggered, popupSceneDock);
    popup.addAction(actionPopupSceneDock);

    popup.addSeparator();

    popup.exec(QCursor::pos());
}

void CMainSceneSource::AddNewSource(QString sourceId, bool addOnProgramMode)
{
    if (sourceId == "window_area_capture") {
        QTimer::singleShot(200, this,
            [this, sourceId, addOnProgramMode]() {
                MAINFRAME->ShowWindowCaptureArea(
                    nullptr,
                    [this, sourceId, addOnProgramMode](const std::optional<WindowCaptureAreaResult>& captureResult) {

                        if (!captureResult)
                            return;

                        AddNewSourceInternal(
                            sourceId,
                            addOnProgramMode,
                            captureResult);
                    });
            });

        return;
    }

    AddNewSourceInternal(sourceId, addOnProgramMode, std::nullopt);
}

void CMainSceneSource::AddNewSourceInternal(const QString & sourceId,
                                        bool addOnProgramMode,
                                        const std::optional<WindowCaptureAreaResult>&windowAreaResult)
{
    if (0 == sourceId.compare("scene")) {
        MAINFRAME->ShowSceneSourceSelectList();
        return;
    }

    if (0 != AFSourceUtil::CheckAddSoopVodSource(sourceId.toStdString().c_str()))
        return;

    if (0 != AFSourceUtil::CheckAddSoopKBOSource(sourceId.toStdString().c_str()))
        return;

    int aiResult = AFSourceUtil::CheckAddSoopAI(sourceId.toStdString().c_str());
    if (1 < aiResult)
        return;

    if (0 != AFSourceUtil::CheckAddSoopFootballSource(sourceId.toStdString().c_str()))
        return;

    //
    if (0 == sourceId.compare("soop_chat_source_mood_check")) {
        int nCnt = config_get_int(USERCONFIG, "SARSA", "UseMinsimCheckCnt");
        config_set_int(USERCONFIG, "SARSA", "UseMinsimCheckCnt", ++nCnt);

        if (AFOutputUtil::IsStreamActive()) {
            if (nCnt == 1) {
                auto startTime = std::chrono::steady_clock::now();
                AUTH_CONTEXT.SetMinsimCheckStartTime(startTime);
            }
        }
    }

    obs_transform_info itemInfo;
    obs_transform_info* pInfo = nullptr;
    if (AFSourceUtil::IsBrowserSizeStretch(sourceId.toStdString().c_str()))
    {
        vec2_set(&itemInfo.pos, 0, 0);
        vec2_set(&itemInfo.scale, 1.0f, 1.0f);
        vec2_set(&itemInfo.bounds, 800, 600);

        itemInfo.alignment = OBS_ALIGN_LEFT | OBS_ALIGN_TOP;
        itemInfo.rot = 0.0f;
        itemInfo.bounds_type = OBS_BOUNDS_STRETCH;
        itemInfo.bounds_alignment = OBS_ALIGN_CENTER;

        pInfo = &itemInfo;
    }

    if(0 == sourceId.compare("painter_source"))
    {
        obs_video_info ovi;
        obs_get_video_info(&ovi);

        vec2_set(&itemInfo.pos, 0, 0);
        vec2_set(&itemInfo.scale, 1.0f, 1.0f);
        vec2_set(&itemInfo.bounds, ovi.base_width, ovi.base_height);

        itemInfo.alignment = OBS_ALIGN_LEFT | OBS_ALIGN_TOP;
        itemInfo.rot = 0.0f;
        itemInfo.bounds_type = OBS_BOUNDS_NONE;
        itemInfo.bounds_alignment = OBS_ALIGN_CENTER;

        pInfo = &itemInfo;
    }

    if (0 == sourceId.compare("image_source") ||
        0 == sourceId.compare("ffmpeg_source") ||
        AFSourceUtil::IsSoopMediaSource(sourceId.toStdString().c_str()))
    {
        // NOTE: Scale must be set in the "media_file_load" callback.
        // Scale should be applied after the file is successfully loaded.
        // recv callback SourceToolbar "image_source" => AFQImageSourceToolbar, "ffmpeg_source" => AFQMediaSourceToolbar
        vec2_set(&itemInfo.pos, 0, 0);
        vec2_set(&itemInfo.scale, 0.0f, 0.0f);
        vec2_set(&itemInfo.bounds, 0.0f, 0.0f);

        itemInfo.alignment = OBS_ALIGN_LEFT | OBS_ALIGN_TOP;
        itemInfo.rot = 0.0f;
        itemInfo.bounds_type = OBS_BOUNDS_NONE;
        itemInfo.bounds_alignment = OBS_ALIGN_CENTER;

        pInfo = &itemInfo;
    }

    OBSSource newSource;
    QString displayText = AFSourceUtil::GetPlaceHodlerText(
        sourceId.toStdString().c_str());

    if (!AFSourceUtil::AddNewSource(MAINFRAME,
                                    sourceId.toStdString().c_str(),
                                    displayText.toStdString().c_str(),
                                    true,
                                    newSource,
                                    pInfo,
                                    nullptr,
                                    addOnProgramMode)) {
        return;
    }

    OBSDataAutoRelease settings = obs_source_get_settings(newSource);
    if (sourceId == "window_area_capture") {
        if (windowAreaResult) {
            WindowCaptureAreaWidget::ApplyResult(
                settings,
                *windowAreaResult);

            obs_source_update(newSource, settings);
        }

        AFSourceUtil::SetUndoRedoAddSource(sourceId.toStdString().c_str(), displayText.toStdString().c_str(), true);
        return;
    }

    if (0 == sourceId.compare("browser_source")) {
        obs_data_set_string(settings, "url", SOOPLIVE_KR_URL);
        obs_source_update(newSource, settings);
    }

    if (0 == sourceId.compare("soop_aimanager_source")) {
        std::string url = AFSourceUtil::GetAIManagerURL();

        obs_video_info ovi;
        obs_get_video_info(&ovi);

        obs_data_set_string(settings, "url", url.c_str());
        obs_data_set_int(settings, "width", ovi.base_width);
        obs_data_set_int(settings, "height", ovi.base_height);

        obs_source_set_monitoring_type(newSource, OBS_MONITORING_TYPE_NONE);
        obs_source_update(newSource, settings);
    }
    
    if (AFSourceUtil::IsSoopKBOSource(sourceId.toStdString().c_str()))
    {
        QSize browserSize = QSize(960, 540);
        if (0 == sourceId.compare("soop_kbo_graphic_source_score"))
            browserSize = QSize(930, 186);
        else if (0 == sourceId.compare("soop_kbo_graphic_source_stadium"))
            browserSize = QSize(780, 632);
        else if (0 == sourceId.compare("soop_kbo_graphic_source_player"))
            browserSize = QSize(448, 720);
        else if (0 == sourceId.compare("soop_kbo_graphic_source_livetext"))
            browserSize = QSize(448, 720);

        obs_data_set_int(settings, "width", browserSize.width());
        obs_data_set_int(settings, "height", browserSize.height());

        obs_source_update(newSource, settings);
    }

    else if (AFSourceUtil::IsSoopFootballSource(sourceId.toStdString().c_str()))
    {
        QSize browserSize = QSize(1920, 1080);    

        if (0 == sourceId.compare("soop_football_graphic_source_player"))
        {
            browserSize = QSize(740, 1080);
        }
        else if (0 == sourceId.compare("soop_football_graphic_source_change"))
        {
            browserSize = QSize(450, 1080);
        }
        else if (0 == sourceId.compare("soop_football_graphic_source_score"))
        {
            browserSize = QSize(706, 146);
        }
        else if (0 == sourceId.compare("soop_football_graphic_source_livetext"))
        {
            browserSize = QSize(706, 920);
        }
        obs_data_set_int(settings, "width", browserSize.width());
        obs_data_set_int(settings, "height", browserSize.height());

        obs_source_update(newSource, settings);
    }
    else if(0 == sourceId.compare("soop_chat_source_mood_check")) {


        std::string user_id;
        int broadNum = 0;
        AUTH_CONTEXT.GetChannelID(PLATFORM_SOOP, user_id);
        AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
        if(broadInfo)
            broadNum = broadInfo->BroadNumber();
        obs_data_set_string(settings, "streamer_id", user_id.c_str());
        obs_data_set_string(settings, "broadNum", std::to_string(broadNum).c_str());
        obs_source_update(newSource, settings);

        auto scene = SCENE_CONTEXT.GetCurrentScene();
        OBSSceneItemAutoRelease item = obs_scene_sceneitem_from_source(scene, newSource);
        obs_sceneitem_defer_update_begin(item);
        obs_transform_info info;
        obs_sceneitem_get_info(item, &info);
        vec2_set(&info.bounds, 400, 300);
        info.bounds_type = OBS_BOUNDS_STRETCH;
        obs_sceneitem_set_info(item.Get(), &info);
        obs_sceneitem_defer_update_end(item);
    }

   if (0 == sourceId.compare("painter_source")) {
        obs_video_info ovi;
        obs_get_video_info(&ovi);

        obs_data_set_int(settings, "width", ovi.base_width);
        obs_data_set_int(settings, "height", ovi.base_height);

        obs_source_update(newSource, settings);
    }

    if (AFSourceUtil::ShouldShowProperties(newSource)) {

        if (AFSourceUtil::IsSoopMediaSource(newSource))
            SOOP_SRC_MANAGER.SetSoopMediaSource(newSource);

        MAINFRAME->CreateSourceProperties(newSource);
    }

    AFSourceUtil::SetUndoRedoAddSource(sourceId.toStdString().c_str(), displayText.toStdString().c_str(), true);
}

void CMainSceneSource::CreateSourcePopupMenu(int idx, bool preview)
{
    AFMainFrame* mainFrame = MAINFRAME;
    if (!mainFrame)
        return;
    bool isSmallResolution = mainFrame->IsSmallResolution();

    Ui::AFMainFrame* ui = MAINFRAME_UI;
    if (!ui)
        return;

    AFQSourceListView* sourceListView = SCENE_CONTEXT.GetSourceListViewPtr();
    if (!sourceListView)
        return;

    CBasicPreview* basicPreview = DYNAMIC_COMPOSIT ? DYNAMIC_COMPOSIT->GetMainPreview() : nullptr;

    if (m_widgetColorSelect) delete m_widgetColorSelect;
    if (m_widgetActionColor) delete m_widgetActionColor;
    if (m_menuScaleFiltering) delete m_menuScaleFiltering;
    if (m_menuBlendingMode) delete m_menuBlendingMode;
    if (m_menuBlendingMethodMode) delete m_menuBlendingMethodMode;
    if (m_menuDeinterlace) delete m_menuDeinterlace;

    AFQCustomMenu popup(mainFrame);
    AFQCustomMenu order(Str("Basic.MainMenu.Edit.Order"), mainFrame, true);
    AFQCustomMenu colorMenu(Str("ChangeBG"), mainFrame, true);
    AFQCustomMenu transform(Str("Basic.MainMenu.Edit.Transform"), mainFrame, true);

    QScreen* screen = mainFrame->screen();
    popup.setMaximumHeight(screen->availableGeometry().height());

    if (preview) {
        QAction* action = popup.addAction(Str("Basic.Main.PreviewConextMenu.Enable"),
            mainFrame, &AFMainFrame::qslotTogglePreview);
        action->setCheckable(true);
        if(basicPreview && basicPreview->GetDisplay())
            action->setChecked(obs_display_enabled(basicPreview->GetDisplay()));
        else
            action->setChecked(false);

        if (MAINFRAME->IsPreviewProgramMode())
            action->setEnabled(false);

        popup.addAction(MAINFRAME_UI->action_LockPreview);

        AFQCustomMenu* scalingMenu = new AFQCustomMenu(mainFrame, true);
        scalingMenu->setFixedWidth(200);

        obs_video_info ovi;
        obs_get_video_info(&ovi);
        {
            QAction* actionCanvas = MAINFRAME_UI->action_ScaleCanvas;
            QString text = QTStr("Basic.MainMenu.Edit.Scale.Canvas");
            text = text.arg(QString::number(ovi.base_width),
                            QString::number(ovi.base_height));
            actionCanvas->setText(text);

            QAction*  actionScaleOutput = MAINFRAME_UI->action_ScaleOutput;
            text = QTStr("Basic.MainMenu.Edit.Scale.Output");
            text = text.arg(QString::number(ovi.output_width),
                            QString::number(ovi.output_height));
            actionScaleOutput->setText(text);
            actionScaleOutput->setVisible(!(ovi.output_width == ovi.base_width &&
                                            ovi.output_height == ovi.base_height));

            if(basicPreview)
            {
                bool fixedScaling = basicPreview->IsFixedScaling();
                float scalingAmount = basicPreview->GetScalingAmount();
                if (!fixedScaling)
                {
                    MAINFRAME_UI->action_ScaleWindow->setChecked(true);
                    MAINFRAME_UI->action_ScaleCanvas->setChecked(false);
                    MAINFRAME_UI->action_ScaleOutput->setChecked(false);
                }
                else
                {
                    MAINFRAME_UI->action_ScaleWindow->setChecked(false);
                    MAINFRAME_UI->action_ScaleCanvas->setChecked(scalingAmount == 1.0f);
                    MAINFRAME_UI->action_ScaleOutput->setChecked(scalingAmount ==
                        float(ovi.output_width) / float(ovi.base_width));
                }
            }
        }

        scalingMenu->addAction(MAINFRAME_UI->action_ScaleWindow);
        scalingMenu->addAction(MAINFRAME_UI->action_ScaleCanvas);
        scalingMenu->addAction(MAINFRAME_UI->action_ScaleOutput);

        connect(MAINFRAME_UI->action_ScaleWindow, &QAction::triggered,
                this, &CMainSceneSource::qslotActionScaleWindow);
        connect(MAINFRAME_UI->action_ScaleCanvas, &QAction::triggered,
                this, &CMainSceneSource::qslotActionScaleCanvas);
        connect(MAINFRAME_UI->action_ScaleOutput, &QAction::triggered,
                this, &CMainSceneSource::qslotActionScaleOutput);

        popup.addMenu(scalingMenu)->setText(QT_UTF8(Str("Basic.MainMenu.Edit.Scale")));
        popup.addSeparator();

        //popup.addAction(Str("AddSource"), this, &CMainSceneSource::qslotShowSelectSourcePopup);
    }
    //else {
    //    QPointer<AFQCustomMenu> addSourceMenu = _CreateAddSourcePopupMenu();
    //    if (addSourceMenu)
    //        popup.addMenu(addSourceMenu);
    //}

    bool shouldAddSource = preview ||
        (idx == -1) ||
        (sourceListView->GroupsSelected());

    if (shouldAddSource) {
        auto action = popup.addAction(Str("AddSource"), this, &CMainSceneSource::qslotShowSelectSourcePopup);
#pragma region _SOOP_BREAKTIME
        action->setEnabled(!BREAKTIME_MANAGER.IsActive());
#pragma endregion
    }

	if (sourceListView->MultipleBaseSelected()) {

		bool notAllowGroup = false;
		for (auto& selectedSource : sourceListView->selectionModel()->selectedIndexes()) {
			OBSSceneItem item = sourceListView->Get(selectedSource.row());
			if (!item)
				continue;

			OBSSource source = obs_sceneitem_get_source(item);
			if (AFSourceUtil::IsSoopMediaSource(source)) {
                notAllowGroup = true;
				break;
			}

            const char* sourceId = obs_source_get_id(source);
            if(strcmp("painter_source", sourceId) == 0 || strcmp("soop_aimanager_source", sourceId) == 0) {
                notAllowGroup = true;
                break;
            }
		}
		if (!notAllowGroup) {
			popup.addSeparator();
			popup.addAction(Str("Basic.Main.GroupItems"), sourceListView,
				&AFQSourceListView::GroupSelectedItems);
		}
	}
	else if (sourceListView->GroupsSelected()) {
		popup.addSeparator();
		popup.addAction(Str("Basic.Main.Ungroup"), sourceListView,
			&AFQSourceListView::UngroupSelectedGroups);
	}

    popup.addSeparator();

    if (isSmallResolution)
    {
        QString copyPasteText = QString("%1/%2").arg(Str("Copy")).arg(Str("Paste"));
        AFQCustomMenu* copyPaste = new AFQCustomMenu(copyPasteText, nullptr, true);
        copyPaste->addAction(MAINFRAME_UI->action_CopySource);
        copyPaste->addAction(MAINFRAME_UI->action_PasteSource);
        //copyPaste->addAction(MAINFRAME_UI->action_PasteSourceRef);
        //copyPaste->addAction(MAINFRAME_UI->action_PasteSourceDuplicate);
        popup.addMenu(copyPaste);
    }
    else
    {
        popup.addAction(MAINFRAME_UI->action_CopySource);
        popup.addAction(MAINFRAME_UI->action_PasteSource);
        //popup.addAction(MAINFRAME_UI->action_PasteSourceRef);
        //popup.addAction(MAINFRAME_UI->action_PasteSourceDuplicate);
    }

    popup.addSeparator();

    if (idx != -1)
    {
        OBSSceneItem sceneItem = SCENE_CONTEXT.GetCurrentOBSSceneItem(idx);
        if (!sceneItem)
            return;

        obs_source_t* source = obs_sceneitem_get_source(sceneItem);
        if (!source)
            return;

        std::string source_id = obs_source_get_unversioned_id(source);
        uint32_t flags = obs_source_get_output_flags(source);
        bool isAsyncVideo = (flags & OBS_SOURCE_ASYNC_VIDEO) == OBS_SOURCE_ASYNC_VIDEO;
        bool hasAudio = (flags & OBS_SOURCE_AUDIO) == OBS_SOURCE_AUDIO;
        bool hasVideo = (flags & OBS_SOURCE_VIDEO) == OBS_SOURCE_VIDEO;
        bool isPainterSource = (0 == source_id.compare("painter_source"));
        bool isAIManagerSource = (0 == source_id.compare("soop_aimanager_source"));

        if (!AFSourceUtil::IsSoopSource(source))
        {
            popup.addSeparator();

            if (isSmallResolution)
            {
                QString filtersText = QString("%1 %2").arg(Str("Filters")).arg(Str("Settings"));
                AFQCustomMenu* filters = new AFQCustomMenu(filtersText, nullptr, true);
                filters->addAction(MAINFRAME_UI->action_Filters);
                filters->addAction(MAINFRAME_UI->action_CopyFilters);
                filters->addAction(MAINFRAME_UI->action_PasteFilters);
                popup.addMenu(filters);
            }
            else
            {
                popup.addAction(MAINFRAME_UI->action_Filters);
                popup.addAction(MAINFRAME_UI->action_CopyFilters);
                popup.addAction(MAINFRAME_UI->action_PasteFilters);
            }
            
#ifdef _WIN32
            if (source_id == "dshow_input")
#elif defined(__APPLE__)
            if (source_id == "av_capture_input")
#endif
                popup.addAction(MAINFRAME_UI->action_SplitEffect);
            popup.addSeparator();
        }

        popup.addSeparator();
        m_widgetActionColor = new QWidgetAction(&colorMenu);
        m_widgetColorSelect = new AFQColorSelect(&colorMenu);
        popup.addMenu(_AddBackgroundColorMenu(
                &colorMenu, m_widgetActionColor, m_widgetColorSelect, sceneItem));
        popup.addAction(MAINFRAME_UI->action_RenameSource);
        popup.addAction(MAINFRAME_UI->action_RemoveSource);
        popup.addSeparator();

        if (!AFSourceUtil::IsSoopMediaSource(source))
        {
            popup.addSeparator();
            order.addAction(MAINFRAME_UI->action_OrderMoveUp);
            order.addAction(MAINFRAME_UI->action_OrderMoveDown);
            order.addAction(MAINFRAME_UI->action_OrderMoveToTop);
            order.addAction(MAINFRAME_UI->action_OrderMoveToBottom);
            popup.addMenu(&order);
            popup.addSeparator();
        }

        popup.addSeparator();

        bool notAllowTransform = (AFSourceUtil::IsSoopMediaSource(source) || isPainterSource || isAIManagerSource);
        if(hasVideo && !notAllowTransform) {
            popup.addSeparator();
            transform.addAction(MAINFRAME_UI->action_EditTransform);
            transform.addAction(MAINFRAME_UI->action_CopyTransform);
            transform.addAction(MAINFRAME_UI->action_PasteTransform);
            transform.addAction(MAINFRAME_UI->action_ResetTransform);
            transform.addSeparator();
            transform.addAction(MAINFRAME_UI->action_Rotate90CW);
            transform.addAction(MAINFRAME_UI->action_Rotate90CCW);
            transform.addAction(MAINFRAME_UI->action_Rotate180);
            transform.addSeparator();
            transform.addAction(MAINFRAME_UI->action_FlipHorizontal);
            transform.addAction(MAINFRAME_UI->action_FlipVertical);
            transform.addSeparator();
            transform.addAction(MAINFRAME_UI->action_FitToScreen);
            transform.addAction(MAINFRAME_UI->action_StretchToScreen);
            transform.addAction(MAINFRAME_UI->action_CenterToScreen);
            transform.addAction(MAINFRAME_UI->action_VerticalCenter);
            transform.addAction(MAINFRAME_UI->action_HorizontalCenter);
            popup.addMenu(&transform);
            popup.addSeparator();
        }

        if (hasAudio) {
            //QAction* actionHideMixer =
            //    popup.addAction(QTStr("HideMixer"), this,
            //        &OBSBasic::ToggleHideMixer);
            //actionHideMixer->setCheckable(true);
            //actionHideMixer->setChecked(SourceMixerHidden(source));
            //popup.addSeparator();
        }

        if (!AFSourceUtil::IsSoopSource(source))
        {
			if (hasVideo) {
				//QAction* resizeOutput = popup.addAction(Str("ResizeOutputSizeOfSource"),
				//	this, &CMainSceneSource::qslotResizeOutputSizeOfSource);

				//int width = obs_source_get_width(source);
				//int height = obs_source_get_height(source);

				//resizeOutput->setEnabled(!obs_video_active());

				//if (width < 32 || height < 32)
				//	resizeOutput->setEnabled(false);

                if (isSmallResolution)
                {
                    QString advancedtext = QString("%1 %2").arg(Str("Basic.Settings.Advanced")).arg(Str("Settings"));
                    AFQCustomMenu* advanced = new AFQCustomMenu(advancedtext, nullptr, true);
                    m_menuScaleFiltering = new AFQCustomMenu(Str("ScaleFiltering"), nullptr, true);
                    advanced->addMenu(_AddScaleFilteringMenu(m_menuScaleFiltering, sceneItem));

                    m_menuBlendingMode = new AFQCustomMenu(Str("BlendingMode"), nullptr, true);
                    advanced->addMenu(_AddBlendingModeMenu(m_menuBlendingMode, sceneItem));

                    m_menuBlendingMethodMode = new AFQCustomMenu(Str("BlendingMethod"), nullptr, true);
                    advanced->addMenu(_AddBlendingMethodMenu(m_menuBlendingMethodMode, sceneItem));

                    if (isAsyncVideo) {
                        m_menuDeinterlace = new AFQCustomMenu(Str("Deinterlacing"), nullptr, true);
                        advanced->addMenu(_AddDeinterlacingMenu(m_menuDeinterlace, source));
                    }
                    popup.addMenu(advanced);
                }
                else
                {
                    m_menuScaleFiltering = new AFQCustomMenu(Str("ScaleFiltering"), nullptr, true);
                    popup.addMenu(_AddScaleFilteringMenu(m_menuScaleFiltering, sceneItem));

                    m_menuBlendingMode = new AFQCustomMenu(Str("BlendingMode"), nullptr, true);
                    popup.addMenu(_AddBlendingModeMenu(m_menuBlendingMode, sceneItem));

                    m_menuBlendingMethodMode = new AFQCustomMenu(Str("BlendingMethod"), nullptr, true);
                    popup.addMenu(_AddBlendingMethodMenu(m_menuBlendingMethodMode, sceneItem));

                    if (isAsyncVideo) {
                        m_menuDeinterlace = new AFQCustomMenu(Str("Deinterlacing"), nullptr, true);
                        popup.addMenu(_AddDeinterlacingMenu(m_menuDeinterlace, source));
                    }
                }

				popup.addSeparator();
			}
        }

        {
            popup.addSeparator();
            {
                if (flags & OBS_SOURCE_INTERACTION) {
                    if(!isPainterSource)
                        popup.addAction(MAINFRAME_UI->action_ShowInteract);
                }

                popup.addAction(MAINFRAME_UI->action_ShowProperties);
                MAINFRAME_UI->action_ShowProperties->setEnabled(obs_source_configurable(source));
            }
            popup.addSeparator();
        }
    }
    popup.exec(QCursor::pos());
}

void CMainSceneSource::CreateDefaultScene(bool firstStart)
{
    LOADSAVE_CONTEXT.IncreaseCheckSaveCnt();

    MAINFRAME->ClearSceneData();

    SCENE_CONTEXT.InitContext();
    SCENE_CONTEXT.InitDefaultTransition();

    SCENE_CONTEXT.SetTransition(SCENE_CONTEXT.GetFadeTransition());

    if (firstStart)
        AFAudioUtil::CreateFirstRunSources();

    OBSSceneAutoRelease firstScene;
    for (int i = 0; i < 1; i++) {
        obs_scene_t* scene = obs_scene_create(Str("Basic.DefaultScene"));

        if (0 == i)
            firstScene = scene;
    }
    obs_source_t* source = obs_scene_get_source(firstScene);

    // register favorite scene
    OBSDataAutoRelease sceneData = obs_source_get_settings(source);
    obs_data_set_int(sceneData, "favorite_scene", 1);

    MAINFRAME->SetCurrentScene(source, true);

    LOADSAVE_CONTEXT.DecreaseCheckSaveCnt();
}

void CMainSceneSource::Screenshot(OBSSource source)
{
    if (!!m_screenshotData) {
        blog(LOG_WARNING, "Cannot take new screenshot, "
            "screenshot currently in progress");
        return;
    }

    m_screenshotData = new ScreenShotObj(AFSceneUtil::CnvtToOBSSource(SCENE_CONTEXT.GetCurrentScene()));
}

void CMainSceneSource::ScreenshotSource(OBSSource source, bool internalSave)
{
    if (!!m_screenshotData) {
        blog(LOG_WARNING, "Cannot take new screenshot, "
            "screenshot currently in progress");
        return;
    }

    ScreenShotObj* screenShotObj = new ScreenShotObj(source, internalSave);
    m_screenshotData = screenShotObj;
    connect(screenShotObj, &ScreenShotObj::ScreenShotFinished, this, &CMainSceneSource::qsignalScreenShotFinished);
}

void CMainSceneSource::UpdateEditMenu()
{
    AFQSourceListView* sourceListView = SCENE_CONTEXT.GetSourceListViewPtr();
    if (!sourceListView)
        return;

    QModelIndexList items = sourceListView->selectionModel()->selectedIndexes();
    int totalCount = items.count();
    size_t filter_count = 0;

    if (totalCount == 1) {
        OBSSceneItem sceneItem = sourceListView->Get(sourceListView->GetTopSelectedSourceItem());
        OBSSource source = obs_sceneitem_get_source(sceneItem);
        filter_count = obs_source_filter_count(source);
    }

    bool allowPastingDuplicate = !!m_clipboard.size()
#pragma region _SOOP_BREAKTIME
        && !BREAKTIME_MANAGER.IsActive();
#pragma endregion

    for (size_t i = m_clipboard.size(); i > 0; i--) {
        const size_t idx = i - 1;
        OBSWeakSource& weak = m_clipboard[idx].weak_source;
        if (obs_weak_source_expired(weak)) {
            m_clipboard.erase(m_clipboard.begin() + idx);
            continue;
        }
        OBSSourceAutoRelease strong = obs_weak_source_get_source(weak.Get());
        if (allowPastingDuplicate &&
            obs_source_get_output_flags(strong) &
            OBS_SOURCE_DO_NOT_DUPLICATE)
            allowPastingDuplicate = false;
    }

    int videoCount = 0;
    bool canTransformMultiple = false;
    bool disableCopySource = false;
    for (int i = 0; i < totalCount; i++) {
        OBSSceneItem item = sourceListView->Get(items.value(i).row());
        OBSSource source = obs_sceneitem_get_source(item);
        const uint32_t flags = obs_source_get_output_flags(source);
        const bool hasVideo = (flags & OBS_SOURCE_VIDEO) != 0;
        if (hasVideo && !obs_sceneitem_locked(item))
            canTransformMultiple = true;

        if (hasVideo)
            videoCount++;

        std::string id = obs_source_get_id(source);
        if (AFSourceUtil::IsSoopKBOSource(source) ||
            AFSourceUtil::IsSoopFootballSource(source) ||
            AFSourceUtil::IsSoopMediaSource(source) ||
            0 == id.compare("soop_videoballoon_source") ||
            0 == id.compare("soop_aimanager_source")) {
            disableCopySource = true;
        }
    }

    const bool canTransformSingle = videoCount == 1 && totalCount == 1;

    bool enableCopy = false;
    if (totalCount > 0) {
        if (totalCount == 1 && disableCopySource)
            enableCopy = false;
        else
            enableCopy = true;
    }
    MAINFRAME_UI->action_CopySource->setEnabled(enableCopy);
    MAINFRAME_UI->action_EditTransform->setEnabled(canTransformSingle);
    MAINFRAME_UI->action_CopyTransform->setEnabled(canTransformSingle);
    MAINFRAME_UI->action_PasteTransform->setEnabled(m_hasCopiedTransform && videoCount > 0);
    MAINFRAME_UI->action_CopyFilters->setEnabled(filter_count > 0);
    MAINFRAME_UI->action_PasteFilters->setEnabled(!obs_weak_source_expired(SCENE_CONTEXT.m_obsCopyFiltersSource) && totalCount > 0);
//    MAINFRAME_UI->action_PasteSourceRef->setEnabled(!!m_clipboard.size()
//#pragma region _SOOP_BREAKTIME
//                && !BREAKTIME_MANAGER.IsActive());
//#pragma endregion
//
//    MAINFRAME_UI->action_PasteSourceDuplicate->setEnabled(allowPastingDuplicate);

    MAINFRAME_UI->action_PasteSource->setEnabled(!!m_clipboard.size() && !BREAKTIME_MANAGER.IsActive());

    MAINFRAME_UI->action_OrderMoveUp->setEnabled(totalCount > 0);
    MAINFRAME_UI->action_OrderMoveDown->setEnabled(totalCount > 0);
    MAINFRAME_UI->action_OrderMoveToTop->setEnabled(totalCount > 0);
    MAINFRAME_UI->action_OrderMoveToBottom->setEnabled(totalCount > 0);

    MAINFRAME_UI->action_ResetTransform->setEnabled(canTransformMultiple);
    MAINFRAME_UI->action_Rotate90CW->setEnabled(canTransformMultiple);
    MAINFRAME_UI->action_Rotate90CCW->setEnabled(canTransformMultiple);
    MAINFRAME_UI->action_Rotate180->setEnabled(canTransformMultiple);
    MAINFRAME_UI->action_FlipHorizontal->setEnabled(canTransformMultiple);
    MAINFRAME_UI->action_FlipVertical->setEnabled(canTransformMultiple);
    MAINFRAME_UI->action_FitToScreen->setEnabled(canTransformMultiple);
    MAINFRAME_UI->action_StretchToScreen->setEnabled(canTransformMultiple);
    MAINFRAME_UI->action_CenterToScreen->setEnabled(canTransformMultiple);
    MAINFRAME_UI->action_VerticalCenter->setEnabled(canTransformMultiple);
    MAINFRAME_UI->action_HorizontalCenter->setEnabled(canTransformMultiple);
}

void CMainSceneSource::ClearClipboard()
{
    m_clipboard.clear();
}

void CMainSceneSource::AddSceneBottomButton(AFQSceneBottomButton* button)
{
    m_sceneButtons.emplace_back(button);
}

void CMainSceneSource::ClearSceneBottomButtons()
{
    auto it = m_sceneButtons.begin();
    for (; it != m_sceneButtons.end(); it++) {
        AFQSceneBottomButton* button = (*it);
        if (button) {
            button->close();
            delete button;
        }
    }
    m_sceneButtons.clear();
}

void CMainSceneSource::SetSceneBottomButtonStyleSheet(OBSSource scene)
{
    std::vector<AFQSceneBottomButton*>::iterator iter = m_sceneButtons.begin();
    for (; iter != m_sceneButtons.end(); ++iter) {
        AFQSceneBottomButton* button = (*iter);
        if (!button || !button->GetObsScene())
            continue;

        const OBSSource source = OBSSource(obs_scene_get_source(button->GetObsScene()));

        button->SetSelectedState(source == scene);
    }
}

inline bool CheckEnableInputSource(const char* id)
{
    size_t idx = 0;
    const char* unversioned_type;
    const char* type;

    while (obs_enum_input_types2(idx++, &type, &unversioned_type)) {
        const char* name = obs_source_get_display_name(type);
        uint32_t caps = obs_get_source_output_flags(type);

        if ((caps & OBS_SOURCE_CAP_DISABLED) != 0)
            continue;

        if ((caps & OBS_SOURCE_DEPRECATED) == 0) {
            return true;
        }
    }

    return false;
}

AFQCustomMenu* CMainSceneSource::_AddBackgroundColorMenu(AFQCustomMenu* menu,
                                                         QWidgetAction* widgetAction,
                                                         AFQColorSelect* select,
                                                         obs_sceneitem_t* item)
{
    QAction* action;

    menu->setStyleSheet(QString(
        "*[bgColor=\"1\"]{background-color:rgba(255,68,68,33%);}"
        "*[bgColor=\"2\"]{background-color:rgba(255,255,68,33%);}"
        "*[bgColor=\"3\"]{background-color:rgba(68,255,68,33%);}"
        "*[bgColor=\"4\"]{background-color:rgba(68,255,255,33%);}"
        "*[bgColor=\"5\"]{background-color:rgba(68,68,255,33%);}"
        "*[bgColor=\"6\"]{background-color:rgba(255,68,255,33%);}"
        "*[bgColor=\"7\"]{background-color:rgba(68,68,68,33%);}"
        "*[bgColor=\"8\"]{background-color:rgba(255,255,255,33%);}"));

    obs_data_t* privData = obs_sceneitem_get_private_settings(item);
    obs_data_release(privData);

    obs_data_set_default_int(privData, "color-preset", 0);
    int preset = obs_data_get_int(privData, "color-preset");

    action = menu->addAction(QTStr("Clear"), this,
                &CMainSceneSource::qslotSourceListItemColorChange);
    action->setCheckable(true);
    action->setProperty("bgColor", 0);
    action->setChecked(preset == 0);

    action = menu->addAction(QTStr("CustomColor"), this,
                &CMainSceneSource::qslotSourceListItemColorChange);
    action->setCheckable(true);
    action->setProperty("bgColor", 1);
    action->setChecked(preset == 1);

    menu->addSeparator();

    widgetAction->setDefaultWidget(select);

    for (int i = 1; i < 9; i++) {
        std::stringstream button;
        button << "preset" << i;

        std::stringstream buttonFrame;
        buttonFrame << "framePreset" << i;

        QFrame* colorButtonFrame =
            select->findChild<QFrame*>(buttonFrame.str().c_str());
        if (preset == i + 1)
            colorButtonFrame->setStyleSheet("QFrame { border: 1px solid #D9D9D9; border-radius:6px; }");

        QPushButton* colorButton =
            colorButtonFrame->findChild<QPushButton*>(button.str().c_str());

        colorButton->setProperty("bgColor", i);
        select->connect(colorButton, &QPushButton::released, this,
                        &CMainSceneSource::qslotSourceListItemColorChange);
    }
    menu->addAction(widgetAction);

    return menu;
}

AFQCustomMenu* CMainSceneSource::_AddScaleFilteringMenu(AFQCustomMenu* menu, obs_sceneitem_t* item)
{
    obs_scale_type scaleFilter = obs_sceneitem_get_scale_filter(item);
    QAction* action;

#define ADD_MODE(name, mode)                                        \
	action = menu->addAction(Str("" name), this,                    \
				 &CMainSceneSource::qslotSetScaleFilter);           \
	action->setProperty("mode", (int)mode);                         \
	action->setCheckable(true);                                     \
	action->setChecked(scaleFilter == mode);

    ADD_MODE("Disable", OBS_SCALE_DISABLE);
    ADD_MODE("ScaleFiltering.Point", OBS_SCALE_POINT);
    ADD_MODE("ScaleFiltering.Bilinear", OBS_SCALE_BILINEAR);
    ADD_MODE("ScaleFiltering.Bicubic", OBS_SCALE_BICUBIC);
    ADD_MODE("ScaleFiltering.Lanczos", OBS_SCALE_LANCZOS);
    ADD_MODE("ScaleFiltering.Area", OBS_SCALE_AREA);
#undef ADD_MODE

    return menu;
}

AFQCustomMenu* CMainSceneSource::_AddBlendingModeMenu(AFQCustomMenu* menu, obs_sceneitem_t* item)
{
    obs_blending_type blendingMode = obs_sceneitem_get_blending_mode(item);
    QAction* action;

#define ADD_MODE(name, mode)                                    \
	action = menu->addAction(Str("" name), this,                \
				 &CMainSceneSource::qslotBlendingMode);         \
	action->setProperty("mode", (int)mode);                     \
	action->setCheckable(true);                                 \
	action->setChecked(blendingMode == mode);

    ADD_MODE("BlendingMode.Normal", OBS_BLEND_NORMAL);
    ADD_MODE("BlendingMode.Additive", OBS_BLEND_ADDITIVE);
    ADD_MODE("BlendingMode.Subtract", OBS_BLEND_SUBTRACT);
    ADD_MODE("BlendingMode.Screen", OBS_BLEND_SCREEN);
    ADD_MODE("BlendingMode.Multiply", OBS_BLEND_MULTIPLY);
    ADD_MODE("BlendingMode.Lighten", OBS_BLEND_LIGHTEN);
    ADD_MODE("BlendingMode.Darken", OBS_BLEND_DARKEN);
#undef ADD_MODE

    return menu;
}

AFQCustomMenu* CMainSceneSource::_AddBlendingMethodMenu(AFQCustomMenu* menu, obs_sceneitem_t* item)
{
    obs_blending_method blendingMethod =
        obs_sceneitem_get_blending_method(item);
    QAction* action;

#define ADD_MODE(name, method)                                  \
	action = menu->addAction(Str("" name), this,                \
				 &CMainSceneSource::qslotBlendingMethod);       \
	action->setProperty("method", (int)method);                 \
	action->setCheckable(true);                                 \
	action->setChecked(blendingMethod == method);

    ADD_MODE("BlendingMethod.Default", OBS_BLEND_METHOD_DEFAULT);
    ADD_MODE("BlendingMethod.SrgbOff", OBS_BLEND_METHOD_SRGB_OFF);
#undef ADD_MODE

    return menu;
}

AFQCustomMenu* CMainSceneSource::_AddDeinterlacingMenu(AFQCustomMenu* menu, obs_source_t* source)
{
    obs_deinterlace_mode deinterlaceMode =
        obs_source_get_deinterlace_mode(source);
    obs_deinterlace_field_order deinterlaceOrder =
        obs_source_get_deinterlace_field_order(source);
    QAction* action;

#define ADD_MODE(name, mode)                                        \
	action = menu->addAction(Str("" name), this,                    \
				 &CMainSceneSource::qslotSetDeinterlaceingMode);    \
	action->setProperty("mode", (int)mode);                         \
	action->setCheckable(true);                                     \
	action->setChecked(deinterlaceMode == mode);

    ADD_MODE("Disable", OBS_DEINTERLACE_MODE_DISABLE);
    ADD_MODE("Deinterlacing.Discard", OBS_DEINTERLACE_MODE_DISCARD);
    ADD_MODE("Deinterlacing.Retro", OBS_DEINTERLACE_MODE_RETRO);
    ADD_MODE("Deinterlacing.Blend", OBS_DEINTERLACE_MODE_BLEND);
    ADD_MODE("Deinterlacing.Blend2x", OBS_DEINTERLACE_MODE_BLEND_2X);
    ADD_MODE("Deinterlacing.Linear", OBS_DEINTERLACE_MODE_LINEAR);
    ADD_MODE("Deinterlacing.Linear2x", OBS_DEINTERLACE_MODE_LINEAR_2X);
    ADD_MODE("Deinterlacing.Yadif", OBS_DEINTERLACE_MODE_YADIF);
    ADD_MODE("Deinterlacing.Yadif2x", OBS_DEINTERLACE_MODE_YADIF_2X);
#undef ADD_MODE

    menu->addSeparator();

#define ADD_ORDER(name, order)                                                  \
	action = menu->addAction(QTStr("Deinterlacing." name), this,                \
				 &CMainSceneSource::qslotSetDeinterlacingOrder);                \
	action->setProperty("order", (int)order);                                   \
	action->setCheckable(true);                                                 \
	action->setChecked(deinterlaceOrder == order);

    ADD_ORDER("TopFieldFirst", OBS_DEINTERLACE_FIELD_ORDER_TOP);
    ADD_ORDER("BottomFieldFirst", OBS_DEINTERLACE_FIELD_ORDER_BOTTOM);
#undef ADD_ORDER

    return menu;
}

QColor CMainSceneSource::_GetSourceListBackgroundColor(int preset)
{
    QColor color;
    switch (preset) {
    case 1: color.setRgb(255, 68, 68, 84); break;
    case 2: color.setRgb(255, 255, 68, 84); break;
    case 3: color.setRgb(68, 255, 68, 84); break;
    case 4: color.setRgb(68, 255, 255, 84); break;
    case 5: color.setRgb(68, 68, 255, 84); break;
    case 6: color.setRgb(255, 68, 255, 84); break;
    case 7: color.setRgb(68, 68, 68, 84); break;
    case 8: color.setRgb(255, 255, 255, 84); break;
    default:
        break;
    }

    return color;
}
//
void RemoveSceneAndReleaseNested(obs_source_t* source)
{
    obs_source_remove(source);
    auto cb = [](void*, obs_source_t* source) {
        if(strcmp(obs_source_get_id(source), "scene") == 0)
            obs_scene_prune_sources(obs_scene_from_source(source));
        return true;
    };
    obs_enum_scenes(cb, NULL);
}