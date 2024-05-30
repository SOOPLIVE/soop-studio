#include "CSourceListView.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <QDropEvent>
#include <QPainter>
#include <QPushButton>

#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Icon/CIconContext.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "Application/CApplication.h"
#include "MainFrame/CMainFrame.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"

#define DESELECT_VISIBLE_ITEM_FONT_COLOR        QColor(255, 255, 255, 178)
#define SELECT_VISIBLE_ITEM_FONT_COLOR          QColor(0, 224, 255, 255)

#define DESELECT_UNVISIBLE_ITEM_FONT_COLOR      QColor(56, 59, 63, 255)
#define SELECT_UNVISIBLE_ITEM_FONT_COLOR        QColor(0, 108, 122, 255)

#define HOVERED_VISIBLE_ITEM_FONT_COLOR        QColor(255, 255, 255, 230)

#define SOURCEVIEW_ITEM_STYLE                 "background-color:%1;   \
                                               font-size:14px;        \
                                               border-radius: 4px;    \
                                               color:%2;"


static inline void MoveData(QVector<OBSSceneItem> &items, int oldIdx, int newIdx)
{
    OBSSceneItem item = items[oldIdx];
    items.remove(oldIdx);
    items.insert(newIdx, item);
}

inline QColor GetSourceListBackgroundColor(int preset)
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

/////////////////SourceViewItem////////////////////
AFQSourceViewItem::AFQSourceViewItem(AFQSourceListView* sourceListView, OBSSceneItem sceneItem) :
    QWidget(sourceListView),
    m_sourceListView(sourceListView),
	m_sceneItem(sceneItem)
{
    // Set Widget Attribute
    setAttribute(Qt::WA_Hover);
	setAttribute(Qt::WA_TranslucentBackground);
	setMouseTracking(true);

    QColor fontColor = DESELECT_VISIBLE_ITEM_FONT_COLOR;
    setStyleSheet(QString(SOURCEVIEW_ITEM_STYLE).arg(
                          m_colorBackground.name(QColor::HexArgb), 
                          fontColor.name(QColor::HexArgb)));

	obs_source_t* source = obs_sceneitem_get_source(m_sceneItem);
	const char* name     = obs_source_get_name(source);
	const char* id       = obs_source_get_id(source);


    OBSDataAutoRelease privData =
        obs_sceneitem_get_private_settings(m_sceneItem);
    int preset = obs_data_get_int(privData, "color-preset");
    if (preset == 1) {
        QColor color(QT_UTF8(obs_data_get_string(privData, "color")));
        SetBackgroundColor(color);
    }
    else if (preset > 1) {
       QColor color = GetSourceListBackgroundColor(preset - 1);
       SetBackgroundColor(color);
    }

    // 
    m_layoutBox = new QHBoxLayout(this);
    m_layoutBox->setContentsMargins(0, 0, 5, 0);
    m_layoutBox->setSpacing(0);

    m_labelIcon = _CreateIconLabel(id);
    m_labelName = _CreateNameLabel(name);
    m_checkBoxVisible = _CreateVisibleCheckBox();
    m_checkBoxLocked  = _CreateLockedCheckBox();

    if (m_labelIcon) {
        m_layoutBox->addWidget(m_labelIcon);
        m_layoutBox->addSpacing(6);
    }
    m_layoutBox->addWidget(m_labelName);
    m_layoutBox->addWidget(m_checkBoxVisible);
    m_layoutBox->addWidget(m_checkBoxLocked);

    Update(false);

    setLayout(m_layoutBox);

    auto setItemVisible = [this](bool val) {
        obs_scene_t* scene = obs_sceneitem_get_scene(m_sceneItem);
        obs_source_t* sceneSource = obs_scene_get_source(scene);
        int64_t id = obs_sceneitem_get_id(m_sceneItem);
        const char* name = obs_source_get_name(sceneSource);
        const char* uuid = obs_source_get_uuid(sceneSource);
        obs_source_t* source = obs_sceneitem_get_source(m_sceneItem);

        auto undo_redo = [](const std::string& uuid, int64_t id, bool val) {
            OBSSourceAutoRelease s = obs_get_source_by_uuid(uuid.c_str());
            obs_scene_t* sc = obs_group_or_scene_from_source(s);
            obs_sceneitem_t* si = obs_scene_find_sceneitem_by_id(sc, id);
            if(si)
                obs_sceneitem_set_visible(si, val);
        };

        QString str = QTStr(val ? "Undo.ShowSceneItem" : "Undo.HideSceneItem");

        AFMainFrame* main = App()->GetMainView();
        main->m_undo_s.AddAction(str.arg(obs_source_get_name(source), name),
                                 std::bind(undo_redo, std::placeholders::_1, id, !val),
                                 std::bind(undo_redo, std::placeholders::_1, id, val), uuid, uuid);

        QSignalBlocker sourceSignalBlocker(this);
        obs_sceneitem_set_visible(m_sceneItem, val);
    };

    auto setItemLocked = [this](bool checked) {
        QSignalBlocker sourcesSignalBlocker(this);
        obs_sceneitem_set_locked(m_sceneItem, checked);
    };

    connect(m_checkBoxVisible, &QAbstractButton::clicked, this, &AFQSourceViewItem::ClickedItemVisible);
    connect(m_checkBoxLocked, &QAbstractButton::clicked, this, &AFQSourceViewItem::ClickedItemLocked);
}

void AFQSourceViewItem::Clear()
{
    DisconnectSignals();
    m_sourceListView = nullptr;
    m_sceneItem = nullptr;
}

void AFQSourceViewItem::EnterEditMode()
{
    setFocusPolicy(Qt::StrongFocus);
    int index = m_layoutBox->indexOf(m_labelName);
    m_layoutBox->removeWidget(m_labelName);
    m_editorName = new QLineEdit(m_labelName->text(),this);
    m_editorName->setStyleSheet("background-color:#2F3238;   \
                                 border:1px solid #AAA;      \
                                 color:rgba(255,255,255,90%);\
                                 font-size:14px;             \
                                 font-style: normal;         \
                                 font-weight: 400;           \
                                 padding-left:2px; ");
    m_editorName->selectAll();
    m_editorName->installEventFilter(this);
    m_layoutBox->insertWidget(index, m_editorName);
    setFocusProxy(m_editorName);
}

void AFQSourceViewItem::ExitEditMode(bool save)
{
    ExitEditModeInternal(save);

    if (m_sourceListView->m_undoSceneData) {
        AFMainFrame* main = App()->GetMainView();
        main->m_undo_s.PopDisabled();

        OBSData redoSceneData = main->BackupScene(AFSceneContext::GetSingletonInstance().GetCurrOBSScene());
        QString text = QTStr("Undo.GroupItems").arg(m_strNewName.c_str());
        main->CreateSceneUndoRedoAction(text, m_sourceListView->m_undoSceneData, redoSceneData);

        m_sourceListView->m_undoSceneData = nullptr;
    }
}

void AFQSourceViewItem::ClickedItemVisible(bool val)
{
    AFLocaleTextManager& localeManager = AFLocaleTextManager::GetSingletonInstance();

    obs_scene_t* scene = obs_sceneitem_get_scene(m_sceneItem);
    obs_source_t* sceneSource = obs_scene_get_source(scene);
    int64_t id = obs_sceneitem_get_id(m_sceneItem);

    const char* name = obs_source_get_name(sceneSource);
    const char* uuid = obs_source_get_uuid(sceneSource);

    auto undo_redo = [](const std::string& uuid, int64_t id, bool val) {
            OBSSourceAutoRelease s = obs_get_source_by_uuid(uuid.c_str());
            obs_scene_t* sc = obs_group_or_scene_from_source(s);
            obs_sceneitem_t* si = obs_scene_find_sceneitem_by_id(sc, id);
            if (si)
                obs_sceneitem_set_visible(si, val);
    };

    QString str = localeManager.Str(val ? "Undo.ShowSceneItem" : "Undo.HideSceneItem");

    AFMainFrame* main = App()->GetMainView();
    main->m_undo_s.AddAction(str.arg(obs_source_get_name(sceneSource), name),
                             std::bind(undo_redo, std::placeholders::_1, id, !val),
                             std::bind(undo_redo, std::placeholders::_1, id, val),
                             uuid, uuid);

    SignalBlocker sourcesSignalBlocker(this);
    obs_sceneitem_set_visible(m_sceneItem, val);
}

void AFQSourceViewItem::ClickedItemLocked(bool val)
{
    SignalBlocker sourcesSignalBlocker(this);
    obs_sceneitem_set_locked(m_sceneItem, val);
}

void AFQSourceViewItem::ExpandClicked(bool checked)
{
    OBSDataAutoRelease data = obs_sceneitem_get_private_settings(m_sceneItem);

    obs_data_set_bool(data, "collapsed", checked);

    if (!checked)
        m_sourceListView->GetStm()->ExpandGroup(m_sceneItem);
    else
        m_sourceListView->GetStm()->CollapseGroup(m_sceneItem);
}


void AFQSourceViewItem::VisibilityChanged(bool visible)
{
    if(m_checkBoxVisible)
        m_checkBoxVisible->setChecked(visible);

    m_isVisible = visible;

    QColor fontColor;
    if (m_isSelected) {
        if (m_isVisible)
            fontColor = SELECT_VISIBLE_ITEM_FONT_COLOR;
        else
            fontColor = SELECT_UNVISIBLE_ITEM_FONT_COLOR;
    }
    else {
        if (m_isVisible)
            fontColor = DESELECT_VISIBLE_ITEM_FONT_COLOR;
        else
            fontColor = DESELECT_UNVISIBLE_ITEM_FONT_COLOR;
    }

    SetFontColor(fontColor);
}


void AFQSourceViewItem::LockedChanged(bool locked)
{
    App()->GetMainView()->UpdateEditMenu();

    if (!m_checkBoxLocked)
        return;

    if (locked)
        m_checkBoxLocked->show();
    else {
        if(m_isHovered)
            m_checkBoxLocked->show();
        else
            m_checkBoxLocked->hide();
    }
}

void AFQSourceViewItem::Select()
{
    m_sourceListView->SelectItem(m_sceneItem, true);
    DYNAMIC_COMPOSIT->UpdateContextPopupDeferred();
    App()->GetMainView()->UpdateEditMenu();
}

void AFQSourceViewItem::DeSelect()
{
    m_sourceListView->SelectItem(m_sceneItem, false);
    DYNAMIC_COMPOSIT->UpdateContextPopupDeferred();
    App()->GetMainView()->UpdateEditMenu();
}

void AFQSourceViewItem::DisconnectSignals()
{
    m_signalSelect.Disconnect();
    m_signalDeSelect.Disconnect();
    m_signalSceneRemove.Disconnect();
    m_signalItemRemove.Disconnect();
    m_signalVisible.Disconnect();
}

void AFQSourceViewItem::ReconnectSignals()
{
    if (!m_sceneItem)
        return;

    DisconnectSignals();

    obs_scene_t* scene = obs_sceneitem_get_scene(m_sceneItem);
    obs_source_t* sceneSource = obs_scene_get_source(scene);
    signal_handler_t* signal = obs_source_get_signal_handler(sceneSource);

    m_signalSceneRemove.Connect(signal, "remove", CallbackRemoveItem, this);
    m_signalItemRemove.Connect(signal, "item_remove", CallbackRemoveItem, this);
    m_signalVisible.Connect(signal, "item_visible", CallbackItemVisible, this);
    m_signalLocked.Connect(signal, "item_locked", CallbackItemLocked, this);
    m_signalSelect.Connect(signal, "item_select", CallbackItemSelect, this);
    m_signalDeSelect.Connect(signal, "item_deselect", CallbackItemDeSelect, this);

    if (obs_sceneitem_is_group(m_sceneItem)) {
        obs_source_t* source = obs_sceneitem_get_source(m_sceneItem);
        signal = obs_source_get_signal_handler(source);

        m_signalGroupReorder.Connect(signal, "reorder", CallbackItemReorderGroup,
            this);
    }

    obs_source_t* source = obs_sceneitem_get_source(m_sceneItem);
    signal = obs_source_get_signal_handler(source);
    m_signalRename.Connect(signal, "rename", CallbackRenamedSource, this);
    m_signalRemove.Connect(signal, "remove", CallbackRemoveSource, this);
}

void AFQSourceViewItem::Update(bool force)
{
    AFSceneContext& contextScene = AFSceneContext::GetSingletonInstance();
    OBSScene scene = contextScene.GetCurrOBSScene();
    obs_scene_t* itemScene = obs_sceneitem_get_scene(m_sceneItem);

    Type newType;

    /* ------------------------------------------------- */
    /* if it's a group item, insert group checkbox       */

    if (obs_sceneitem_is_group(m_sceneItem)) {
        newType = Type::Group;

        /* ------------------------------------------------- */
        /* if it's a group sub-item                          */

    }
    else if (itemScene != scene) {
        newType = Type::SubItem;

        /* ------------------------------------------------- */
        /* if it's a regular item                            */

    }
    else {
        newType = Type::Item;
    }

    /* ------------------------------------------------- */

    if (!force && newType == m_type) {
        return;
    }

    /* ------------------------------------------------- */

    ReconnectSignals();

    if (groupSpacer) {
        m_layoutBox->removeItem(groupSpacer);
        delete groupSpacer;
        groupSpacer = nullptr;
    }

    if (m_type == Type::Group) {
        m_layoutBox->removeWidget(groupExpand);
        groupExpand->deleteLater();
        groupExpand = nullptr;
    }

    m_type = newType;

    if (m_type == Type::SubItem) {
        groupSpacer = new QSpacerItem(24, 1);
        m_layoutBox->insertItem(0, groupSpacer);

    }
    else if (m_type == Type::Group) {
        groupExpand = new SourceTreeSubItemCheckBox();
        groupExpand->setSizePolicy(QSizePolicy::Maximum,
            QSizePolicy::Maximum);
        groupExpand->setMaximumSize(24, 24);
        groupExpand->setMinimumSize(24, 0);
#ifdef __APPLE__
        groupExpand->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif
        m_layoutBox->insertWidget(0, groupExpand);

        OBSDataAutoRelease data =
            obs_sceneitem_get_private_settings(m_sceneItem);
        groupExpand->blockSignals(true);
        groupExpand->setChecked(obs_data_get_bool(data, "collapsed"));
        groupExpand->blockSignals(false);

        connect(groupExpand, &QPushButton::toggled, this,
                &AFQSourceViewItem::ExpandClicked);
    }
    else {
        groupSpacer = new QSpacerItem(3, 1);
        m_layoutBox->insertItem(0, groupSpacer);
    }
}

void AFQSourceViewItem::SetSelected(bool select, bool refresh)
{
    m_isSelected = select;

    if (m_isSelected) {
        if (m_isVisible)
            m_colorFont = SELECT_VISIBLE_ITEM_FONT_COLOR;
        else
            m_colorFont = SELECT_UNVISIBLE_ITEM_FONT_COLOR;
    }
    else {
        if (m_isVisible)
            m_colorFont = DESELECT_VISIBLE_ITEM_FONT_COLOR;
        else
            m_colorFont = DESELECT_UNVISIBLE_ITEM_FONT_COLOR;
    }

    if (refresh)
        RefreshSourceListItemColor();
}

void AFQSourceViewItem::SetBackgroundColor(const QColor& color, bool refresh)
{
    m_colorBackground = color;

    if(refresh)
        RefreshSourceListItemColor();
}

void AFQSourceViewItem::SetFontColor(const QColor& color, bool refresh)
{
    m_colorFont = color;

    if (refresh)
        RefreshSourceListItemColor();
}


void AFQSourceViewItem::RefreshSourceListItemColor()
{
    QPixmap pixmapIconSource = m_iconSource.pixmap(QSize(24, 24));
    QPainter painter(&pixmapIconSource);

    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmapIconSource.rect(), m_colorFont);
    painter.end();

    if(m_labelIcon)
        m_labelIcon->setPixmap(pixmapIconSource);

    setStyleSheet(QString(SOURCEVIEW_ITEM_STYLE).arg(
                  m_colorBackground.name(QColor::HexArgb),
                  m_colorFont.name(QColor::HexArgb)));
}

bool AFQSourceViewItem::IsEditing()
{
    return m_editorName != nullptr;
}

QColor AFQSourceViewItem::GetBackgroundColor()
{
    return m_colorBackground;
}

QLabel* AFQSourceViewItem::_CreateIconLabel(const char* id)
{
    AFIconContext& iconContext = AFIconContext::GetSingletonInstance();

    QLabel* iconLabel = new QLabel(this);
    iconLabel->setFixedSize(24, 24);
    iconLabel->setObjectName("sourceIconLabel");


    if (strcmp(id, "scene") == 0)
        m_iconSource = iconContext.GetSceneIcon();
    else if (strcmp(id, "group") == 0)
        m_iconSource = iconContext.GetGroupIcon();
    else
        m_iconSource = iconContext.GetSourceIcon(id);

    QPixmap pixmap = m_iconSource.pixmap(QSize(24, 24));
    iconLabel->setPixmap(pixmap);
    iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    iconLabel->setStyleSheet("background:none;");

    return iconLabel;
}

QLabel* AFQSourceViewItem::_CreateNameLabel(const char* name)
{
    QLabel* nameLabel = new QLabel(this);
    nameLabel->setText(name);
    nameLabel->setObjectName("sourceNameLabel"); 
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    nameLabel->setAttribute(Qt::WA_TranslucentBackground);
    nameLabel->setMinimumWidth(0);

    return nameLabel;
}


AFQVisibleCheckBox* AFQSourceViewItem::_CreateVisibleCheckBox()
{
    m_isVisible = obs_sceneitem_visible(m_sceneItem);

    AFQVisibleCheckBox* vis = new AFQVisibleCheckBox();
    vis->setFixedSize(QSize(24, 24));
    vis->setStyleSheet("background: none");
    vis->setObjectName("checkBox_sourceViewCheckbox");
    vis->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    vis->setChecked(m_isVisible);
    vis->hide();

    if (m_isVisible)
        SetFontColor(DESELECT_VISIBLE_ITEM_FONT_COLOR);
	else
        SetFontColor(DESELECT_UNVISIBLE_ITEM_FONT_COLOR);

    return vis;
}

AFQLockedCheckBox* AFQSourceViewItem::_CreateLockedCheckBox()
{
    bool locked = obs_sceneitem_locked(m_sceneItem);

    AFQLockedCheckBox* lock = new AFQLockedCheckBox();
    lock->setFixedSize(QSize(24, 24));
    lock->setStyleSheet("background: none");
    lock->setObjectName("checkBox_sourceLockCheckbox");
    lock->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    lock->setChecked(locked);
    if(!locked)
        lock->hide();

    return lock;
}

void AFQSourceViewItem::ExitEditModeInternal(bool save)
{
    if (!m_editorName) {
        return;
    }

    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
    OBSScene scene = sceneContext.GetCurrOBSScene();

    m_strNewName = QT_TO_UTF8(m_editorName->text());

    setFocusProxy(nullptr);
    int index = m_layoutBox->indexOf(m_editorName);
    m_layoutBox->removeWidget(m_editorName);
    delete m_editorName;
    m_editorName = nullptr;
    setFocusPolicy(Qt::NoFocus);
    m_layoutBox->insertWidget(index, m_labelName);
    setFocus();

    /* ----------------------------------------- */
    /* check for empty string                    */

    if (!save)
        return;

    if (m_strNewName.empty()) {
        return;
    }

    /* ----------------------------------------- */
    /* Check for same name                       */

    obs_source_t* source = obs_sceneitem_get_source(m_sceneItem);
    if (m_strNewName == obs_source_get_name(source))
        return;

    /* ----------------------------------------- */
    /* check for existing source                 */

    OBSSourceAutoRelease existingSource =
        obs_get_source_by_name(m_strNewName.c_str());
    bool exists = !!existingSource;

    if (exists) {
        return;
    }

    /* ----------------------------------------- */
    /* rename                                    */

    AFMainFrame* main = App()->GetMainView();
    SignalBlocker sourcesSignalBlocker(this);
    std::string prevName(obs_source_get_name(source));
    std::string scene_uuid = obs_source_get_uuid(AFSourceUtil::GetCurrentSource());
    auto undo = [scene_uuid, prevName, main](const std::string& data) {
        OBSSourceAutoRelease source = obs_get_source_by_uuid(data.c_str());
        obs_source_set_name(source, prevName.c_str());
        OBSSourceAutoRelease scene_source = obs_get_source_by_uuid(scene_uuid.c_str());
        main->GetMainWindow()->SetCurrentScene(scene_source.Get(), true);
    };

    std::string editedName = m_strNewName;

    auto redo = [scene_uuid, main, editedName](const std::string& data) {
        OBSSourceAutoRelease source = obs_get_source_by_uuid(data.c_str());
        obs_source_set_name(source, editedName.c_str());

        OBSSourceAutoRelease scene_source = obs_get_source_by_uuid(scene_uuid.c_str());
        main->GetMainWindow()->SetCurrentScene(scene_source.Get(), true);
    };

    const char* uuid = obs_source_get_uuid(source);
    main->m_undo_s.AddAction(QTStr("Undo.Rename").arg(m_strNewName.c_str()),
                             undo, redo, uuid, uuid);

    obs_source_set_name(source, m_strNewName.c_str());
    m_labelName->setText(QT_UTF8(m_strNewName.c_str()));
}

void AFQSourceViewItem::CallbackRemoveItem(void* data, calldata_t* cd)
{
    AFQSourceViewItem* sourceItem = reinterpret_cast<AFQSourceViewItem*>(data);
    obs_sceneitem_t* curItem =
        (obs_sceneitem_t*)calldata_ptr(cd, "item");

    if (curItem == sourceItem->m_sceneItem) {
        QMetaObject::invokeMethod(sourceItem->m_sourceListView, "Remove",
            Q_ARG(OBSSceneItem, curItem));
        curItem = nullptr;
    }
    if (!curItem)
        QMetaObject::invokeMethod(sourceItem, "Clear");
}

void AFQSourceViewItem::CallbackItemVisible(void* data, calldata_t* cd)
{
    AFQSourceViewItem* sourceItem = reinterpret_cast<AFQSourceViewItem*>(data);

    obs_sceneitem_t* curItem = (obs_sceneitem_t*)calldata_ptr(cd, "item");
    bool visible = calldata_bool(cd, "visible");

    if (curItem == sourceItem->m_sceneItem)
        QMetaObject::invokeMethod(sourceItem, "VisibilityChanged",
            Q_ARG(bool, visible));
}


void AFQSourceViewItem::CallbackItemSelect(void* data, calldata_t* cd)
{
    AFQSourceViewItem* sourceItem =
        reinterpret_cast<AFQSourceViewItem*>(data);
    obs_sceneitem_t* curItem =
        (obs_sceneitem_t*)calldata_ptr(cd, "item");

    if (curItem == sourceItem->m_sceneItem)
        QMetaObject::invokeMethod(sourceItem, "Select");
}

void AFQSourceViewItem::CallbackItemDeSelect(void* data, calldata_t* cd)
{
    AFQSourceViewItem* sourceItem =
        reinterpret_cast<AFQSourceViewItem*>(data);
    obs_sceneitem_t* curItem =
        (obs_sceneitem_t*)calldata_ptr(cd, "item");

    if (curItem == sourceItem->m_sceneItem)
        QMetaObject::invokeMethod(sourceItem, "DeSelect");
}


void AFQSourceViewItem::CallbackItemLocked(void* data, calldata_t* cd)
{
    AFQSourceViewItem* sourceItem =
        reinterpret_cast<AFQSourceViewItem*>(data);
    obs_sceneitem_t* curItem =
        (obs_sceneitem_t*)calldata_ptr(cd, "item");
    bool locked = calldata_bool(cd, "locked");

    if (curItem == sourceItem->m_sceneItem)
        QMetaObject::invokeMethod(sourceItem, "LockedChanged",
                                  Q_ARG(bool, locked));
}


void AFQSourceViewItem::CallbackItemReorderGroup(void* data, calldata_t* cd)
{
    AFQSourceViewItem* sourceItem =
        reinterpret_cast<AFQSourceViewItem*>(data);
    QMetaObject::invokeMethod(sourceItem->m_sourceListView, "ReorderItems");
}

void AFQSourceViewItem::CallbackRenamedSource(void* data, calldata_t* cd)
{
    AFQSourceViewItem* sourceItem =
        reinterpret_cast<AFQSourceViewItem*>(data);
    const char* name = calldata_string(cd, "new_name");

    QMetaObject::invokeMethod(sourceItem, "Renamed",
                              Q_ARG(QString, QT_UTF8(name)));
}

void AFQSourceViewItem::CallbackRemoveSource(void* data, calldata_t* cd)
{
    AFQSourceViewItem* sourceItem =
        reinterpret_cast<AFQSourceViewItem*>(data);
    sourceItem->DisconnectSignals();
    sourceItem->m_sceneItem = nullptr;
    QMetaObject::invokeMethod(sourceItem->m_sourceListView, "RefreshItems");
}

void AFQSourceViewItem::mouseDoubleClickEvent(QMouseEvent* event)
{
    QWidget::mouseDoubleClickEvent(event);

    if (groupExpand) {
        groupExpand->setChecked(!groupExpand->isChecked());
    }
    else {
        obs_source_t* source = obs_sceneitem_get_source(m_sceneItem);

        if (obs_source_configurable(source)) {
            App()->GetMainView()->CreatePropertiesPopup(source);
        }
    }
}

void AFQSourceViewItem::enterEvent(QEnterEvent* event)
{
    if(m_checkBoxVisible)
        m_checkBoxVisible->show();

    if(m_checkBoxLocked)
        m_checkBoxLocked->show();

    if (!m_isSelected && m_isVisible) {
        SetFontColor(HOVERED_VISIBLE_ITEM_FONT_COLOR);
    }

    m_isHovered = true;
}

void AFQSourceViewItem::leaveEvent(QEvent* event)
{
    if (m_checkBoxVisible)
        m_checkBoxVisible->hide();

    if (m_checkBoxLocked) {
        if(!obs_sceneitem_locked(m_sceneItem))
            m_checkBoxLocked->hide();
    }

    QColor fontColor;
    if (m_isSelected) {
        if (m_isVisible)
            fontColor = SELECT_VISIBLE_ITEM_FONT_COLOR;
        else
            fontColor = SELECT_UNVISIBLE_ITEM_FONT_COLOR;
    }
    else {
        if (m_isVisible)
            fontColor = DESELECT_VISIBLE_ITEM_FONT_COLOR;
        else
            fontColor = DESELECT_UNVISIBLE_ITEM_FONT_COLOR;
    }

    SetFontColor(fontColor);

    m_isHovered = false;

}

void AFQSourceViewItem::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    QWidget::paintEvent(event);
}

bool AFQSourceViewItem::eventFilter(QObject* object, QEvent* event)
{
    if (m_editorName != object)
        return false;

    if (LineEditCanceled(event)) {
        QMetaObject::invokeMethod(this, "ExitEditMode",
            Qt::QueuedConnection,
            Q_ARG(bool, false));
        return true;
    }
    if (LineEditChanged(event)) {
        QMetaObject::invokeMethod(this, "ExitEditMode",
            Qt::QueuedConnection,
            Q_ARG(bool, true));
        return true;
    }

    return false;

}

/////////////////SourceViewModel////////////////////


AFQSourceViewModel::AFQSourceViewModel(AFQSourceListView* parent)
    : QAbstractListModel(parent)
    , m_sourceListView(parent)
{

}

int AFQSourceViewModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_vSourceList.count();
}

QVariant AFQSourceViewModel::data(const QModelIndex& index, int role) const
{
    //if (role == Qt::AccessibleTextRole) {
    //    OBSSceneItem item = m_vSourceList[index.row()];
    //    obs_source_t* source = obs_sceneitem_get_source(item);
    //    return QVariant(QT_UTF8(obs_source_get_name(source)));
    //}
    if (!index.isValid())
        return QVariant();

    if (role == Qt::SizeHintRole)
    {
        return QSize(0, 30);
    }

    return QVariant();
}

Qt::ItemFlags AFQSourceViewModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return QAbstractListModel::flags(index) | Qt::ItemIsDropEnabled;

    obs_sceneitem_t* item = m_vSourceList[index.row()];
    bool is_group = obs_sceneitem_is_group(item);

    return QAbstractListModel::flags(index) | Qt::ItemIsEditable |
        Qt::ItemIsDragEnabled |
        (is_group ? Qt::ItemIsDropEnabled : Qt::NoItemFlags);
}

Qt::DropActions AFQSourceViewModel::supportedDropAction() const
{
    return QAbstractItemModel::supportedDropActions() | Qt::MoveAction;
}

void AFQSourceViewModel::Clear()
{
    beginResetModel();
    m_vSourceList.clear();
    endResetModel();

    m_hasGroups = false;
}

static bool enumItem(obs_scene_t*, obs_sceneitem_t* item, void* ptr)
{
    QVector<OBSSceneItem>& items =
        *reinterpret_cast<QVector<OBSSceneItem> *>(ptr);

    obs_source_t* src = obs_sceneitem_get_source(item);
    if (obs_source_removed(src)) {
        return true;
    }

    if (obs_sceneitem_is_group(item)) {
        OBSDataAutoRelease data =
            obs_sceneitem_get_private_settings(item);

        bool collapse = obs_data_get_bool(data, "collapsed");
        if (!collapse) {
            obs_scene_t* scene =
                obs_sceneitem_group_get_scene(item);

            obs_scene_enum_items(scene, enumItem, &items);
        }
    }

    items.insert(0, item);
    return true;
}

void AFQSourceViewModel::SceneChanged()
{
    AFSceneContext& contextScene = AFSceneContext::GetSingletonInstance();

    OBSScene scene = contextScene.GetCurrOBSScene();

    beginResetModel();
    m_vSourceList.clear();
    obs_scene_enum_items(scene, enumItem, &m_vSourceList);
    endResetModel();

    UpdateGroupState(false);
    m_sourceListView->ResetWidgets();

    bool findSelect = false;
    for (int i = 0; i < m_vSourceList.count(); i++) {
        bool select = obs_sceneitem_selected(m_vSourceList[i]);
        QModelIndex index = createIndex(i, 0);

        m_sourceListView->selectionModel()->select(
                index, select ? QItemSelectionModel::Select
                              : QItemSelectionModel::Deselect);

        if (select)
            findSelect = true;
    }

    emit m_sourceListView->qSignalCheckSourceClicked(findSelect);
}


void AFQSourceViewModel::ReorderItems()
{
    AFSceneContext& contextScene = AFSceneContext::GetSingletonInstance();
    OBSScene scene = contextScene.GetCurrOBSScene();

    QVector<OBSSceneItem> newitems;
    obs_scene_enum_items(scene, enumItem, &newitems);

    /* if item list has changed size, do full reset */
    if (newitems.count() != m_vSourceList.count()) {
        SceneChanged();
        return;
    }

    for (;;) {
        int idx1Old = 0;
        int idx1New = 0;
        int count;
        int i;

        /* find first starting changed item index */
        for (i = 0; i < newitems.count(); i++) {
            obs_sceneitem_t* oldItem = m_vSourceList[i];
            obs_sceneitem_t* newItem = newitems[i];
            if (oldItem != newItem) {
                idx1Old = i;
                break;
            }
        }

        /* if everything is the same, break */
        if (i == newitems.count()) {
            break;
        }

        /* find new starting index */
        for (i = idx1Old + 1; i < newitems.count(); i++) {
            obs_sceneitem_t* oldItem = m_vSourceList[idx1Old];
            obs_sceneitem_t* newItem = newitems[i];

            if (oldItem == newItem) {
                idx1New = i;
                break;
            }
        }

        /* if item could not be found, do full reset */
        if (i == newitems.count()) {
            SceneChanged();
            return;
        }

        /* get move count */
        for (count = 1; (idx1New + count) < newitems.count(); count++) {
            int oldIdx = idx1Old + count;
            int newIdx = idx1New + count;

            obs_sceneitem_t* oldItem = m_vSourceList[oldIdx];
            obs_sceneitem_t* newItem = newitems[newIdx];

            if (oldItem != newItem) {
                break;
            }
        }

        /* move items */
        beginMoveRows(QModelIndex(), idx1Old, idx1Old + count - 1,
            QModelIndex(), idx1New + count);
        for (i = 0; i < count; i++) {
            int to = idx1New + count;
            if (to > idx1Old)
                to--;
            MoveData(m_vSourceList, idx1Old, to);
        }
        endMoveRows();
    }
}


//int AFQSourceViewModel::Count()
//{
//    return m_vSourceList.count();
//}

void AFQSourceViewModel::Add(obs_sceneitem_t* item)
{
    if (obs_sceneitem_is_group(item)) {
        SceneChanged();
    }
    else {
        beginInsertRows(QModelIndex(), 0, 0);
        m_vSourceList.insert(0, item);
        endInsertRows();

        m_sourceListView->UpdateWidget(createIndex(0, 0, nullptr), item);
    }
}

void AFQSourceViewModel::Remove(obs_sceneitem_t* item)
{
    int idx = -1;
    for (int i = 0; i < m_vSourceList.count(); i++) {
        if (m_vSourceList[i] == item) {
            idx = i;
            break;
        }
    }

    if (idx == -1)
        return;

    int startIdx = idx;
    int endIdx = idx;

    bool is_group = obs_sceneitem_is_group(item);
    if (is_group) {
        obs_scene_t* scene = obs_sceneitem_group_get_scene(item);

        for (int i = endIdx + 1; i < m_vSourceList.count(); i++) {
            obs_sceneitem_t* subitem = m_vSourceList[i];
            obs_scene_t* subscene =
                obs_sceneitem_get_scene(subitem);

            if (subscene == scene)
                endIdx = i;
            else
                break;
        }
    }

    beginRemoveRows(QModelIndex(), startIdx, endIdx);
    m_vSourceList.remove(idx, endIdx - startIdx + 1);
    endRemoveRows();

    if (is_group)
        UpdateGroupState(true);
}

OBSSceneItem AFQSourceViewModel::Get(int idx)
{
    if (idx == -1 || idx >= m_vSourceList.count())
        return OBSSceneItem();
    return m_vSourceList[idx];
}


QString AFQSourceViewModel::GetNewGroupName()
{
    AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

    QString name = locale.Str("Group");

    int i = 2;
    for (;;) {
        OBSSourceAutoRelease group =
            obs_get_source_by_name(QT_TO_UTF8(name));
        if (!group)
            break;

        QString group_name = locale.Str("Basic.Main.Group");
        name = group_name.arg(QString::number(i++));
    }
    return name;
}

void AFQSourceViewModel::GroupSelectedItems(QModelIndexList& indices)
{
    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

    if (indices.count() == 0)
        return;

    AFMainFrame* main = App()->GetMainView();
    OBSScene scene = sceneContext.GetCurrOBSScene();
    QString name = GetNewGroupName();

    QVector<obs_sceneitem_t*> item_order;

    for (int i = indices.count() - 1; i >= 0; i--) {
        obs_sceneitem_t* item = m_vSourceList[indices[i].row()];
        item_order << item;
    }

    m_sourceListView->m_undoSceneData = main->BackupScene(scene);
    obs_sceneitem_t* item = obs_scene_insert_group(scene, QT_TO_UTF8(name),
                                                   item_order.data(), item_order.size());
    if (!item) {
        m_sourceListView->m_undoSceneData = nullptr;
        return;
    }

    main->m_undo_s.PushDisabled();

    m_hasGroups = true;
    m_sourceListView->UpdateWidgets(true);

    obs_sceneitem_select(item, true);

    /* ----------------------------------------------------------------- */
    /* obs_scene_insert_group triggers a full refresh of scene items via */
    /* the item_add signal. No need to insert a row, just edit the one   */
    /* that's created automatically.                                     */

    int newIdx = indices[0].row();
    QMetaObject::invokeMethod(m_sourceListView, "NewGroupEdit", Qt::QueuedConnection,
        Q_ARG(int, newIdx));
}

void AFQSourceViewModel::UngroupSelectedGroups(QModelIndexList& indices)
{
    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
    if (indices.count() == 0)
        return;

    OBSScene scene = sceneContext.GetCurrOBSScene();
    AFMainFrame* main = App()->GetMainView();
    OBSData undoData = main->BackupScene(scene);

    for (int i = indices.count() - 1; i >= 0; i--) {
        obs_sceneitem_t* item = m_vSourceList[indices[i].row()];
        obs_sceneitem_group_ungroup(item);
    }

    SceneChanged();

    OBSData redoData = main->BackupScene(scene);
    main->CreateSceneUndoRedoAction(QTStr("Basic.Main.Ungroup"), undoData, redoData);
}

void AFQSourceViewModel::ExpandGroup(obs_sceneitem_t* item)
{
    int itemIdx = m_vSourceList.indexOf(item);
    if (itemIdx == -1)
        return;

    itemIdx++;

    obs_scene_t* scene = obs_sceneitem_group_get_scene(item);

    QVector<OBSSceneItem> subItems;
    obs_scene_enum_items(scene, enumItem, &subItems);

    if (!subItems.size())
        return;

    beginInsertRows(QModelIndex(), itemIdx, itemIdx + subItems.size() - 1);
    for (int i = 0; i < subItems.size(); i++)
        m_vSourceList.insert(i + itemIdx, subItems[i]);
    endInsertRows();

    m_sourceListView->UpdateWidgets();
}

void AFQSourceViewModel::CollapseGroup(obs_sceneitem_t* item)
{
    int startIdx = -1;
    int endIdx = -1;

    obs_scene_t* scene = obs_sceneitem_group_get_scene(item);

    for (int i = 0; i < m_vSourceList.size(); i++) {
        obs_scene_t* itemScene = obs_sceneitem_get_scene(m_vSourceList[i]);

        if (itemScene == scene) {
            if (startIdx == -1)
                startIdx = i;
            endIdx = i;
        }
    }

    if (startIdx == -1)
        return;

    beginRemoveRows(QModelIndex(), startIdx, endIdx);
    m_vSourceList.remove(startIdx, endIdx - startIdx + 1);
    endRemoveRows();
}


void AFQSourceViewModel::UpdateGroupState(bool update)
{
    bool nowHasGroups = false;
    for (auto& item : m_vSourceList) {
        if (obs_sceneitem_is_group(item)) {
            nowHasGroups = true;
            break;
        }
    }

    if (nowHasGroups != m_hasGroups) {
        m_hasGroups = nowHasGroups;
        if (update) {
            m_sourceListView->UpdateWidgets(true);
        }
    }
}

bool AFQSourceListView::Edit(int row)
{
    AFQSourceViewModel* stm = GetStm();
    if (row < 0 || row >= stm->m_vSourceList.count())
        return false;

    QModelIndex index = stm->createIndex(row, 0);
    QWidget* widget = indexWidget(index);
    AFQSourceViewItem* itemWidget = reinterpret_cast<AFQSourceViewItem*>(widget);
    if (itemWidget->IsEditing()) {
#ifdef __APPLE__
        itemWidget->ExitEditMode(true);
#endif
        return false;
    }

    itemWidget->EnterEditMode();
    edit(index);
    return true;
}

void AFQSourceListView::Remove(OBSSceneItem item)
{
    GetStm()->Remove(item);

    //main->SaveProject();

    //if (!main->SavingDisabled()) {
    //    obs_scene_t* scene = obs_sceneitem_get_scene(item);
    //    obs_source_t* sceneSource = obs_scene_get_source(scene);
    //    obs_source_t* itemSource = obs_sceneitem_get_source(item);
    //    blog(LOG_INFO, "User Removed source '%s' (%s) from scene '%s'",
    //        obs_source_get_name(itemSource),
    //        obs_source_get_id(itemSource),
    //        obs_source_get_name(sceneSource));
    //}

    // Send Remove Source Info
    AFMainFrame* mainFrame = App()->GetMainView();
    if (!mainFrame)
        return;

    mainFrame->RecvRemovedSource(item);
}


void AFQSourceListView::ShowContextMenu(const QPoint& pos)
{
    QModelIndex idx = indexAt(pos);
    App()->GetMainView()->CreateSourcePopupMenu(idx.row());
}

void AFQSourceListView::GroupSelectedItems()
{
    QModelIndexList indices = selectedIndexes();
    std::sort(indices.begin(), indices.end());
    GetStm()->GroupSelectedItems(indices);
}

void AFQSourceListView::UngroupSelectedGroups()
{
    QModelIndexList indices = selectedIndexes();
    GetStm()->UngroupSelectedGroups(indices);
}

void AFQSourceListView::NewGroupEdit(int idx)
{
    if(!Edit(idx)) {
        AFMainFrame* main = App()->GetMainView();
        main->m_undo_s.PopDisabled();

        blog(LOG_WARNING,
             "Uh, somehow the edit didn't process, this "
             "code should never be reached.\nAnd by "
             "\"never be reached\", I mean that "
             "theoretically, it should be\nimpossible "
             "for this code to be reached. But if this "
             "code is reached,\nfeel free to laugh at "
             "Lain, because apparently it is, in fact, "
             "actually\npossible for this code to be "
             "reached. But I mean, again, theoretically\n"
             "it should be impossible. So if you see "
             "this in your log, just know that\nit's "
             "really dumb, and depressing. But at least "
             "the undo/redo action is\nstill covered, so "
             "in theory things *should* be fine. But "
             "it's entirely\npossible that they might "
             "not be exactly. But again, yea. This "
             "really\nshould not be possible.");

        OBSData redoSceneData = main->BackupScene(AFSceneContext::GetSingletonInstance().GetCurrOBSScene());
        QString text = QTStr("Undo.GroupItems").arg("Unknown");
        main->CreateSceneUndoRedoAction(text, m_undoSceneData, redoSceneData);

        m_undoSceneData = nullptr;
    }
}

void AFQSourceListView::UpdateNoSourcesMessage()
{
    QTextOption opt(Qt::AlignHCenter);
    opt.setWrapMode(QTextOption::NoWrap);
    m_textNoSources.setTextOption(opt);

    QString message = Str("Basic.SceneSourceDock.EmptySourceList");
    m_textNoSources.setText(message.replace("\n", "<br/>"));
    
    m_textPrepared = false;
}


void AFQSourceListView::qSlotHideScrollBar()
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    if(m_timerScrollVisible)
        m_timerScrollVisible->stop();
}

AFQSourceListView::AFQSourceListView(QWidget* parent)
{
    AFQSourceViewModel* model = new AFQSourceViewModel(this);
    setModel(model);

    NoFocusAndIndicatorColorDelegate* delegate = new NoFocusAndIndicatorColorDelegate(this);
    setItemDelegate(delegate);

    setFocusPolicy(Qt::NoFocus);
    setDefaultDropAction(Qt::MoveAction);
    setAcceptDrops(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDropIndicatorShown(true);

    this->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(ShowContextMenu(QPoint)));

    setFocusPolicy(Qt::StrongFocus);

    UpdateNoSourcesMessage();

    //
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    //m_timerScrollVisible = new QTimer(this);
    //m_timerScrollVisible->setInterval(200);
    //m_timerScrollVisible->setSingleShot(true);
    //connect(m_timerScrollVisible, &QTimer::timeout,
    //        this, &AFQSourceListView::qSlotHideScrollBar);
}

void AFQSourceListView::Clear()
{
    //m_vpreviouseSelectedSourceItems.clear();
    GetStm()->Clear();
}

int AFQSourceListView::GetTopSelectedSourceItem()
{
    QModelIndexList selectedItems = selectionModel()->selectedIndexes();

    return selectedItems.count() ? selectedItems[0].row() : -1;
}

void AFQSourceListView::ResetWidgets()
{
    AFQSourceViewModel* stm = GetStm();

    for (int i = 0; i < stm->m_vSourceList.count(); i++) {
        QModelIndex index = stm->createIndex(i, 0, nullptr);
        setIndexWidget(index, new AFQSourceViewItem(this, stm->m_vSourceList[i]));
    }
}

void AFQSourceListView::UpdateWidget(const QModelIndex& idx, obs_sceneitem_t* item)
{
    setIndexWidget(idx, new AFQSourceViewItem(this, item));
}

void AFQSourceListView::UpdateWidgets(bool force)
{
    AFQSourceViewModel* stm = GetStm();

    for (int i = 0; i < stm->m_vSourceList.size(); i++) {
        obs_sceneitem_t* item = stm->m_vSourceList[i];
        AFQSourceViewItem* widget = GetItemWidget(i);

        if (!widget) {
            UpdateWidget(stm->createIndex(i, 0), item);
        }
        else {
            widget->Update(force);
        }
    }
}

void AFQSourceListView::RefreshSourceItem()
{
    AFQSourceViewModel* stm = GetStm();
    stm->SceneChanged();
}

void AFQSourceListView::_ShowVerticalScrollBar()
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    if(m_timerScrollVisible)
        m_timerScrollVisible->start();
}

void AFQSourceListView::SelectItem(obs_sceneitem_t* sceneitem, bool select)
{
    AFQSourceViewModel* stm = GetStm();
    int i = 0;

    for (; i < stm->m_vSourceList.count(); i++) {
        if (stm->m_vSourceList[i] == sceneitem)
            break;
    }

    if (i == stm->m_vSourceList.count())
        return;

    QModelIndex index = stm->createIndex(i, 0);
    if (index.isValid() && select != selectionModel()->isSelected(index))
        selectionModel()->select(
            index, select ? QItemSelectionModel::Select
            : QItemSelectionModel::Deselect);
}

bool AFQSourceListView::MultipleBaseSelected() const
{
    AFQSourceViewModel* stm = GetStm();
    QModelIndexList selectedIndices = selectedIndexes();

    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
    OBSScene scene = sceneContext.GetCurrOBSScene();

    if (selectedIndices.size() < 1) {
        return false;
    }

    for (auto& idx : selectedIndices) {
        obs_sceneitem_t* item = stm->m_vSourceList[idx.row()];
        if (obs_sceneitem_is_group(item)) {
            return false;
        }

        obs_scene* itemScene = obs_sceneitem_get_scene(item);
        if (itemScene != scene) {
            return false;
        }
    }

    return true;
}

bool AFQSourceListView::GroupsSelected() const
{
    AFQSourceViewModel* stm = GetStm();
    QModelIndexList selectedIndices = selectedIndexes();

    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
    OBSScene scene = sceneContext.GetCurrOBSScene();

    if (selectedIndices.size() < 1) {
        return false;
    }

    for (auto& idx : selectedIndices) {
        obs_sceneitem_t* item = stm->m_vSourceList[idx.row()];
        if (!obs_sceneitem_is_group(item)) {
            return false;
        }
    }

    return true;
}

void AFQSourceListView::RegisterShortCut(QAction* removeSourceAction)
{
    if (!removeSourceAction)
        return;

    addAction(removeSourceAction);
}

void AFQSourceListView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        QListView::mouseDoubleClickEvent(event);
}


void AFQSourceListView::dragMoveEvent(QDragMoveEvent* event)
{
    //_ShowVerticalScrollBar();

    QListView::dragMoveEvent(event);
}

void AFQSourceListView::dropEvent(QDropEvent *event)
{
    if(event->source() != this) {
        QListView::dropEvent(event);
        return;
    }

    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

    OBSScene scene = sceneContext.GetCurrOBSScene();
    obs_source_t* scenesource = obs_scene_get_source(scene);
    AFQSourceViewModel* stm = GetStm();
    auto& items = stm->m_vSourceList;
    QModelIndexList indices = selectedIndexes();

    DropIndicatorPosition indicator = dropIndicatorPosition();
    int row = indexAt(event->position().toPoint()).row();
    bool emptyDrop = row == -1;

    if (emptyDrop) {
        if (!items.size()) {
            QListView::dropEvent(event);
            return;
        }

        row = items.size() - 1;
        indicator = QAbstractItemView::BelowItem;
    }

    /* --------------------------------------- */
    /* store destination group if moving to a  */
    /* group                                   */

    obs_sceneitem_t* dropItem = items[row]; /* item being dropped on */
    bool itemIsGroup = obs_sceneitem_is_group(dropItem);

    obs_sceneitem_t* dropGroup =
        itemIsGroup ? dropItem
        : obs_sceneitem_get_group(scene, dropItem);

    /* not a group if moving above the group */
    if (indicator == QAbstractItemView::AboveItem && itemIsGroup)
        dropGroup = nullptr;
    if (emptyDrop)
        dropGroup = nullptr;

    /* --------------------------------------- */
    /* remember to remove list items if        */
    /* dropping on collapsed group             */

    bool dropOnCollapsed = false;
    if (dropGroup) {
        obs_data_t* data =
            obs_sceneitem_get_private_settings(dropGroup);
        dropOnCollapsed = obs_data_get_bool(data, "collapsed");
        obs_data_release(data);
    }

    if (indicator == QAbstractItemView::BelowItem ||
        indicator == QAbstractItemView::OnItem ||
        indicator == QAbstractItemView::OnViewport)
        row++;

    if (row < 0 || row > stm->m_vSourceList.count()) {
        QListView::dropEvent(event);
        return;
    }

    /* --------------------------------------- */
    /* determine if any base group is selected */

    bool hasGroups = false;
    for (int i = 0; i < indices.size(); i++) {
        obs_sceneitem_t* item = items[indices[i].row()];
        if (obs_sceneitem_is_group(item)) {
            hasGroups = true;
            break;
        }
    }

    /* --------------------------------------- */
    /* if dropping a group, detect if it's     */
    /* below another group                     */

    obs_sceneitem_t* itemBelow;
    if (row == stm->m_vSourceList.count())
        itemBelow = nullptr;
    else
        itemBelow = stm->m_vSourceList[row];

    if (hasGroups) {
        if (!itemBelow ||
            obs_sceneitem_get_group(scene, itemBelow) != dropGroup) {
            dropGroup = nullptr;
            dropOnCollapsed = false;
        }
    }

    /* --------------------------------------- */
    /* if dropping groups on other groups,     */
    /* disregard as invalid drag/drop          */

    if (dropGroup && hasGroups) {
        QListView::dropEvent(event);
        return;
    }

    AFMainFrame* main = App()->GetMainView();
    /* --------------------------------------- */
    /* save undo data                          */
    std::vector<obs_source_t*> sources;
    for (int i = 0; i < indices.size(); i++)
    {
        obs_sceneitem_t* item = items[indices[i].row()];
        if (obs_sceneitem_get_scene(item) != scene)
            sources.push_back(obs_scene_get_source(obs_sceneitem_get_scene(item)));
    }
    if (dropGroup)
        sources.push_back(obs_sceneitem_get_source(dropGroup));
    OBSData undo_data = main->BackupScene(scene, &sources);

    /* --------------------------------------- */
    /* if selection includes base group items, */
    /* include all group sub-items and treat   */
    /* them all as one                         */

    if (hasGroups) {
        /* remove sub-items if selected */
        for (int i = indices.size() - 1; i >= 0; i--) {
            obs_sceneitem_t* item = items[indices[i].row()];
            obs_scene_t* itemScene = obs_sceneitem_get_scene(item);

            if (itemScene != scene) {
                indices.removeAt(i);
            }
        }

        /* add all sub-items of selected groups */
        for (int i = indices.size() - 1; i >= 0; i--) {
            obs_sceneitem_t* item = items[indices[i].row()];

            if (obs_sceneitem_is_group(item)) {
                for (int j = items.size() - 1; j >= 0; j--) {
                    obs_sceneitem_t* subitem = items[j];
                    obs_sceneitem_t* subitemGroup =
                        obs_sceneitem_get_group(
                            scene, subitem);

                    if (subitemGroup == item) {
                        QModelIndex idx =
                            stm->createIndex(j, 0);
                        indices.insert(i + 1, idx);
                    }
                }
            }
        }
    }

    /* --------------------------------------- */
    /* build persistent indices                */

    QList<QPersistentModelIndex> persistentIndices;
    persistentIndices.reserve(indices.count());
    for (QModelIndex& index : indices)
        persistentIndices.append(index);
    std::sort(persistentIndices.begin(), persistentIndices.end());

    /* --------------------------------------- */
    /* move all items to destination index     */

    int r = row;
    for (auto& persistentIdx : persistentIndices) {
        int from = persistentIdx.row();
        int to = r;
        int itemTo = to;

        if (itemTo > from)
            itemTo--;

        if (itemTo != from) {
            stm->beginMoveRows(QModelIndex(), from, from,
                QModelIndex(), to);
            MoveData(items, from, itemTo);
            stm->endMoveRows();
        }

        r = persistentIdx.row() + 1;
    }

    std::sort(persistentIndices.begin(), persistentIndices.end());
    int firstIdx = persistentIndices.front().row();
    int lastIdx = persistentIndices.back().row();

    /* --------------------------------------- */
    /* reorder scene items in back-end         */

    QVector<struct obs_sceneitem_order_info> orderList;
    obs_sceneitem_t* lastGroup = nullptr;
    int insertCollapsedIdx = 0;

    auto insertCollapsed = [&](obs_sceneitem_t* item) {
        struct obs_sceneitem_order_info info;
        info.group = lastGroup;
        info.item = item;

        orderList.insert(insertCollapsedIdx++, info);
    };

    using insertCollapsed_t = decltype(insertCollapsed);

    auto preInsertCollapsed = [](obs_scene_t*, obs_sceneitem_t* item,
        void* param) {
            (*reinterpret_cast<insertCollapsed_t*>(param))(item);
            return true;
    };

    auto insertLastGroup = [&]() {
        OBSDataAutoRelease data =
            obs_sceneitem_get_private_settings(lastGroup);
        bool collapsed = obs_data_get_bool(data, "collapsed");

        if (collapsed) {
            insertCollapsedIdx = 0;
            obs_sceneitem_group_enum_items(lastGroup,
                preInsertCollapsed,
                &insertCollapsed);
        }

        struct obs_sceneitem_order_info info;
        info.group = nullptr;
        info.item = lastGroup;
        orderList.insert(0, info);
    };

    auto updateScene = [&]() {
        struct obs_sceneitem_order_info info;

        for (int i = 0; i < items.size(); i++) {
            obs_sceneitem_t* item = items[i];
            obs_sceneitem_t* group;

            if (obs_sceneitem_is_group(item)) {
                if (lastGroup) {
                    insertLastGroup();
                }
                lastGroup = item;
                continue;
            }

            if (!hasGroups && i >= firstIdx && i <= lastIdx)
                group = dropGroup;
            else
                group = obs_sceneitem_get_group(scene, item);

            if (lastGroup && lastGroup != group) {
                insertLastGroup();
            }

            lastGroup = group;

            info.group = group;
            info.item = item;
            orderList.insert(0, info);
        }

        if (lastGroup) {
            insertLastGroup();
        }

        obs_scene_reorder_items2(scene, orderList.data(),
            orderList.size());
    };

    using updateScene_t = decltype(updateScene);

    auto preUpdateScene = [](void* data, obs_scene_t*) {
        (*reinterpret_cast<updateScene_t*>(data))();
    };

    m_isIgnoreReorder = true;
    obs_scene_atomic_update(scene, preUpdateScene, &updateScene);
    m_isIgnoreReorder = false;

    /* --------------------------------------- */
    /* save redo data                          */

    OBSData redo_data = main->BackupScene(scene, &sources);

    /* --------------------------------------- */
    /* add undo/redo action                    */

    const char* scene_name = obs_source_get_name(scenesource);
    QString action_name = QTStr("Undo.ReorderSources").arg(scene_name);
    main->CreateSceneUndoRedoAction(action_name, undo_data, redo_data);

    /* --------------------------------------- */
    /* remove items if dropped in to collapsed */
    /* group                                   */
    if (dropOnCollapsed) {
        stm->beginRemoveRows(QModelIndex(), firstIdx, lastIdx);
        items.remove(firstIdx, lastIdx - firstIdx + 1);
        stm->endRemoveRows();
    }

    /* --------------------------------------- */
    /* update widgets and accept event         */

    UpdateWidgets(true);

    event->accept();
    event->setDropAction(Qt::CopyAction);

    QListView::dropEvent(event);
}

void AFQSourceListView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    bool clicked = false;
    {
        SignalBlocker sourcesSignalBlocker(this);
        AFQSourceViewModel* stm = GetStm();

        QModelIndexList selectedIdxs = selected.indexes();
        QModelIndexList deselectedIdxs = deselected.indexes();

        for (int i = 0; i < selectedIdxs.count(); i++) {
            int idx = selectedIdxs[i].row();
            obs_sceneitem_select(stm->m_vSourceList[idx], true);

            AFQSourceViewItem* item = GetItemWidget(selectedIdxs[i].row());
            if (item)
                item->SetSelected(true);
        }

        for (int i = 0; i < deselectedIdxs.count(); i++) {
            int idx = deselectedIdxs[i].row();
            obs_sceneitem_select(stm->m_vSourceList[idx], false);

            AFQSourceViewItem* item = GetItemWidget(deselectedIdxs[i].row());
            if(item)
                item->SetSelected(false);
        }

        clicked = (0 == selectedIdxs.count() ? false : true);
    }

    emit qSignalCheckSourceClicked(clicked);

    QListView::selectionChanged(selected, deselected);
}

void AFQSourceListView::paintEvent(QPaintEvent* event)
{
    AFQSourceViewModel* stm = GetStm();
    if (stm && !stm->m_vSourceList.count()) {
        QPainter p(viewport());

        QColor textColor(200, 200, 200); 
        p.setPen(textColor);

        if (!m_textPrepared) {
            m_textNoSources.prepare(QTransform(), p.font());
            m_textPrepared = true;
        }

        //QRectF iconRect = iconNoSources.viewBoxF();
        //iconRect.setSize(QSizeF(32.0, 32.0));

        QSizeF iconSize = QSizeF(0, 0);//iconRect.size();
        QSizeF textSize = m_textNoSources.size();
        QSizeF thisSize = size();
        const qreal spacing = 5.0;

        qreal totalHeight =
            iconSize.height() + spacing + textSize.height();

        qreal x = thisSize.width() / 2.0 - iconSize.width() / 2.0;
        qreal y = thisSize.height() / 2.0 - totalHeight / 2.0;
        //iconRect.moveTo(std::round(x), std::round(y));
        //iconNoSources.render(&p, iconRect);

        x = thisSize.width() / 2.0 - textSize.width() / 2.0 - 10;
        //y += spacing + iconSize.height();
        p.drawStaticText(x, y, m_textNoSources);
    }
    else {
        QListView::paintEvent(event);
    }
}

void AFQSourceListView::wheelEvent(QWheelEvent* event)
{
    //_ShowVerticalScrollBar();

    QListView::wheelEvent(event);
}