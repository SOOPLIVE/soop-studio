#include "CBasicFilters.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#endif

#include <QPushButton>
#include <QVariant>

#include "qt-wrapper.h"

#include "Common/MathMiscUtils.h"

#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Scene/CSceneContext.h"

#include "UIComponent/CQtDisplay.h"
#include "UIComponent/CNameDialog.h"
#include "UIComponent/CItemWidgetHelper.h"
#include "UIComponent/CVisibilityItemWidget.h"
#include "UIComponent/CMessageBox.h"

#include "PopupWindows/SourceDialog/CPropertiesView.h"
#include "UIComponent/CCustomMenu.h"

#include "Application/CApplication.h"
#include "MainFrame/CMainFrame.h"

using namespace std;

Q_DECLARE_METATYPE(OBSSource);


inline void FilterChangeUndoRedo(void* vp, obs_data_t* nd_old_settings, obs_data_t* new_settings)
{
	obs_source_t* source = reinterpret_cast<obs_source_t*>(vp);
	const char* source_uuid = obs_source_get_uuid(source);
	const char* name = obs_source_get_name(source);

	AFMainFrame* main = App()->GetMainView();

	OBSDataAutoRelease redo_wrapper = obs_data_create();
	obs_data_set_string(redo_wrapper, "uuid", source_uuid);
	obs_data_set_string(redo_wrapper, "settings", obs_data_get_json(new_settings));

	OBSDataAutoRelease undo_wrapper = obs_data_create();
	obs_data_set_string(undo_wrapper, "uuid", source_uuid);
	obs_data_set_string(undo_wrapper, "settings", obs_data_get_json(nd_old_settings));

	auto undo_redo = [](const std::string& data)
	{
		OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
		const char* filter_uuid = obs_data_get_string(dat, "uuid");
		OBSSourceAutoRelease filter = obs_get_source_by_uuid(filter_uuid);
		OBSDataAutoRelease new_settings = obs_data_create_from_json(obs_data_get_string(dat, "settings"));

		OBSDataAutoRelease current_settings = obs_source_get_settings(filter);
		obs_data_clear(current_settings);

		obs_source_update(filter, new_settings);
		obs_source_update_properties(filter);
	};

	main->m_undo_s.Enable();

	std::string undo_data = obs_data_get_json(undo_wrapper);
	std::string redo_data = obs_data_get_json(redo_wrapper);
	main->m_undo_s.AddAction(QTStr("Undo.Filters").arg(name),
							 undo_redo, undo_redo, undo_data, redo_data);
	obs_source_update(source, new_settings);
}

AFQBasicFilters::AFQBasicFilters(QWidget* parent, OBSSource source) :
	AFQRoundedDialogBase(parent, Qt::WindowFlags(), false),
	ui(new Ui::AFQBasicFilters),
	m_obsSource(source),
	m_signalAdd(obs_source_get_signal_handler(source), "filter_add",
				AFQBasicFilters::OBSSourceFilterAdded, this),
	m_signalRemove(obs_source_get_signal_handler(source), "filter_remove",
				   AFQBasicFilters::OBSSourceFilterRemoved, this),
	m_signalReorder(obs_source_get_signal_handler(source),
					"reorder_filters", AFQBasicFilters::OBSSourceReordered,
					this),
	m_signalRemoveSource(obs_source_get_signal_handler(source), "remove",
						 AFQBasicFilters::SourceRemoved, this),
	m_signalRenameSource(obs_source_get_signal_handler(source), "rename",
						 AFQBasicFilters::SourceRenamed, this)
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	this->SetHeightFixed(true);
	this->SetWidthFixed(true);

	ui->setupUi(this);

	QPushButton* resetButton = ui->buttonBox->button(QDialogButtonBox::RestoreDefaults);
	ChangeStyleSheet(resetButton, STYLESHEET_RESET_BUTTON);

	ui->asyncFilters->setItemDelegate(
			new VisibilityItemDelegate(ui->asyncFilters));
	ui->effectFilters->setItemDelegate(
			new VisibilityItemDelegate(ui->effectFilters));
	
	QString title = locale.Str("Basic.Filters.Title");
	const char* name = obs_source_get_name(source);

	installEventFilter(CreateShortcutFilter());

	RenameFiltersTitle(title.arg(QT_UTF8(name)));

	uint32_t caps = obs_source_get_output_flags(source);
	bool audio = (caps & OBS_SOURCE_AUDIO) != 0;
	bool audioOnly = (caps & OBS_SOURCE_VIDEO) == 0;
	bool async = (caps & OBS_SOURCE_ASYNC) != 0;

	if (!async && !audio) {
		ui->asyncWidget->setVisible(false);
	}
	if (audioOnly) {
		ui->asyncWidget->setStyleSheet("#asyncWidget {border-bottom: none;}");
		ui->effectWidget->setVisible(false);
		UpdateSplitter(false);
	}

	if (async && !audioOnly && ui->asyncFilters->count() == 0 &&
		ui->effectFilters->count() != 0) {
		ui->effectFilters->setFocus();
	}

	if (audioOnly || (audio && !async))
		ui->asyncLabel->setText(locale.Str("Basic.Filters.AudioFilters"));

	if (async && audio && ui->asyncFilters->count() == 0) {
		UpdateSplitter(false);
	}
	else if (!audioOnly) {
		UpdateSplitter();
	}

	obs_source_inc_showing(m_obsSource);

	auto addDrawCallback = [this]() {
		obs_display_add_draw_callback(ui->preview->GetDisplay(),
									  AFQBasicFilters::DrawPreview,
									  this);
	};

	enum obs_source_type type = obs_source_get_type(source);
	bool drawable_type = type == OBS_SOURCE_TYPE_INPUT ||
		type == OBS_SOURCE_TYPE_SCENE;

	if ((caps & OBS_SOURCE_VIDEO) != 0) {
		ui->previewFrame->show();
		//ui->preview->show();
		if (drawable_type)
			connect(ui->preview, &AFQTDisplay::qsignalDisplayCreated,
					addDrawCallback);
	}
	else {
		ui->previewFrame->hide();
		//ui->preview->hide();
	}

	SetSignalSlotUI();

	UpdateFilters();
}

AFQBasicFilters::~AFQBasicFilters()
{
	obs_source_dec_showing(m_obsSource);
	ClearListItems(ui->effectFilters);
	ClearListItems(ui->asyncFilters);
}

static bool ConfirmReset(QWidget* parent)
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

	int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
		parent,
		locale.Str("ConfirmReset.Title"),
		locale.Str("ConfirmReset.Text"));

	return (result == QDialog::Accepted);
}

void AFQBasicFilters::AddFilter(OBSSource filter, bool focus)
{
	uint32_t flags = obs_source_get_output_flags(filter);
	bool async = (flags & OBS_SOURCE_ASYNC) != 0;
	QListWidget* list = async ? ui->asyncFilters : ui->effectFilters;

	QListWidgetItem* item = new QListWidgetItem();
	Qt::ItemFlags itemFlags = item->flags();

	item->setFlags(itemFlags | Qt::ItemIsEditable);
	item->setData(Qt::UserRole, QVariant::fromValue(filter));

	list->addItem(item);
	if (focus)
		list->setCurrentItem(item);

	SetupVisibilityItem(list, item, filter);

	UpdateItemFontColor(list->currentRow(), async);
}

struct FilterOrderInfo {
	int asyncIdx = 0;
	int effectIdx = 0;
	AFQBasicFilters* window;

	inline FilterOrderInfo(AFQBasicFilters* window_) : window(window_) {}
};

void AFQBasicFilters::ReorderFilters()
{
	FilterOrderInfo info(this);

	obs_source_enum_filters(
		m_obsSource,
		[](obs_source_t*, obs_source_t* filter, void* p) {
			FilterOrderInfo* info =
				reinterpret_cast<FilterOrderInfo*>(p);
			uint32_t flags;
			bool async;

			flags = obs_source_get_output_flags(filter);
			async = (flags & OBS_SOURCE_ASYNC) != 0;

			if (async) {
				info->window->ReorderFilter(
					info->window->ui->asyncFilters, filter,
					info->asyncIdx++);
			}
			else {
				info->window->ReorderFilter(
					info->window->ui->effectFilters, filter,
					info->effectIdx++);
			}
		},
		&info);
}

void AFQBasicFilters::RenameAsyncFilter()
{
	EditItem(ui->asyncFilters->currentItem(), true);
}

void AFQBasicFilters::RenameEffectFilter()
{
	EditItem(ui->effectFilters->currentItem(), false);
}

void AFQBasicFilters::ResetFilters()
{
	QListWidget* list = m_isAsync ? ui->asyncFilters : ui->effectFilters;
	int row = list->currentRow();

	OBSSource filter = GetFilter(row, m_isAsync);

	if (!filter)
		return;

	if (!ConfirmReset(this))
		return;

	OBSDataAutoRelease settings = obs_source_get_settings(filter);

	OBSDataAutoRelease empty_settings = obs_data_create();
	FilterChangeUndoRedo((void*)filter, settings, empty_settings);

	obs_data_clear(settings);

	if (!m_propsView->DeferUpdate())
		obs_source_update(filter, nullptr);

	m_propsView->ReloadProperties();
}

void AFQBasicFilters::RenameFiltersTitle(QString title)
{
	setWindowTitle(title);

	ui->labelTitle->setText(title);
}

static bool QueryRemove(QWidget* parent, obs_source_t* source)
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

	const char* name = obs_source_get_name(source);

	QString text = locale.Str("ConfirmRemove.Text");
	text = text.arg(QT_UTF8(name));

	int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
											parent,
											locale.Str("ConfirmRemove.Title"),
											text);

	return (result == QDialog::Accepted);
}

void AFQBasicFilters::qSlotAddAsyncFilterClicked()
{
	ui->asyncFilters->setFocus();
	QScopedPointer<AFQCustomMenu> popup(CreateAddFilterPopupMenu(true));
	if (popup)
		popup->exec(QCursor::pos());
}

void AFQBasicFilters::qSlotRemoveAsyncFilterClicked()
{
	OBSSource filter = GetFilter(ui->asyncFilters->currentRow(), true);
	if (filter) {
		if (QueryRemove(this, filter))
			delete_filter(filter);
	}
}

void AFQBasicFilters::qSlotMoveUpAsyncFilterClicked()
{
	OBSSource filter = GetFilter(ui->asyncFilters->currentRow(), true);
	if (filter)
		obs_source_filter_set_order(m_obsSource, filter, OBS_ORDER_MOVE_UP);

}

void AFQBasicFilters::qSlotMoveDownAsyncFilterClicked()
{
	OBSSource filter = GetFilter(ui->asyncFilters->currentRow(), true);
	if (filter)
		obs_source_filter_set_order(m_obsSource, filter,
									OBS_ORDER_MOVE_DOWN);
}

void AFQBasicFilters::qSlotAsyncFiltersCurrentRowChanged(int row)
{
	UpdateItemFontColor(row, true);
	UpdatePropertiesView(row, true);
}

void AFQBasicFilters::qSlotAsyncFiltersGotFocus()
{
	UpdateItemFontColor(ui->asyncFilters->currentRow(), true);
	UpdatePropertiesView(ui->asyncFilters->currentRow(), true);
	m_isAsync = true;
}

void AFQBasicFilters::qSlotAsyncFiltersCustomContextMenuRequested(const QPoint& pos)
{
	CustomContextMenu(pos, true);
}

void AFQBasicFilters::qSlotAddEffectFilterClicked()
{
	ui->asyncFilters->setFocus();
	QScopedPointer<AFQCustomMenu> popup(CreateAddFilterPopupMenu(false));
	if (popup)
		popup->exec(QCursor::pos());
}

void AFQBasicFilters::qSlotRemoveEffectFilterClicked()
{
	OBSSource filter = GetFilter(ui->effectFilters->currentRow(), false);
	if (filter) {
		if (QueryRemove(this, filter)) {
			delete_filter(filter);
		}
	}
}

void AFQBasicFilters::qSlotMoveUpEffectFilterClicked()
{
	OBSSource filter = GetFilter(ui->effectFilters->currentRow(), false);
	if (filter)
		obs_source_filter_set_order(m_obsSource, filter, OBS_ORDER_MOVE_UP);
}

void AFQBasicFilters::qSlotMoveDownEffectFilterClicked()
{
	OBSSource filter = GetFilter(ui->effectFilters->currentRow(), false);
	if (filter)
		obs_source_filter_set_order(m_obsSource, filter,
									OBS_ORDER_MOVE_DOWN);
}

void AFQBasicFilters::qSlotEffectFiltersCurrentRowChanged(int row)
{
	UpdateItemFontColor(row, false);
	UpdatePropertiesView(row, false);
}

void AFQBasicFilters::qSlotEffectFiltersGotFocus()
{
	UpdateItemFontColor(ui->effectFilters->currentRow(), false);
	UpdatePropertiesView(ui->effectFilters->currentRow(), false);
	m_isAsync = false;
}

void AFQBasicFilters::qSlotEffectFiltersCustomContextMenuRequested(const QPoint& pos)
{
	CustomContextMenu(pos, false);
}

void AFQBasicFilters::qSlotRenameFilterTriggered()
{
	if (ui->asyncFilters->hasFocus())
		RenameAsyncFilter();
	else if (ui->effectFilters->hasFocus())
		RenameEffectFilter();
}

void AFQBasicFilters::qSlotRemoveFilterTriggered()
{
	if (ui->asyncFilters->hasFocus())
			qSlotRemoveAsyncFilterClicked();
	else if (ui->effectFilters->hasFocus())
			qSlotRemoveEffectFilterClicked();
}

void AFQBasicFilters::qSlotMoveUpFilterTriggered()
{
	if (ui->asyncFilters->hasFocus())
			qSlotMoveUpAsyncFilterClicked();
	else if (ui->effectFilters->hasFocus())
			qSlotMoveUpEffectFilterClicked();
}

void AFQBasicFilters::qSlotMoveDownFilterTriggered()
{
	if (ui->asyncFilters->hasFocus())
			qSlotMoveDownAsyncFilterClicked();
	else if (ui->effectFilters->hasFocus())
			qSlotMoveDownEffectFilterClicked();
}

void AFQBasicFilters::FiltersMoved(const QModelIndex&, int srcIdxStart, int,
	const QModelIndex&, int)
{
	QListWidget* list = m_isAsync ? ui->asyncFilters : ui->effectFilters;
	int neighborIdx = 0;

	if (srcIdxStart < list->currentRow())
		neighborIdx = list->currentRow() - 1;
	else if (srcIdxStart > list->currentRow())
		neighborIdx = list->currentRow() + 1;
	else
		return;

	if (neighborIdx > list->count() - 1)
		neighborIdx = list->count() - 1;
	else if (neighborIdx < 0)
		neighborIdx = 0;

	OBSSource neighbor = GetFilter(neighborIdx, m_isAsync);
	int idx = obs_source_filter_get_index(m_obsSource, neighbor);

	OBSSource filter = GetFilter(list->currentRow(), m_isAsync);
	obs_source_filter_set_index(m_obsSource, filter, idx);
}

void AFQBasicFilters::CopyFilter()
{
	OBSSource filter = nullptr;

	if (m_isAsync)
		filter = GetFilter(ui->asyncFilters->currentRow(), true);
	else
		filter = GetFilter(ui->effectFilters->currentRow(), false);

	AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

	sceneContext.m_copyFilter = OBSGetWeakRef(filter);
}

void AFQBasicFilters::PasteFilter()
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();
	AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

	OBSSource filter = OBSGetStrongRef(sceneContext.m_copyFilter);
	if (!filter)
		return;

	OBSDataArrayAutoRelease undo_array = obs_source_backup_filters(m_obsSource);
	obs_source_copy_single_filter(m_obsSource, filter);
	OBSDataArrayAutoRelease redo_array = obs_source_backup_filters(m_obsSource);

	const char* filterName = obs_source_get_name(filter);
	const char* sourceName = obs_source_get_name(m_obsSource);
	QString text = locale.Str("Undo.Filters.Paste.Single");
	text = text.arg(filterName, sourceName);

	AFMainFrame* main = App()->GetMainView();
	main->CreateFilterPasteUndoRedoAction(text, m_obsSource, undo_array, redo_array);
}

void AFQBasicFilters::closeEvent(QCloseEvent* event)
{
	QDialog::closeEvent(event);
	if (!event->isAccepted())
		return;

	obs_display_remove_draw_callback(ui->preview->GetDisplay(),
									 AFQBasicFilters::DrawPreview, this);

	App()->GetMainView()->UpdateEditMenu();

	//main->SaveProject();
}

void AFQBasicFilters::SetSignalSlotUI()
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

	m_actionRenameFilter = new QAction(locale.Str("Rename"), this);
	m_actionRenameFilter->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	m_actionRenameFilter->setShortcut({ Qt::Key_F2 });
	connect(m_actionRenameFilter, &QAction::triggered,
			this, &AFQBasicFilters::qSlotRenameFilterTriggered);

	m_actionRemoveFilter = new QAction(locale.Str("Remove"), this);
	m_actionRemoveFilter->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	m_actionRemoveFilter->setShortcut({ Qt::Key_Delete });
	connect(m_actionRemoveFilter, &QAction::triggered,
			this, &AFQBasicFilters::qSlotRemoveFilterTriggered);

	m_actionMoveUp = new QAction(locale.Str("Basic.MainMenu.Edit.Order.MoveUp"), this);
	m_actionMoveUp->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	m_actionMoveUp->setShortcut(QKeySequence("Ctrl+Up"));
	connect(m_actionMoveUp, &QAction::triggered,
			this, &AFQBasicFilters::qSlotMoveUpFilterTriggered);

	m_actionMoveDown = new QAction(locale.Str("Basic.MainMenu.Edit.Order.MoveDown"), this);
	m_actionMoveDown->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	m_actionMoveDown->setShortcut(QKeySequence("Ctrl+Down"));
	connect(m_actionMoveDown, &QAction::triggered,
			this, &AFQBasicFilters::qSlotMoveDownFilterTriggered);

	addAction(m_actionRenameFilter);
	addAction(m_actionRemoveFilter);
	addAction(m_actionMoveUp);
	addAction(m_actionMoveDown);

	connect(ui->asyncFilters->itemDelegate(),
			&QAbstractItemDelegate::closeEditor, [this](QWidget* editor) {
			FilterNameEdited(editor, ui->asyncFilters);
		});

	connect(ui->effectFilters->itemDelegate(),
			&QAbstractItemDelegate::closeEditor, [this](QWidget* editor) {
			FilterNameEdited(editor, ui->effectFilters);
		});

	QPushButton* close = ui->buttonBox->button(QDialogButtonBox::Close);
	connect(close, &QPushButton::clicked, this, &AFQBasicFilters::close);
	close->setDefault(true);

	connect(ui->buttonBox->button(QDialogButtonBox::RestoreDefaults),
		&QPushButton::clicked, this, &AFQBasicFilters::ResetFilters);

	connect(ui->asyncFilters->model(), &QAbstractItemModel::rowsMoved, 
			this, &AFQBasicFilters::FiltersMoved);
	connect(ui->effectFilters->model(), &QAbstractItemModel::rowsMoved,
			this, &AFQBasicFilters::FiltersMoved);

	connect(ui->closeButton, &QPushButton::clicked, this, &AFQBasicFilters::close);
	
	connect(ui->addAsyncFilter, &QPushButton::clicked,
			this, &AFQBasicFilters::qSlotAddAsyncFilterClicked);

	connect(ui->removeAsyncFilter, &QPushButton::clicked,
			this, &AFQBasicFilters::qSlotRemoveAsyncFilterClicked);

	connect(ui->moveAsyncFilterUp, &QPushButton::clicked,
			this, &AFQBasicFilters::qSlotMoveUpAsyncFilterClicked);

	connect(ui->moveAsyncFilterDown, &QPushButton::clicked,
			this, &AFQBasicFilters::qSlotMoveDownAsyncFilterClicked);

	connect(ui->asyncFilters, &AFQFocusList::GotFocus,
			this, &AFQBasicFilters::qSlotAsyncFiltersGotFocus);

	connect(ui->asyncFilters, &AFQFocusList::currentRowChanged,
			this, &AFQBasicFilters::qSlotAsyncFiltersCurrentRowChanged);

	connect(ui->asyncFilters, &AFQFocusList::customContextMenuRequested,
			this, &AFQBasicFilters::qSlotAsyncFiltersCustomContextMenuRequested);

	connect(ui->addEffectFilter, &QPushButton::clicked,
			this, &AFQBasicFilters::qSlotAddEffectFilterClicked);

	connect(ui->removeEffectFilter, &QPushButton::clicked,
			this, &AFQBasicFilters::qSlotRemoveEffectFilterClicked);

	connect(ui->moveEffectFilterUp, &QPushButton::clicked,
			this, &AFQBasicFilters::qSlotMoveUpEffectFilterClicked);

	connect(ui->moveEffectFilterDown, &QPushButton::clicked,
			this, &AFQBasicFilters::qSlotMoveDownEffectFilterClicked);

	connect(ui->effectFilters, &AFQFocusList::GotFocus, 
			this, &AFQBasicFilters::qSlotEffectFiltersGotFocus);

	connect(ui->effectFilters, &AFQFocusList::currentRowChanged,
			this, &AFQBasicFilters::qSlotEffectFiltersCurrentRowChanged);

	connect(ui->effectFilters, &AFQFocusList::customContextMenuRequested,
		this, &AFQBasicFilters::qSlotEffectFiltersCustomContextMenuRequested);

}

static bool filter_compatible(bool async, uint32_t sourceFlags,
	uint32_t filterFlags)
{
	bool filterVideo = (filterFlags & OBS_SOURCE_VIDEO) != 0;
	bool filterAsync = (filterFlags & OBS_SOURCE_ASYNC) != 0;
	bool filterAudio = (filterFlags & OBS_SOURCE_AUDIO) != 0;
	bool audio = (sourceFlags & OBS_SOURCE_AUDIO) != 0;
	bool audioOnly = (sourceFlags & OBS_SOURCE_VIDEO) == 0;
	bool asyncSource = (sourceFlags & OBS_SOURCE_ASYNC) != 0;

	if (async &&
		((audioOnly && filterVideo) || (!audio && !asyncSource) ||
			(filterAudio && !audio) || (!asyncSource && !filterAudio)))
		return false;

	return (async && (filterAudio || filterAsync)) ||
		(!async && !filterAudio && !filterAsync);
}

AFQCustomMenu* AFQBasicFilters::CreateAddFilterPopupMenu(bool async)
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

	uint32_t sourceFlags = obs_source_get_output_flags(m_obsSource);
	const char* type_str;
	bool foundValues = false;
	size_t idx = 0;

	struct FilterInfo {
		string type;
		string name;

		inline FilterInfo(const char* type_, const char* name_)
			: type(type_),
			name(name_)
		{
		}

		bool operator<(const FilterInfo& r) const
		{
			return name < r.name;
		}
	};

	vector<FilterInfo> types;
	while (obs_enum_filter_types(idx++, &type_str)) {
		const char* name = obs_source_get_display_name(type_str);
		uint32_t caps = obs_get_source_output_flags(type_str);

		if ((caps & OBS_SOURCE_DEPRECATED) != 0)
			continue;
		if ((caps & OBS_SOURCE_CAP_DISABLED) != 0)
			continue;
		if ((caps & OBS_SOURCE_CAP_OBSOLETE) != 0)
			continue;

		types.emplace_back(type_str, name);
	}

	sort(types.begin(), types.end());

	AFQCustomMenu* popup = new AFQCustomMenu(locale.Str("Add"), this, true);
	for (FilterInfo& type : types) {
		uint32_t filterFlags =
			obs_get_source_output_flags(type.type.c_str());

		if (!filter_compatible(async, sourceFlags, filterFlags))
			continue;

		QAction* popupItem =
			new QAction(QT_UTF8(type.name.c_str()), this);
		popupItem->setData(QT_UTF8(type.type.c_str()));
		connect(popupItem, &QAction::triggered,
			[this, type]() { AddNewFilter(type.type.c_str()); });
		popup->addAction(popupItem);

		foundValues = true;
	}

	if (!foundValues) {
		delete popup;
		popup = nullptr;
	}

	return popup;
}

inline OBSSource AFQBasicFilters::GetFilter(int row, bool async)
{
	if (row == -1)
		return OBSSource();

	QListWidget* list = async ? ui->asyncFilters : ui->effectFilters;
	QListWidgetItem* item = list->item(row);
	if (!item)
		return OBSSource();

	QVariant v = item->data(Qt::UserRole);
	return v.value<OBSSource>();
}

void AFQBasicFilters::UpdateFilters()
{
	if (!m_obsSource)
		return;

	ClearListItems(ui->effectFilters);
	ClearListItems(ui->asyncFilters);

	obs_source_enum_filters(
		m_obsSource,
		[](obs_source_t*, obs_source_t* filter, void* p) {
			AFQBasicFilters* window =
				reinterpret_cast<AFQBasicFilters*>(p);

			window->AddFilter(filter, false);
			window->adjustSize();
		},
		this);

	if (ui->asyncFilters->count() > 0) {
		ui->asyncFilters->setCurrentItem(ui->asyncFilters->item(0));
		ui->asyncFilters->adjustSize();
	}
	else if (ui->effectFilters->count() > 0) {
		ui->effectFilters->setCurrentItem(ui->effectFilters->item(0));
		ui->effectFilters->adjustSize();
	}

	//main->SaveProject();
}

void AFQBasicFilters::UpdateSplitter()
{
	bool show_splitter_frame =
		ui->asyncFilters->count() + ui->effectFilters->count() > 0;
	UpdateSplitter(show_splitter_frame);
}

void AFQBasicFilters::UpdateSplitter(bool show_splitter_frame)
{
	bool show_splitter_handle = show_splitter_frame;
	uint32_t caps = obs_source_get_output_flags(m_obsSource);
	if ((caps & OBS_SOURCE_VIDEO) == 0)
			show_splitter_handle = false;

	//for (int i = 0; i < ui->rightLayout->count(); i++) {
	//	QSplitterHandle* hndl = ui->rightLayout->handle(i);
	//	hndl->setEnabled(show_splitter_handle);
	//}

	ui->propertiesFrame->setVisible(show_splitter_frame);
}

void AFQBasicFilters::UpdatePropertiesView(int row, bool async)
{
	OBSSource filter = GetFilter(row, async);
	if (filter && m_propsView && m_propsView->IsObject(filter)) {
		/* do not recreate properties view if already using a view
		 * with the same object */
		return;
	}

	if (m_propsView) {
		m_signalUpdateProperties.Disconnect();
		ui->propertiesFrame->setVisible(false);
		/* Deleting a filter will trigger a visibility change, which will also
		 * trigger a focus change if the focus has not been on the list itself
		 * (e.g. after interacting with the property view).
		 *
		 * When an async filter list is available in the view, it will be the first
		 * candidate to receive focus. If this list is empty, we hide the property
		 * view by default and set the view to a `nullptr`.
		 *
		 * When the call for the visibility change returns, we need to check for
		 * this possibility, as another event might have hidden (and deleted) the
		 * view already.
		 *
		 * macOS might be especially affected as it doesn't switch keyboard focus
		 * to buttons like Windows does. */

		if (m_propsView) {
			m_propsView->hide();
			m_propsView->deleteLater();
			m_propsView = nullptr;
		}
	}

	if (!filter)
		return;

	OBSDataAutoRelease settings = obs_source_get_settings(filter);

	auto disabled_undo = [](void* vp, obs_data_t* settings) {
		AFMainFrame* main = App()->GetMainView();
		main->m_undo_s.Disable();
		obs_source_t* source = reinterpret_cast<obs_source_t*>(vp);
		obs_source_update(source, settings);
	};

	m_propsView = new AFQPropertiesView(settings.Get(), filter,
										(PropertiesReloadCallback)obs_source_properties,
										(PropertiesUpdateCallback)FilterChangeUndoRedo,
										(PropertiesVisualUpdateCb)disabled_undo);

	m_propsView->SetComboBoxFixedSize(QSize(235,40));

	m_signalUpdateProperties.Connect(obs_source_get_signal_handler(filter),
									 "update_properties",
									 AFQBasicFilters::UpdateProperties, this);

	m_propsView->setMinimumHeight(150);
	UpdateSplitter();
	ui->propertiesLayout->addWidget(m_propsView);
	m_propsView->show();
}


void AFQBasicFilters::UpdateItemFontColor(int row, bool async)
{
	QListWidget* list = (async ? ui->asyncFilters : ui->effectFilters);

	for (int i = 0; i < list->count(); i++) {
		QListWidgetItem* item = list->item(i);
		if (!item)
			continue;

		QWidget* widget = list->itemWidget(item);
		if (!widget)
			continue;

		VisibilityItemWidget* itemWidget = qobject_cast<VisibilityItemWidget*>(widget);
		if (!itemWidget)
			continue;

		if (row == i)
			itemWidget->SetItemSelected(true);
		else
			itemWidget->SetItemSelected(false);
	}
}

void AFQBasicFilters::AddNewFilter(const char* id)
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

	if (id && *id) {
		OBSSourceAutoRelease existing_filter;
		string name = obs_source_get_display_name(id);

		QString placeholder = QString::fromStdString(name);
		QString text{ placeholder };
		int i = 2;
		while ((existing_filter = obs_source_get_filter_by_name(
			m_obsSource, QT_TO_UTF8(text)))) {
			text = QString("%1 %2").arg(placeholder).arg(i++);
		}

		bool success = AFQNameDialog::AskForName(
			this, locale.Str("Basic.Filters.AddFilter.Title"),
			locale.Str("Basic.Filters.AddFilter.Text"), name, text);
		
		if (!success)
			return;

		if (name.empty()) {
			AFCMessageBox::warning(this,
								   locale.Str("NoNameEntered.Title"),
								   locale.Str("NoNameEntered.Text"));
			AddNewFilter(id);
			return;
		}

		existing_filter =
			obs_source_get_filter_by_name(m_obsSource, name.c_str());
		if (existing_filter) {
			AFCMessageBox::warning(this, locale.Str("NameExists.Title"),
								   locale.Str("NameExists.Text"));
			AddNewFilter(id);
			return;
		}

		OBSSourceAutoRelease filter =
			obs_source_create(id, name.c_str(), nullptr, nullptr);
		if (filter) {
			const char* sourceName = obs_source_get_name(m_obsSource);

			blog(LOG_INFO,
				"User added filter '%s' (%s) to source '%s'",
				name.c_str(), id, sourceName);

			obs_source_filter_add(m_obsSource, filter);
		}
		else {
			blog(LOG_WARNING, "Creating filter '%s' failed!", id);
			return;
		}

		AFMainFrame* main = App()->GetMainView();
		std::string parent_uuid(obs_source_get_uuid(m_obsSource));
		std::string scene_uuid = obs_source_get_uuid(AFSourceUtil::GetCurrentSource());
		/* In order to ensure that the UUID persists through undo/redo,
		 * we save the source data rather than just recreating the
		 * source from scratch. */
		OBSDataAutoRelease rwrapper = obs_save_source(filter);
		obs_data_set_string(rwrapper, "undo_uuid", parent_uuid.c_str());

		OBSDataAutoRelease uwrapper = obs_data_create();
		obs_data_set_string(uwrapper, "fname",
					obs_source_get_name(filter));
		obs_data_set_string(uwrapper, "suuid", parent_uuid.c_str());

		AFMainDynamicComposit* mainComposit = main->GetMainWindow();
		auto undo = [scene_uuid, mainComposit](const std::string& data) {
			OBSSourceAutoRelease ssource = obs_get_source_by_uuid(scene_uuid.c_str());
			mainComposit->SetCurrentScene(ssource.Get(), true);

			OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
			OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(dat, "suuid"));
			OBSSourceAutoRelease filter = obs_source_get_filter_by_name(source, obs_data_get_string(dat, "fname"));
			obs_source_filter_remove(source, filter);
		};

		auto redo = [scene_uuid, mainComposit](const std::string& data)
		{
			OBSSourceAutoRelease ssource = obs_get_source_by_uuid(scene_uuid.c_str());
			mainComposit->SetCurrentScene(ssource.Get(), true);

			OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
			OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(dat, "undo_uuid"));
			OBSSourceAutoRelease filter = obs_load_source(dat);
			obs_source_filter_add(source, filter);
		};

		std::string undo_data(obs_data_get_json(uwrapper));
		std::string redo_data(obs_data_get_json(rwrapper));
		main->m_undo_s.AddAction(QTStr("Undo.Add").arg(obs_source_get_name(filter)),
								 undo, redo, undo_data, redo_data, false);
	}
}


void AFQBasicFilters::RemoveFilter(OBSSource filter)
{
	uint32_t flags = obs_source_get_output_flags(filter);
	bool async = (flags & OBS_SOURCE_ASYNC) != 0;
	QListWidget* list = async ? ui->asyncFilters : ui->effectFilters;

	for (int i = 0; i < list->count(); i++) {
		QListWidgetItem* item = list->item(i);
		QVariant v = item->data(Qt::UserRole);
		OBSSource curFilter = v.value<OBSSource>();

		if (filter == curFilter) {
			DeleteListItem(list, item);
			break;
		}
	}

	const char* filterName = obs_source_get_name(filter);
	const char* sourceName = obs_source_get_name(m_obsSource);
	if (!sourceName || !filterName)
		return;

	const char* filterId = obs_source_get_id(filter);

	blog(LOG_INFO, "User removed filter '%s' (%s) from source '%s'",
		filterName, filterId, sourceName);

	//main->SaveProject();
}

void AFQBasicFilters::ReorderFilter(QListWidget* list, obs_source_t* filter, size_t idx)
{
	int count = list->count();

	for (int i = 0; i < count; i++) {
		QListWidgetItem* listItem = list->item(i);
		QVariant v = listItem->data(Qt::UserRole);
		OBSSource filterItem = v.value<OBSSource>();

		if (filterItem == filter) {
			if ((int)idx != i) {
				bool sel = (list->currentRow() == i);

				listItem = TakeListItem(list, i);
				if (listItem) {
					list->insertItem((int)idx, listItem);
					SetupVisibilityItem(list, listItem,
						filterItem);

					if (sel)
						list->setCurrentRow((int)idx);
				}
			}

			break;
		}
	}
}

void AFQBasicFilters::OBSSourceFilterAdded(void* param, calldata_t* data)
{
	AFQBasicFilters* window = reinterpret_cast<AFQBasicFilters*>(param);
	obs_source_t* filter = (obs_source_t*)calldata_ptr(data, "filter");

	QMetaObject::invokeMethod(window, "AddFilter",
							  Q_ARG(OBSSource, OBSSource(filter)));
}


void AFQBasicFilters::OBSSourceFilterRemoved(void* param, calldata_t* data)
{
	AFQBasicFilters* window = reinterpret_cast<AFQBasicFilters*>(param);
	obs_source_t* filter = (obs_source_t*)calldata_ptr(data, "filter");

	QMetaObject::invokeMethod(window, "RemoveFilter",
							  Q_ARG(OBSSource, OBSSource(filter)));
}

void AFQBasicFilters::OBSSourceReordered(void* param, calldata_t* data)
{
	QMetaObject::invokeMethod(reinterpret_cast<AFQBasicFilters*>(param),
						      "ReorderFilters");
}

void AFQBasicFilters::SourceRemoved(void* param, calldata_t* data)
{
	QMetaObject::invokeMethod(static_cast<AFQBasicFilters*>(param),
							  "close");
}

void AFQBasicFilters::SourceRenamed(void* param, calldata_t* data)
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

	const char* name = calldata_string(data, "new_name");
	QString title = locale.Str("Basic.Filters.Title");
	title = title.arg(QT_UTF8(name));

	QMetaObject::invokeMethod(static_cast<AFQBasicFilters*>(param),
							  "RenameFiltersTitle", Q_ARG(QString, title));
}

void AFQBasicFilters::UpdateProperties(void* data, calldata_t* params)
{
	QMetaObject::invokeMethod(static_cast<AFQBasicFilters*>(data)->m_propsView,
							  "ReloadProperties");
}

void AFQBasicFilters::DrawPreview(void* data, uint32_t cx, uint32_t cy)
{
	AFQBasicFilters* window = static_cast<AFQBasicFilters*>(data);

	if (!window->m_obsSource)
		return;

	uint32_t sourceCX = max(obs_source_get_width(window->m_obsSource), 1u);
	uint32_t sourceCY = max(obs_source_get_height(window->m_obsSource), 1u);

	int x, y;
	int newCX, newCY;
	float scale;

	GetScaleAndCenterPos(sourceCX, sourceCY, cx, cy, x, y, scale);

	newCX = int(scale * float(sourceCX));
	newCY = int(scale * float(sourceCY));

	gs_viewport_push();
	gs_projection_push();
	const bool previous = gs_set_linear_srgb(true);

	gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY), -100.0f, 100.0f);
	gs_set_viewport(x, y, newCX, newCY);
	obs_source_video_render(window->m_obsSource);

	gs_set_linear_srgb(previous);
	gs_projection_pop();
	gs_viewport_pop();
}

void AFQBasicFilters::CustomContextMenu(const QPoint& pos, bool async)
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();
	AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

	QListWidget* list = async ? ui->asyncFilters : ui->effectFilters;
	QListWidgetItem* item = list->itemAt(pos);

	AFQCustomMenu popup(window());

	QPointer<AFQCustomMenu> addMenu = CreateAddFilterPopupMenu(async);
	if (addMenu)
		popup.addMenu(addMenu);

	if (item) {
		popup.addSeparator();
		popup.addAction(locale.Str("Duplicate"), this, [&]() {
			DuplicateItem(async ? ui->asyncFilters->currentItem()
				: ui->effectFilters->currentItem());
			});
		popup.addSeparator();
		popup.addAction(m_actionRenameFilter);
		popup.addAction(m_actionRemoveFilter);
		popup.addSeparator();

		QAction* copyAction = new QAction(locale.Str("Copy"));
		connect(copyAction, &QAction::triggered, this,
			&AFQBasicFilters::CopyFilter);
		copyAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_C));
		ui->effectWidget->addAction(copyAction);
		ui->asyncWidget->addAction(copyAction);
		popup.addAction(copyAction);
	}

	QAction* pasteAction = new QAction(locale.Str("Paste"));
	pasteAction->setEnabled(sceneContext.m_copyFilter);
	connect(pasteAction, &QAction::triggered, this,
		&AFQBasicFilters::PasteFilter);
	pasteAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_V));
	ui->effectWidget->addAction(pasteAction);
	ui->asyncWidget->addAction(pasteAction);
	popup.addAction(pasteAction);

	popup.exec(QCursor::pos());
}

void AFQBasicFilters::EditItem(QListWidgetItem* item, bool async)
{
	if (m_editActive)
		return;

	Qt::ItemFlags flags = item->flags();
	OBSSource filter = item->data(Qt::UserRole).value<OBSSource>();
	const char* name = obs_source_get_name(filter);
	QListWidget* list = async ? ui->asyncFilters : ui->effectFilters;

	item->setText(QT_UTF8(name));
	item->setFlags(flags | Qt::ItemIsEditable);
	list->removeItemWidget(item);
	list->editItem(item);
	item->setFlags(flags);
	m_editActive = true;
}

void AFQBasicFilters::DuplicateItem(QListWidgetItem* item)
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

	OBSSource filter = item->data(Qt::UserRole).value<OBSSource>();
	string name = obs_source_get_name(filter);
	OBSSourceAutoRelease existing_filter;

	QString placeholder = QString::fromStdString(name);
	QString text{ placeholder };
	int i = 2;
	while ((existing_filter = obs_source_get_filter_by_name(
		m_obsSource, QT_TO_UTF8(text)))) {
		text = QString("%1 %2").arg(placeholder).arg(i++);
	}

	bool success = AFQNameDialog::AskForName(
		this, locale.Str("Basic.Filters.AddFilter.Title"),
		locale.Str("Basic.Filters.AddFilter.Text"), name, text);
	if (!success)
		return;

	if (name.empty()) {
		AFCMessageBox::warning(this, locale.Str("NoNameEntered.Title"),
			locale.Str("NoNameEntered.Text"));
		DuplicateItem(item);
		return;
	}

	existing_filter = obs_source_get_filter_by_name(m_obsSource, name.c_str());
	if (existing_filter) {
		AFCMessageBox::warning(this, locale.Str("NameExists.Title"),
			locale.Str("NameExists.Text"));
		DuplicateItem(item);
		return;
	}
	bool enabled = obs_source_enabled(filter);
	OBSSourceAutoRelease new_filter =
		obs_source_duplicate(filter, name.c_str(), false);
	if (new_filter) {
		const char* sourceName = obs_source_get_name(m_obsSource);
		const char* id = obs_source_get_id(new_filter);
		blog(LOG_INFO,
			"User duplicated filter '%s' (%s) from '%s' "
			"to source '%s'",
			name.c_str(), id, name.c_str(), sourceName);
		obs_source_set_enabled(new_filter, enabled);
		obs_source_filter_add(m_obsSource, new_filter);
	}
}

void AFQBasicFilters::FilterNameEdited(QWidget* editor, QListWidget* list)
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

	QListWidgetItem* listItem = list->currentItem();
	OBSSource filter = listItem->data(Qt::UserRole).value<OBSSource>();
	QLineEdit* edit = qobject_cast<QLineEdit*>(editor);
	string name = QT_TO_UTF8(edit->text().trimmed());

	const char* prevName = obs_source_get_name(filter);
	bool sameName = (name == prevName);
	OBSSourceAutoRelease foundFilter = nullptr;

	if (!sameName)
		foundFilter =
		obs_source_get_filter_by_name(m_obsSource, name.c_str());

	if (foundFilter || name.empty() || sameName) {
		listItem->setText(QT_UTF8(prevName));

		if (foundFilter) {
			AFCMessageBox::information(window(),
									   locale.Str("NameExists.Title"),
									   locale.Str("NameExists.Text"));
		}
		else if (name.empty()) {
			AFCMessageBox::information(window(),
									   locale.Str("NoNameEntered.Title"),
									   locale.Str("NoNameEntered.Text"));
		}
	}
	else {
		const char* sourceName = obs_source_get_name(m_obsSource);

		blog(LOG_INFO,
			"User renamed filter '%s' on source '%s' to '%s'",
			prevName, sourceName, name.c_str());

		listItem->setText(QT_UTF8(name.c_str()));
		obs_source_set_name(filter, name.c_str());

		AFMainFrame* main = App()->GetMainView();
		AFMainDynamicComposit* mainComposit = main->GetMainWindow();
		std::string scene_uuid = obs_source_get_uuid(AFSourceUtil::GetCurrentSource());
		auto undo = [scene_uuid, prev = std::string(prevName), name, 
			mainComposit](const std::string& uuid)
		{
			OBSSourceAutoRelease ssource = obs_get_source_by_uuid(scene_uuid.c_str());
			mainComposit->SetCurrentScene(ssource.Get(), true);

			OBSSourceAutoRelease filter = obs_get_source_by_uuid(uuid.c_str());
			obs_source_set_name(filter, prev.c_str());
		};

		auto redo = [scene_uuid, prev = std::string(prevName), name,
			mainComposit](const std::string& uuid) {
			OBSSourceAutoRelease ssource = obs_get_source_by_uuid(scene_uuid.c_str());
			mainComposit->SetCurrentScene(ssource.Get(), true);

			OBSSourceAutoRelease filter = obs_get_source_by_uuid(uuid.c_str());
			obs_source_set_name(filter, name.c_str());
		};

		std::string filter_uuid(obs_source_get_uuid(filter));
		main->m_undo_s.AddAction(QTStr("Undo.Rename").arg(name.c_str()),
								 undo, redo, filter_uuid, filter_uuid);
	}

	listItem->setText(QString());
	SetupVisibilityItem(list, listItem, filter);
	m_editActive = false;
}

void AFQBasicFilters::delete_filter(OBSSource filter)
{
	OBSDataAutoRelease wrapper = obs_save_source(filter);
	std::string parent_uuid(obs_source_get_uuid(m_obsSource));
	obs_data_set_string(wrapper, "undo_uuid", parent_uuid.c_str());

	AFMainFrame* main = App()->GetMainView();
	AFMainDynamicComposit* mainComposit = main->GetMainWindow();
	std::string scene_uuid = obs_source_get_uuid(AFSourceUtil::GetCurrentSource());
	auto undo = [scene_uuid, mainComposit](const std::string& data) {
		OBSSourceAutoRelease ssource = obs_get_source_by_uuid(scene_uuid.c_str());
		mainComposit->SetCurrentScene(ssource.Get(), true);

		OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
		OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(dat, "undo_uuid"));
		OBSSourceAutoRelease filter = obs_load_source(dat);
		obs_source_filter_add(source, filter);
	};

	OBSDataAutoRelease rwrapper = obs_data_create();
	obs_data_set_string(rwrapper, "fname", obs_source_get_name(filter));
	obs_data_set_string(rwrapper, "suuid", parent_uuid.c_str());
	auto redo = [scene_uuid, mainComposit](const std::string& data) {
		OBSSourceAutoRelease ssource = obs_get_source_by_uuid(scene_uuid.c_str());
		mainComposit->SetCurrentScene(ssource.Get(), true);

		OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
		OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(dat, "suuid"));
		OBSSourceAutoRelease filter = obs_source_get_filter_by_name(source, obs_data_get_string(dat, "fname"));
		obs_source_filter_remove(source, filter);
	};

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	main->m_undo_s.AddAction(QTStr("Undo.Delete").arg(obs_source_get_name(filter)),
							 undo, redo, undo_data, redo_data, false);

	obs_source_filter_remove(m_obsSource, filter);
}