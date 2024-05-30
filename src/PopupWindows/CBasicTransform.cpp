#include "CBasicTransform.h"
#include "ui_transform-dialog.h"

#include <QPushButton>

#include "qt-wrapper.h"

#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Scene/CSceneContext.h"

#include "Application/CApplication.h"
#include "MainFrame/CMainFrame.h"


static bool find_sel(obs_scene_t*, obs_sceneitem_t* item, void* param)
{
	OBSSceneItem& dst = *reinterpret_cast<OBSSceneItem*>(param);

	if (obs_sceneitem_selected(item)) {
		dst = item;
		return false;
	}
	if (obs_sceneitem_is_group(item)) {
		obs_sceneitem_group_enum_items(item, find_sel, param);
		if (!!dst) {
			return false;
		}
	}

	return true;
};

static OBSSceneItem FindASelectedItem(obs_scene_t* scene)
{
	OBSSceneItem item;
	obs_scene_enum_items(scene, find_sel, &item);
	return item;
}

#define COMBO_CHANGED &QComboBox::currentIndexChanged
#define ISCROLL_CHANGED &QSpinBox::valueChanged
#define DSCROLL_CHANGED &QDoubleSpinBox::valueChanged

static const uint32_t listToAlign[] = { OBS_ALIGN_TOP | OBS_ALIGN_LEFT,
					   OBS_ALIGN_TOP,
					   OBS_ALIGN_TOP | OBS_ALIGN_RIGHT,
					   OBS_ALIGN_LEFT,
					   OBS_ALIGN_CENTER,
					   OBS_ALIGN_RIGHT,
					   OBS_ALIGN_BOTTOM | OBS_ALIGN_LEFT,
					   OBS_ALIGN_BOTTOM,
					   OBS_ALIGN_BOTTOM | OBS_ALIGN_RIGHT };

static int AlignToList(uint32_t align)
{
	int index = 0;
	for (uint32_t curAlign : listToAlign) {
		if (curAlign == align)
			return index;

		index++;
	}

	return 0;
}

AFQBasicTransform::AFQBasicTransform(OBSSceneItem item, QWidget* parent) :
	AFQRoundedDialogBase(parent, Qt::WindowFlags(), false),
	ui(new Ui::AFQBasicTransform)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	QPushButton* resetButton = ui->buttonBox->button(QDialogButtonBox::Reset);
	ChangeStyleSheet(resetButton, STYLESHEET_RESET_BUTTON);

	HookWidget(ui->positionX, DSCROLL_CHANGED,
				&AFQBasicTransform::OnControlChanged);
	HookWidget(ui->positionY, DSCROLL_CHANGED,
				&AFQBasicTransform::OnControlChanged);
	HookWidget(ui->rotation, DSCROLL_CHANGED,
				&AFQBasicTransform::OnControlChanged);
	HookWidget(ui->sizeX, DSCROLL_CHANGED,
				&AFQBasicTransform::OnControlChanged);
	HookWidget(ui->sizeY, DSCROLL_CHANGED,
				&AFQBasicTransform::OnControlChanged);
	HookWidget(ui->align, COMBO_CHANGED,
				&AFQBasicTransform::OnControlChanged);
	HookWidget(ui->boundsType, COMBO_CHANGED,
				&AFQBasicTransform::OnBoundsType);
	HookWidget(ui->boundsAlign, COMBO_CHANGED,
				&AFQBasicTransform::OnControlChanged);
	HookWidget(ui->boundsWidth, DSCROLL_CHANGED,
				&AFQBasicTransform::OnControlChanged);
	HookWidget(ui->boundsHeight, DSCROLL_CHANGED,
				&AFQBasicTransform::OnControlChanged);
	HookWidget(ui->cropLeft, ISCROLL_CHANGED,
				&AFQBasicTransform::OnCropChanged);
	HookWidget(ui->cropRight, ISCROLL_CHANGED,
				&AFQBasicTransform::OnCropChanged);
	HookWidget(ui->cropTop, ISCROLL_CHANGED,
				&AFQBasicTransform::OnCropChanged);
	HookWidget(ui->cropBottom, ISCROLL_CHANGED,
				&AFQBasicTransform::OnCropChanged);

	connect(ui->closeButton, &QPushButton::clicked,
			this, &AFQBasicTransform::qslotCloseButtonClicked);

	AFMainFrame* main = App()->GetMainView();
	connect(ui->buttonBox->button(QDialogButtonBox::Reset),
			&QPushButton::clicked, main,
			&AFMainFrame::qSlotActionResetTransform);

	installEventFilter(CreateShortcutFilter());

	OBSScene scene = obs_sceneitem_get_scene(item);
	SetScene(scene);
	SetItem(item);

	std::string name = obs_source_get_name(obs_sceneitem_get_source(item));
	setWindowTitle(QTStr("Basic.TransformWindow.Title").arg(name.c_str()));

	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(AFSourceUtil::GetCurrentScene(), false);
	undo_data = std::string(obs_data_get_json(wrapper));

	m_signalChannelChanged.Connect(obs_get_signal_handler(), "channel_change", AFChannelChanged, this);
    
	this->SetHeightFixed(true);
}

AFQBasicTransform::~AFQBasicTransform()
{
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(AFSourceUtil::GetCurrentScene(), false);

	auto undo_redo = [](const std::string& data) {
		OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
		OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(dat, "scene_uuid"));
		AFMainFrame* main = App()->GetMainView();
		main->GetMainWindow()->SetCurrentScene(source.Get(), true);
		obs_scene_load_transform_states(data.c_str());
	};

	std::string redo_data(obs_data_get_json(wrapper));
	if (undo_data.compare(redo_data) != 0)
	{
		AFMainFrame* main = App()->GetMainView();
		main->m_undo_s.AddAction(QTStr("Undo.Transform").arg(obs_source_get_name(AFSourceUtil::GetCurrentSource())),
								 undo_redo, undo_redo, undo_data, redo_data);
	}
}

void AFQBasicTransform::OnBoundsType(int index)
{
	if (index == -1)
		return;

	obs_bounds_type type = (obs_bounds_type)index;
	bool enable = (type != OBS_BOUNDS_NONE);

	ui->boundsAlign->setEnabled(enable);
	ui->boundsWidth->setEnabled(enable);
	ui->boundsHeight->setEnabled(enable);

	if (!m_ignoreItemChange) {
		obs_bounds_type lastType = obs_sceneitem_get_bounds_type(m_sceneItem);
		if (lastType == OBS_BOUNDS_NONE) {
			OBSSource source = obs_sceneitem_get_source(m_sceneItem);
			int width = (int)obs_source_get_width(source);
			int height = (int)obs_source_get_height(source);

			ui->boundsWidth->setValue(width);
			ui->boundsHeight->setValue(height);
		}
	}

	OnControlChanged();
}

void AFQBasicTransform::OnControlChanged()
{
	if (m_ignoreItemChange)
		return;

	obs_source_t* source = obs_sceneitem_get_source(m_sceneItem);
	uint32_t source_cx = obs_source_get_width(source);
	uint32_t source_cy = obs_source_get_height(source);
	double width = double(source_cx);
	double height = double(source_cy);

	obs_transform_info oti;
	obs_sceneitem_get_info(m_sceneItem, &oti);

	/* do not scale a source if it has 0 width/height */
	if (source_cx != 0 && source_cy != 0) {
		oti.scale.x = float(ui->sizeX->value() / width);
		oti.scale.y = float(ui->sizeY->value() / height);
	}

	oti.pos.x = float(ui->positionX->value());
	oti.pos.y = float(ui->positionY->value());
	oti.rot = float(ui->rotation->value());
	oti.alignment = listToAlign[ui->align->currentIndex()];

	oti.bounds_type = (obs_bounds_type)ui->boundsType->currentIndex();
	oti.bounds_alignment = listToAlign[ui->boundsAlign->currentIndex()];
	oti.bounds.x = float(ui->boundsWidth->value());
	oti.bounds.y = float(ui->boundsHeight->value());

	m_ignoreTransformSignal = true;
	obs_sceneitem_set_info(m_sceneItem, &oti);
	m_ignoreTransformSignal = false;

}

void AFQBasicTransform::OnCropChanged()
{
	if (m_ignoreItemChange)
		return;

	obs_sceneitem_crop crop;
	crop.left = uint32_t(ui->cropLeft->value());
	crop.right = uint32_t(ui->cropRight->value());
	crop.top = uint32_t(ui->cropTop->value());
	crop.bottom = uint32_t(ui->cropBottom->value());

	m_ignoreTransformSignal = true;
	obs_sceneitem_set_crop(m_sceneItem, &crop);
	m_ignoreTransformSignal = false;
}

void AFQBasicTransform::AFChannelChanged(void* param, calldata_t* data)
{
	AFQBasicTransform* window =
		reinterpret_cast<AFQBasicTransform*>(param);
	uint32_t channel = (uint32_t)calldata_int(data, "channel");
	OBSSource source = (obs_source_t*)calldata_ptr(data, "source");

	if (channel == 0) {
		OBSScene scene = obs_scene_from_source(source);
		window->SetScene(scene);

		if (!scene)
			window->SetItem(nullptr);
		else
			window->SetItem(FindASelectedItem(scene));
	}
}

void AFQBasicTransform::AFSceneItemTransform(void* param, calldata_t* data)
{
	AFQBasicTransform* window =
		reinterpret_cast<AFQBasicTransform*>(param);
	OBSSceneItem item = (obs_sceneitem_t*)calldata_ptr(data, "item");

	if (item == window->m_sceneItem && !window->m_ignoreTransformSignal)
		QMetaObject::invokeMethod(window, "RefreshControls");
}

void AFQBasicTransform::AFSceneItemRemoved(void* param, calldata_t* data)
{
	AFQBasicTransform* window =
		reinterpret_cast<AFQBasicTransform*>(param);
	obs_scene_t* scene = (obs_scene_t*)calldata_ptr(data, "scene");
	obs_sceneitem_t* item = (obs_sceneitem_t*)calldata_ptr(data, "item");

	if (item == window->m_sceneItem)
		window->SetItem(FindASelectedItem(scene));
}

void AFQBasicTransform::AFSceneItemSelect(void* param, calldata_t* data)
{
	AFQBasicTransform* window =
		reinterpret_cast<AFQBasicTransform*>(param);
	OBSSceneItem item = (obs_sceneitem_t*)calldata_ptr(data, "item");

	if (item != window->m_sceneItem)
		window->SetItem(item);
}

void AFQBasicTransform::AFSceneItemDeselect(void* param, calldata_t* data)
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

	AFQBasicTransform* window =
		reinterpret_cast<AFQBasicTransform*>(param);
	obs_scene_t* scene = (obs_scene_t*)calldata_ptr(data, "scene");
	obs_sceneitem_t* item = (obs_sceneitem_t*)calldata_ptr(data, "item");

	if (item == window->m_sceneItem) {
		window->setWindowTitle(
			locale.Str("Basic.TransformWindow.NoSelectedSource"));
		window->SetItem(FindASelectedItem(scene));
	}
}

void AFQBasicTransform::RefreshControls()
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

	if (!m_sceneItem)
		return;

	obs_transform_info osi;
	obs_sceneitem_crop crop;
	obs_sceneitem_get_info(m_sceneItem, &osi);
	obs_sceneitem_get_crop(m_sceneItem, &crop);

	obs_source_t* source = obs_sceneitem_get_source(m_sceneItem);
	uint32_t source_cx = obs_source_get_width(source);
	uint32_t source_cy = obs_source_get_height(source);
	float width = float(source_cx);
	float height = float(source_cy);

	int alignIndex = AlignToList(osi.alignment);
	int boundsAlignIndex = AlignToList(osi.bounds_alignment);

	m_ignoreItemChange = true;
	ui->positionX->setValue(osi.pos.x);
	ui->positionY->setValue(osi.pos.y);
	ui->rotation->setValue(osi.rot);
	ui->sizeX->setValue(osi.scale.x * width);
	ui->sizeY->setValue(osi.scale.y * height);
	ui->align->setCurrentIndex(alignIndex);

	bool valid_size = source_cx != 0 && source_cy != 0;
	ui->sizeX->setEnabled(valid_size);
	ui->sizeY->setEnabled(valid_size);

	ui->boundsType->setCurrentIndex(int(osi.bounds_type));
	ui->boundsAlign->setCurrentIndex(boundsAlignIndex);
	ui->boundsWidth->setValue(osi.bounds.x);
	ui->boundsHeight->setValue(osi.bounds.y);

	ui->cropLeft->setValue(int(crop.left));
	ui->cropRight->setValue(int(crop.right));
	ui->cropTop->setValue(int(crop.top));
	ui->cropBottom->setValue(int(crop.bottom));
	m_ignoreItemChange = false;

	std::string name = obs_source_get_name(source);
	QString title = QString(locale.Str("Basic.TransformWindow.Title"));
	title = title.arg(name.c_str());

	ui->labelTitle->setText(title);
	setWindowTitle(title);
}

void AFQBasicTransform::SetItemQt(OBSSceneItem newItem)
{
	m_sceneItem = newItem;
	if (m_sceneItem)
		RefreshControls();

	bool enable = !!m_sceneItem;
	ui->container->setEnabled(enable);
	ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(enable);
}

void AFQBasicTransform::qslotCloseButtonClicked()
{
	close();
}

void AFQBasicTransform::SetScene(OBSScene scene)
{
	m_signalTransform.Disconnect();

	if (scene)
	{
		OBSSource source = obs_scene_get_source(scene);
		signal_handler_t* signal =
			obs_source_get_signal_handler(source);

		m_signalTransform.Connect(signal, "item_transform", AFSceneItemTransform, this);
		m_signalRemove.Connect(signal, "item_remove", AFSceneItemRemoved, this);
		m_signalSelect.Connect(signal, "item_select", AFSceneItemSelect, this);
		m_signalDeselect.Connect(signal, "item_deselect",AFSceneItemDeselect, this);
	}
}

void AFQBasicTransform::SetItem(OBSSceneItem newItem)
{
	QMetaObject::invokeMethod(this, "SetItemQt",
			Q_ARG(OBSSceneItem, OBSSceneItem(newItem)));
}