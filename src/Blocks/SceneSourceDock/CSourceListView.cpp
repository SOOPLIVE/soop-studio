#include "CSourceListView.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <QDropEvent>
#include <QPainter>
#include <QPushButton>

#include "qt-wrappers.hpp"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Icon/CIconContext.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/OBSOutput/COutput.h"

#include "Application/CApplication.h"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/SceneSource/CMainSceneSource.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"

#define DESELECT_VISIBLE_ITEM_FONT_COLOR        QColor(213, 215, 220, 255)
#define SELECT_VISIBLE_ITEM_FONT_COLOR          QColor(0, 163, 255, 255)

#define DESELECT_UNVISIBLE_ITEM_FONT_COLOR      QColor(117, 123, 138, 255)
#define SELECT_UNVISIBLE_ITEM_FONT_COLOR        QColor(13, 79, 143, 255)

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
    m_pSourceListView(sourceListView),
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
    m_pLayoutBox = new QHBoxLayout(this);
    m_pLayoutBox->setContentsMargins(0, 0, 5, 0);
    m_pLayoutBox->setSpacing(0);

    m_pLabelIcon = _CreateIconLabel(id);
    m_pLabelName = _CreateNameLabel(name);
    if (!AFSourceUtil::IsSoopMediaSource(source)) {
        m_visibleCheckBox = _CreateVisibleCheckBox();
    }
    m_lockCheckBox  = _CreateLockedCheckBox();

    if (m_pLabelIcon) {
        m_pLayoutBox->addWidget(m_pLabelIcon);
        m_pLayoutBox->addSpacing(6);
    }
    m_pLayoutBox->addWidget(m_pLabelName);
    if(m_visibleCheckBox)
        m_pLayoutBox->addWidget(m_visibleCheckBox);
    m_pLayoutBox->addWidget(m_lockCheckBox);

    Update(false);

    setLayout(m_pLayoutBox);

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

        UNDO_STACK.AddAction(str.arg(obs_source_get_name(source), name),
                             std::bind(undo_redo, std::placeholders::_1, id, !val),
                             std::bind(undo_redo, std::placeholders::_1, id, val), uuid, uuid);

        QSignalBlocker sourceSignalBlocker(this);
        obs_sceneitem_set_visible(m_sceneItem, val);
    };

    auto setItemLocked = [this](bool checked) {
        QSignalBlocker sourcesSignalBlocker(this);
        obs_sceneitem_set_locked(m_sceneItem, checked);
    };

    connect(m_visibleCheckBox, &QAbstractButton::clicked, this, &AFQSourceViewItem::ClickedItemVisible);
    connect(m_lockCheckBox, &QAbstractButton::clicked, this, &AFQSourceViewItem::ClickedItemLocked);
}

void AFQSourceViewItem::Clear()
{
    DisconnectSignals();
    m_pSourceListView = nullptr;
    m_sceneItem = nullptr;
}

void AFQSourceViewItem::EnterEditMode()
{
    setFocusPolicy(Qt::StrongFocus);
    int index = m_pLayoutBox->indexOf(m_pLabelName);
    m_pLayoutBox->removeWidget(m_pLabelName);
    m_pEditorName = new QLineEdit(m_pLabelName->text(),this);
    m_pEditorName->setStyleSheet("background-color:#2F3238;   \
                                 border:1px solid #AAA;      \
                                 color:rgba(255,255,255,90%);\
                                 font-size:13px;             \
                                 font-style: normal;         \
                                 font-weight: 400;           \
                                 padding-left:2px; ");
    m_pEditorName->selectAll();
    m_pEditorName->installEventFilter(this);
    m_pLayoutBox->insertWidget(index, m_pEditorName);
    setFocusProxy(m_pEditorName);
}

void AFQSourceViewItem::ExitEditMode(bool save)
{
    ExitEditModeInternal(save);

    if (m_pSourceListView->m_undoSceneData) {
        UNDO_STACK.PopDisabled();

        OBSData redoSceneData = MAINFRAME->BackupScene(SCENE_CONTEXT.GetCurrentScene());
        QString text = QTStr("Undo.GroupItems").arg(m_newName.c_str());
        MAINFRAME->CreateSceneUndoRedoAction(text, m_pSourceListView->m_undoSceneData, redoSceneData);

        m_pSourceListView->m_undoSceneData = nullptr;
    }
}

void AFQSourceViewItem::ClickedItemVisible(bool val)
{
    if (!m_sceneItem)
        return;

    obs_scene_t* scene = obs_sceneitem_get_scene(m_sceneItem);
    if (!scene)
        return;

    obs_source_t* sceneSource = obs_scene_get_source(scene);
    if (!sceneSource)
        return;

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

    QString str = Str(val ? "Undo.ShowSceneItem" : "Undo.HideSceneItem");

    UNDO_STACK.AddAction(str.arg(obs_source_get_name(sceneSource), name),
                         std::bind(undo_redo, std::placeholders::_1, id, !val),
                         std::bind(undo_redo, std::placeholders::_1, id, val),
                         uuid, uuid);

    QSignalBlocker sourcesSignalBlocker(this);
    obs_sceneitem_set_visible(m_sceneItem, val);

    // 
    obs_source_t* src = obs_sceneitem_get_source(m_sceneItem);
    QString src_name = QString("%1").arg(obs_source_get_id(src));

    if (src_name == "soop_chat_source_mood_check") {
        
        auto broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
        if (val) {
            int nCnt = config_get_int(USERCONFIG, "SARSA", "UseMinsimCheckCnt");
            config_set_int(USERCONFIG, "SARSA", "UseMinsimCheckCnt", ++nCnt);
            if (nCnt == 1 && AFOutputUtil::IsStreamActive()) {
                auto startTime = std::chrono::steady_clock::now();
                AUTH_CONTEXT.SetMinsimCheckStartTime(startTime);
            }
        }
        else {
            int nCnt = config_get_int(USERCONFIG, "SARSA", "UseMinsimCheckCnt");
            config_set_int(USERCONFIG, "SARSA", "UseMinsimCheckCnt", --nCnt < 0 ? 0 : nCnt);

            if (nCnt <= 0 && AFOutputUtil::IsStreamActive()) {
                auto endTime = std::chrono::steady_clock::now();
                auto startTime = AUTH_CONTEXT.GetMinsimCheckStartTime();
                if (startTime != std::chrono::steady_clock::time_point{}) {
                    AUTH_CONTEXT.InitMinsimCheckStartTime();
                }
            }
        }
    }
}

void AFQSourceViewItem::ClickedItemLocked(bool val)
{
    if (!m_sceneItem)
        return;

    QSignalBlocker sourcesSignalBlocker(this);
    obs_sceneitem_set_locked(m_sceneItem, val);
}

void AFQSourceViewItem::ExpandClicked(bool checked)
{
    OBSDataAutoRelease data = obs_sceneitem_get_private_settings(m_sceneItem);

    obs_data_set_bool(data, "collapsed", checked);
    
    if (!checked)
        m_pSourceListView->GetStm()->ExpandGroup(m_sceneItem);
    else
        m_pSourceListView->GetStm()->CollapseGroup(m_sceneItem);
}


void AFQSourceViewItem::VisibilityChanged(bool visible)
{
    if(m_visibleCheckBox)
        m_visibleCheckBox->setChecked(visible);

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
    MAIN_SCENESOURCE->UpdateEditMenu();

    if (!m_lockCheckBox)
        return;

    if (locked)
        m_lockCheckBox->show();
    else {
        if(m_isHovered)
            m_lockCheckBox->show();
        else
            m_lockCheckBox->hide();
    }
}

void AFQSourceViewItem::Select()
{
    m_pSourceListView->SelectItem(m_sceneItem, true);
    MAINFRAME->UpdateContextToolBarDeferred();
    MAIN_SCENESOURCE->UpdateEditMenu();
}

void AFQSourceViewItem::DeSelect()
{
    m_pSourceListView->SelectItem(m_sceneItem, false);
    MAINFRAME->UpdateContextToolBarDeferred();
    MAIN_SCENESOURCE->UpdateEditMenu();
}

void AFQSourceViewItem::Renamed(QString name)
{
    if (m_pLabelName)
        m_pLabelName->setText(QT_UTF8(name.toStdString().c_str()));
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
    OBSScene scene = SCENE_CONTEXT.GetCurrentScene();
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

    if (m_pGroupSpacer) {
        m_pLayoutBox->removeItem(m_pGroupSpacer);
        delete m_pGroupSpacer;
        m_pGroupSpacer = nullptr;
    }

    if (m_type == Type::Group) {
        m_pLayoutBox->removeWidget(m_groupExpend);
        m_groupExpend->deleteLater();
        m_groupExpend = nullptr;
    }

    m_type = newType;

    if (m_type == Type::SubItem) {
        m_pGroupSpacer = new QSpacerItem(24, 1);
        m_pLayoutBox->insertItem(0, m_pGroupSpacer);

    }
    else if (m_type == Type::Group) {
        m_groupExpend = new QCheckBox();
        m_groupExpend->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        m_groupExpend->setObjectName("expandCheckBox");
        m_groupExpend->setMaximumSize(24, 24);
        m_groupExpend->setMinimumSize(24, 0);
#ifdef __APPLE__
        m_groupExpend->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif
        m_pLayoutBox->insertWidget(0, m_groupExpend);

        OBSDataAutoRelease data = obs_sceneitem_get_private_settings(m_sceneItem);
        m_groupExpend->blockSignals(true);
        m_groupExpend->setChecked(obs_data_get_bool(data, "collapsed"));
        m_groupExpend->blockSignals(false);

        connect(m_groupExpend, &QPushButton::toggled, this, &AFQSourceViewItem::ExpandClicked);
    }
    else {
        m_pGroupSpacer = new QSpacerItem(3, 1);
        m_pLayoutBox->insertItem(0, m_pGroupSpacer);
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

    if(m_pLabelIcon)
        m_pLabelIcon->setPixmap(pixmapIconSource);

    setStyleSheet(QString(SOURCEVIEW_ITEM_STYLE).arg(
                  m_colorBackground.name(QColor::HexArgb),
                  m_colorFont.name(QColor::HexArgb)));
}

bool AFQSourceViewItem::IsEditing()
{
    return m_pEditorName != nullptr;
}

QColor AFQSourceViewItem::GetBackgroundColor()
{
    return m_colorBackground;
}

QLabel* AFQSourceViewItem::_CreateIconLabel(const char* id)
{
    QLabel* iconLabel = new QLabel(this);
    iconLabel->setFixedSize(24, 24);
    iconLabel->setObjectName("sourceIconLabel");


    if (strcmp(id, "scene") == 0)
        m_iconSource = ICON_CONTEXT.GetSceneIcon();
    else if (strcmp(id, "group") == 0)
        m_iconSource = ICON_CONTEXT.GetGroupIcon();
    else
        m_iconSource = ICON_CONTEXT.GetSourceIcon(id);

    QPixmap pixmap = m_iconSource.pixmap(QSize(24, 24));
    iconLabel->setPixmap(pixmap);
    iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    iconLabel->setStyleSheet("background:none;");

    return iconLabel;
}

QLabel* AFQSourceViewItem::_CreateNameLabel(const char* name)
{
    QLabel* nameLabel = new QLabel(this);

    QString adjustName = name;
    adjustName.replace("\n", " ");

    nameLabel->setText(adjustName);
    nameLabel->setObjectName("sourceNameLabel"); 
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    nameLabel->setAttribute(Qt::WA_TranslucentBackground);
    nameLabel->setMinimumWidth(0);

    return nameLabel;
}


QCheckBox* AFQSourceViewItem::_CreateVisibleCheckBox()
{
    m_isVisible = obs_sceneitem_visible(m_sceneItem);

    QCheckBox* vis = new QCheckBox();
    vis->setFixedSize(QSize(24, 24));
    vis->setStyleSheet("background: none");
    vis->setObjectName("visibleCheckBox");
    vis->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    vis->setChecked(m_isVisible);
    vis->hide();

    if (m_isVisible)
        SetFontColor(DESELECT_VISIBLE_ITEM_FONT_COLOR);
	else
        SetFontColor(DESELECT_UNVISIBLE_ITEM_FONT_COLOR);

    return vis;
}

QCheckBox* AFQSourceViewItem::_CreateLockedCheckBox()
{
    bool locked = obs_sceneitem_locked(m_sceneItem);

    QCheckBox* lock = new QCheckBox();
    lock->setFixedSize(QSize(24, 24));
    lock->setStyleSheet("background: none");
    lock->setObjectName("lockCheckBox");
    lock->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    lock->setChecked(locked);
    if(!locked)
        lock->hide();

    return lock;
}

void AFQSourceViewItem::ExitEditModeInternal(bool save)
{
    if (!m_pEditorName) {
        return;
    }

    OBSScene scene = SCENE_CONTEXT.GetCurrentScene();

    m_newName = QT_TO_UTF8(m_pEditorName->text());

    setFocusProxy(nullptr);
    int index = m_pLayoutBox->indexOf(m_pEditorName);
    m_pLayoutBox->removeWidget(m_pEditorName);
    delete m_pEditorName;
    m_pEditorName = nullptr;
    setFocusPolicy(Qt::NoFocus);
    m_pLayoutBox->insertWidget(index, m_pLabelName);
    setFocus();

    /* ----------------------------------------- */
    /* check for empty string                    */

    if (!save)
        return;

    if (m_newName.empty()) {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
            QTStr("NoNameEntered.Title"),
            QTStr("NoNameEntered.Text"));
        return;
    }

    /* ----------------------------------------- */
    /* Check for same name                       */

    obs_source_t* source = obs_sceneitem_get_source(m_sceneItem);
    if (m_newName == obs_source_get_name(source))
        return;

    /* ----------------------------------------- */
    /* check for existing source                 */

    OBSSourceAutoRelease existingSource = obs_get_source_by_name(m_newName.c_str());
    bool exists = !!existingSource;

    if (exists) {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
            QTStr("NameExists.Title"),
            QTStr("NameExists.Text"));
        return;
    }

    /* ----------------------------------------- */
    /* rename                                    */

    QSignalBlocker sourcesSignalBlocker(this);
    std::string prevName(obs_source_get_name(source));
    std::string scene_uuid = obs_source_get_uuid(SCENE_CONTEXT.GetCurrentSceneSource());
    auto undo = [scene_uuid, prevName](const std::string& data) {
        OBSSourceAutoRelease source = obs_get_source_by_uuid(data.c_str());
        obs_source_set_name(source, prevName.c_str());
        OBSSourceAutoRelease scene_source = obs_get_source_by_uuid(scene_uuid.c_str());
        DYNAMIC_COMPOSIT->SetCurrentScene(scene_source.Get(), true);
    };

    std::string editedName = m_newName;

    auto redo = [scene_uuid, editedName](const std::string& data) {
        OBSSourceAutoRelease source = obs_get_source_by_uuid(data.c_str());
        obs_source_set_name(source, editedName.c_str());

        OBSSourceAutoRelease scene_source = obs_get_source_by_uuid(scene_uuid.c_str());
        DYNAMIC_COMPOSIT->SetCurrentScene(scene_source.Get(), true);
    };

    const char* uuid = obs_source_get_uuid(source);
    UNDO_STACK.AddAction(QTStr("Undo.Rename").arg(m_newName.c_str()), undo, redo, uuid, uuid);
    
    obs_data_t* settings = obs_source_get_settings(source);
    obs_data_set_bool(settings, "is_changed_name", true);

    obs_source_set_name(source, m_newName.c_str());
    m_pLabelName->setText(QT_UTF8(m_newName.c_str()));
}

void AFQSourceViewItem::CallbackRemoveItem(void* data, calldata_t* cd)
{
    AFQSourceViewItem* sourceItem = reinterpret_cast<AFQSourceViewItem*>(data);
    obs_sceneitem_t* curItem =
        (obs_sceneitem_t*)calldata_ptr(cd, "item");

    if (curItem == sourceItem->m_sceneItem) {
        QMetaObject::invokeMethod(sourceItem->m_pSourceListView, "Remove",
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
    AFQSourceViewItem* sourceItem = reinterpret_cast<AFQSourceViewItem*>(data);
    obs_sceneitem_t* curItem = (obs_sceneitem_t*)calldata_ptr(cd, "item");

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
    QMetaObject::invokeMethod(sourceItem->m_pSourceListView, "ReorderItems");
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
    QMetaObject::invokeMethod(sourceItem->m_pSourceListView, "RefreshItems");
}

void AFQSourceViewItem::mouseDoubleClickEvent(QMouseEvent* event)
{
    QWidget::mouseDoubleClickEvent(event);

    if (m_groupExpend) {
        m_groupExpend->setChecked(!m_groupExpend->isChecked());
    }
    else {
        obs_source_t* source = obs_sceneitem_get_source(m_sceneItem);

        if (AFSourceUtil::ShouldShowProperties(source)) {
            MAINFRAME->CreateSourceProperties(source);
        }
    }
}

void AFQSourceViewItem::enterEvent(QEnterEvent* event)
{
    if(m_visibleCheckBox)
        m_visibleCheckBox->show();

    if(m_lockCheckBox)
        m_lockCheckBox->show();

    if (!m_isSelected && m_isVisible) {
        SetFontColor(HOVERED_VISIBLE_ITEM_FONT_COLOR);
    }

    m_isHovered = true;
}

void AFQSourceViewItem::leaveEvent(QEvent* event)
{
    if (m_visibleCheckBox)
        m_visibleCheckBox->hide();

    if (m_lockCheckBox) {
        if(!obs_sceneitem_locked(m_sceneItem))
            m_lockCheckBox->hide();
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
    if (m_pEditorName != object)
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
    , m_pSourceListView(parent)
{

}

int AFQSourceViewModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_sourceList.count();
}

QVariant AFQSourceViewModel::data(const QModelIndex& index, int role) const
{
    //if (role == Qt::AccessibleTextRole) {
    //    OBSSceneItem item = m_sourceList[index.row()];
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

    obs_sceneitem_t* item = m_sourceList[index.row()];
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
    m_sourceList.clear();
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
    bool hadMediaSource = false;
    for (int i = 0; i < m_sourceList.count(); i++) {
        obs_source_t* source = obs_sceneitem_get_source(m_sourceList[i]);
        if (AFSourceUtil::IsSoopMediaSource(source))
        {
            hadMediaSource = true;
            break;
        }
    }

    OBSScene scene = SCENE_CONTEXT.GetCurrentScene();

    beginResetModel();
    m_sourceList.clear();
    obs_scene_enum_items(scene, enumItem, &m_sourceList);
    endResetModel();

    UpdateGroupState(false);
    m_pSourceListView->ResetWidgets();

    bool findSelect = false;
    for (int i = 0; i < m_sourceList.count(); i++) {
        if (!hadMediaSource)
        {
            obs_source_t* source = obs_sceneitem_get_source(m_sourceList[i]);
            if (AFSourceUtil::IsSoopMediaSource(source))
            {
            }
        }

        bool select = obs_sceneitem_selected(m_sourceList[i]);
        QModelIndex index = createIndex(i, 0);

        m_pSourceListView->selectionModel()->select(
                index, select ? QItemSelectionModel::Select
                              : QItemSelectionModel::Deselect);
    }
}


void AFQSourceViewModel::ReorderItems()
{
    OBSScene scene = SCENE_CONTEXT.GetCurrentScene();

    QVector<OBSSceneItem> newitems;
    obs_scene_enum_items(scene, enumItem, &newitems);

    /* if item list has changed size, do full reset */
    if (newitems.count() != m_sourceList.count()) {
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
            obs_sceneitem_t* oldItem = m_sourceList[i];
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
            obs_sceneitem_t* oldItem = m_sourceList[idx1Old];
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

            obs_sceneitem_t* oldItem = m_sourceList[oldIdx];
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
            MoveData(m_sourceList, idx1Old, to);
        }
        endMoveRows();
    }
}


//int AFQSourceViewModel::Count()
//{
//    return m_sourceList.count();
//}

void AFQSourceViewModel::Add(obs_sceneitem_t* item)
{
    if (obs_sceneitem_is_group(item)) {
        SceneChanged();
    }
    else {
        bool findTopFixedSource = false;
        for (int i = 0; i < m_sourceList.count(); i++) {
            obs_source_t* source = obs_sceneitem_get_source(m_sourceList[i]);
            if (source) {
                if (AFSourceUtil::IsMustTopSource(source)) {
                    findTopFixedSource = true;
                    break;
                }
            }
        }
        beginInsertRows(QModelIndex(), 0, 0);
        m_sourceList.insert(0, item);
        endInsertRows();

        m_pSourceListView->UpdateWidget(createIndex(0, 0, nullptr), item);

        if (findTopFixedSource) {
            obs_source_t* source = obs_sceneitem_get_source(item);
            if (!source)
                return;

            obs_sceneitem_set_order(item, OBS_ORDER_MOVE_DOWN);
        }
    }
}

void AFQSourceViewModel::Remove(obs_sceneitem_t* item)
{
    int idx = -1;
    for (int i = 0; i < m_sourceList.count(); i++) {
        if (m_sourceList[i] == item) {
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

        for (int i = endIdx + 1; i < m_sourceList.count(); i++) {
            obs_sceneitem_t* subitem = m_sourceList[i];
            obs_scene_t* subscene =
                obs_sceneitem_get_scene(subitem);

            if (subscene == scene)
                endIdx = i;
            else
                break;
        }
    }

    beginRemoveRows(QModelIndex(), startIdx, endIdx);
    m_sourceList.remove(idx, endIdx - startIdx + 1);
    endRemoveRows();

    if (is_group)
        UpdateGroupState(true);
}

OBSSceneItem AFQSourceViewModel::Get(int idx)
{
    if (idx == -1 || idx >= m_sourceList.count())
        return OBSSceneItem();
    return m_sourceList[idx];
}


QString AFQSourceViewModel::GetNewGroupName()
{
    QString name = Str("Group");

    int i = 2;
    for (;;) {
        OBSSourceAutoRelease group =
            obs_get_source_by_name(QT_TO_UTF8(name));
        if (!group)
            break;

        QString group_name = Str("Basic.Main.Group");
        name = group_name.arg(QString::number(i++));
    }
    return name;
}

void AFQSourceViewModel::GroupSelectedItems(QModelIndexList& indices)
{
    if (indices.count() == 0)
        return;

    OBSScene scene = SCENE_CONTEXT.GetCurrentScene();
    QString name = GetNewGroupName();

    QVector<obs_sceneitem_t*> item_order;

    for (int i = indices.count() - 1; i >= 0; i--) {
        obs_sceneitem_t* item = m_sourceList[indices[i].row()];
        item_order << item;
    }

    m_pSourceListView->m_undoSceneData = MAINFRAME->BackupScene(scene);
    obs_sceneitem_t* item = obs_scene_insert_group(scene, QT_TO_UTF8(name), item_order.data(), item_order.size());
    if (!item) {
        m_pSourceListView->m_undoSceneData = nullptr;
        return;
    }

    UNDO_STACK.PushDisabled();

    m_hasGroups = true;
    m_pSourceListView->UpdateWidgets(true);

    obs_sceneitem_select(item, true);

    /* ----------------------------------------------------------------- */
    /* obs_scene_insert_group triggers a full refresh of scene items via */
    /* the item_add signal. No need to insert a row, just edit the one   */
    /* that's created automatically.                                     */

    int newIdx = indices[0].row();
    QMetaObject::invokeMethod(m_pSourceListView, "NewGroupEdit",
                              Qt::QueuedConnection, Q_ARG(int, newIdx));
}

void AFQSourceViewModel::UngroupSelectedGroups(QModelIndexList& indices)
{
    if (indices.count() == 0)
        return;

    OBSScene scene = SCENE_CONTEXT.GetCurrentScene();
    OBSData undoData = MAINFRAME->BackupScene(scene);
    for (int i = indices.count() - 1; i >= 0; i--) {
        obs_sceneitem_t* item = m_sourceList[indices[i].row()];
        obs_sceneitem_group_ungroup(item);
    }

    SceneChanged();

    OBSData redoData = MAINFRAME->BackupScene(scene);
    MAINFRAME->CreateSceneUndoRedoAction(QTStr("Basic.Main.Ungroup"), undoData, redoData);
}

void AFQSourceViewModel::ExpandGroup(obs_sceneitem_t* item)
{
    int itemIdx = m_sourceList.indexOf(item);
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
        m_sourceList.insert(i + itemIdx, subItems[i]);
    endInsertRows();

    m_pSourceListView->UpdateWidgets();
}

void AFQSourceViewModel::CollapseGroup(obs_sceneitem_t* item)
{
    int startIdx = -1;
    int endIdx = -1;

    obs_scene_t* scene = obs_sceneitem_group_get_scene(item);

    for (int i = 0; i < m_sourceList.size(); i++) {
        obs_scene_t* itemScene = obs_sceneitem_get_scene(m_sourceList[i]);

        if (itemScene == scene) {
            if (startIdx == -1)
                startIdx = i;
            endIdx = i;
        }
    }

    if (startIdx == -1)
        return;

    beginRemoveRows(QModelIndex(), startIdx, endIdx);
    m_sourceList.remove(startIdx, endIdx - startIdx + 1);
    endRemoveRows();
}


void AFQSourceViewModel::UpdateGroupState(bool update)
{
    bool nowHasGroups = false;
    for (auto& item : m_sourceList) {
        if (obs_sceneitem_is_group(item)) {
            nowHasGroups = true;
            break;
        }
    }

    if (nowHasGroups != m_hasGroups) {
        m_hasGroups = nowHasGroups;
        if (update) {
            m_pSourceListView->UpdateWidgets(true);
        }
    }
}

bool AFQSourceListView::Edit(int row)
{
    AFQSourceViewModel* stm = GetStm();
    if (row < 0 || row >= stm->m_sourceList.count())
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
    //OBSBasic* main = reinterpret_cast<OBSBasic*>(App()->GetMainWindow());
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
    if (MAINFRAME)
        MAINFRAME->RecvRemovedSource(item);
}


void AFQSourceListView::ShowContextMenu(const QPoint& pos)
{
    QModelIndex idx = indexAt(pos);
    MAIN_SCENESOURCE->CreateSourcePopupMenu(idx.row());
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
        UNDO_STACK.PopDisabled();

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

        OBSData redoSceneData = MAINFRAME->BackupScene(SCENE_CONTEXT.GetCurrentScene());
        QString text = QTStr("Undo.GroupItems").arg("Unknown");
        MAINFRAME->CreateSceneUndoRedoAction(text, m_undoSceneData, redoSceneData);

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
    _HideVeritcalScrollBar();

    //m_timerScrollVisible = new QTimer(this);
    //m_timerScrollVisible->setInterval(200);
    //m_timerScrollVisible->setSingleShot(true);
    //connect(m_timerScrollVisible, &QTimer::timeout,
    //        this, &AFQSourceListView::qSlotHideScrollBar);
}

void AFQSourceListView::Clear()
{
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

    for (int i = 0; i < stm->m_sourceList.count(); i++) {
        QModelIndex index = stm->createIndex(i, 0, nullptr);
        setIndexWidget(index, new AFQSourceViewItem(this, stm->m_sourceList[i]));
    }
}

void AFQSourceListView::UpdateWidget(const QModelIndex& idx, obs_sceneitem_t* item)
{
    setIndexWidget(idx, new AFQSourceViewItem(this, item));
}

void AFQSourceListView::UpdateWidgets(bool force)
{
    AFQSourceViewModel* stm = GetStm();

    for (int i = 0; i < stm->m_sourceList.size(); i++) {
        obs_sceneitem_t* item = stm->m_sourceList[i];
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

void AFQSourceListView::_HideVeritcalScrollBar()
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void AFQSourceListView::SelectItem(obs_sceneitem_t* sceneitem, bool select)
{
    AFQSourceViewModel* stm = GetStm();
    int i = 0;

    for (; i < stm->m_sourceList.count(); i++) {
        if (stm->m_sourceList[i] == sceneitem)
            break;
    }

    if (i == stm->m_sourceList.count())
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

    OBSScene scene = SCENE_CONTEXT.GetCurrentScene();

    if (selectedIndices.size() < 1) {
        return false;
    }

    for (auto& idx : selectedIndices) {
        obs_sceneitem_t* item = stm->m_sourceList[idx.row()];
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

    OBSScene scene = SCENE_CONTEXT.GetCurrentScene();

    if (selectedIndices.size() < 1) {
        return false;
    }

    for (auto& idx : selectedIndices) {
        obs_sceneitem_t* item = stm->m_sourceList[idx.row()];
        if (!obs_sceneitem_is_group(item)) {
            return false;
        }
    }

    return true;
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

    OBSScene scene = SCENE_CONTEXT.GetCurrentScene();
    obs_source_t* scenesource = obs_scene_get_source(scene);
    AFQSourceViewModel* stm = GetStm();
    auto& items = stm->m_sourceList;
    QModelIndexList indices = selectedIndexes();

    obs_sceneitem_t* firstItem = items[indices[0].row()];
    obs_source_t* fromSource = obs_sceneitem_get_source(firstItem);

    if(AFSourceUtil::IsMustTopSource(fromSource))
        return;

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

    obs_source_t* dropSource = obs_sceneitem_get_source(dropItem);
    const char* drop_source_id = obs_source_get_id(dropSource);

    bool itemIsGroup = obs_sceneitem_is_group(dropItem);

    obs_sceneitem_t* dropGroup =
        itemIsGroup ? dropItem
        : obs_sceneitem_get_group(scene, dropItem);

    /* not a group if moving above the group */
    if (indicator == QAbstractItemView::AboveItem && itemIsGroup)
        dropGroup = nullptr;
    if (emptyDrop)
        dropGroup = nullptr;

    /* not allow group - painter_source, soop media source, ai manager source */
    {
        bool notAllowGroup = false;
        const bool isGroupingDrop =
            (dropGroup != nullptr) || (indicator == QAbstractItemView::OnItem);

        if (isGroupingDrop) {
            for (const QModelIndex& idx : indices) {
                obs_sceneitem_t* it = items[idx.row()];
                obs_source_t* src = obs_sceneitem_get_source(it);
                if (!src) continue;

                if (AFSourceUtil::IsSoopMediaSource(src)) {
                    notAllowGroup = true;
                    break;
                }

                const char* sourceId = obs_source_get_unversioned_id(src);
                if (sourceId && (strcmp(sourceId, "painter_source") == 0 || strcmp(sourceId, "soop_aimanager_source") == 0)) {
                    notAllowGroup = true;
                    break;
                }
            }

            if (notAllowGroup) {
                event->ignore();
                event->setDropAction(Qt::IgnoreAction);
                return;
            }
        }
    }

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

    if (row < 0 || row > stm->m_sourceList.count()) {
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
    if (row == stm->m_sourceList.count())
        itemBelow = nullptr;
    else
        itemBelow = stm->m_sourceList[row];

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
    OBSData undo_data = MAINFRAME->BackupScene(scene, &sources);

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
    if (AFSourceUtil::IsMustTopSource(dropSource) &&
        indicator == QAbstractItemView::AboveItem) {
        r++;
    }

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

    OBSData redo_data = MAINFRAME->BackupScene(scene, &sources);

    /* --------------------------------------- */
    /* add undo/redo action                    */

    const char* scene_name = obs_source_get_name(scenesource);
    QString action_name = QTStr("Undo.ReorderSources").arg(scene_name);
    MAINFRAME->CreateSceneUndoRedoAction(action_name, undo_data, redo_data);

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
        QSignalBlocker sourcesSignalBlocker(this);
        AFQSourceViewModel* stm = GetStm();

        QModelIndexList selectedIdxs = selected.indexes();
        QModelIndexList deselectedIdxs = deselected.indexes();

        for (int i = 0; i < selectedIdxs.count(); i++) {
            int idx = selectedIdxs[i].row();
            obs_sceneitem_select(stm->m_sourceList[idx], true);

            AFQSourceViewItem* item = GetItemWidget(selectedIdxs[i].row());
            if (item)
                item->SetSelected(true);
        }

        for (int i = 0; i < deselectedIdxs.count(); i++) {
            int idx = deselectedIdxs[i].row();
            obs_sceneitem_select(stm->m_sourceList[idx], false);

            AFQSourceViewItem* item = GetItemWidget(deselectedIdxs[i].row());
            if(item)
                item->SetSelected(false);
        }
    }
    QListView::selectionChanged(selected, deselected);
}

void AFQSourceListView::paintEvent(QPaintEvent* event)
{
    AFQSourceViewModel* stm = GetStm();
    if (stm && !stm->m_sourceList.count()) {
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

void AFQSourceListView::enterEvent(QEnterEvent* event)
{
    _ShowVerticalScrollBar();
}

void AFQSourceListView::leaveEvent(QEvent* event)
{
    _HideVeritcalScrollBar();
}


