#include "CHotkeyComponent.h"
#include "ui_hotkey-widget.h"

#include "include/qt-wrapper.h"
//#include "obs-app.hpp"

#include <util/dstr.hpp>
#include <QPointer>
#include <QStyle>
#include <QAction>

#include "CoreModel/Icon/CIconContext.h"

#include "PopupWindows/SettingPopup/CHotkeySettingAreaWidget.h"

static inline void UpdateStyle(QWidget* widget)
{
    auto style = widget->style();
    style->unpolish(widget);
    style->polish(widget);
    widget->update();
}

void AFQHotkeyLabel::HighlightPair(bool highlight)
{
    if (!m_PairPartner)
        return;

    m_PairPartner->setProperty("hotkeyPairHover", highlight);
    UpdateStyle(m_PairPartner);
    setProperty("hotkeyPairHover", highlight);
    UpdateStyle(this);
}

void AFQHotkeyLabel::SetToolTip(const QString& toolTip)
{
    QLabel::setToolTip(toolTip);
    if (m_Widget)
        m_Widget->setToolTip(toolTip);
}


void AFQHotkeyLabel::SetRegisterType(obs_hotkey_registerer_t type)
{
    m_registerType = type;
}

obs_hotkey_registerer_t AFQHotkeyLabel::GetRegisterType()
{
    return m_registerType;
}

void AFQHotkeyLabel::SetHotkeyAreaName(QString name)
{
    m_hotkeyAreaName = name;
}

QString AFQHotkeyLabel::GetHotkeyAreaName()
{
    return m_hotkeyAreaName;
}

void AFQHotkeyLabel::enterEvent(QEnterEvent* event)
{
    if (!m_PairPartner)
        return;

    event->accept();
    HighlightPair(true);
}

void AFQHotkeyLabel::leaveEvent(QEvent* event)
{
    if (!m_PairPartner)
        return;

    event->accept();
    HighlightPair(false);
}

AFQHotkeyEdit::AFQHotkeyEdit(QWidget* parent)
    : QLineEdit(parent),
    original({}),
    settings(nullptr)
{
#ifdef __APPLE__
    // disable the input cursor on OSX, focus should be clear
    // enough with the default focus frame
    setReadOnly(true);
#endif
    setAttribute(Qt::WA_InputMethodEnabled, false);
    setAttribute(Qt::WA_MacShowFocusRect, true);
    InitSignalHandler();
    qslotResetKey();
}

AFQHotkeyEdit::AFQHotkeyEdit(QWidget* parent, obs_key_combination_t original,
    AFQHotkeySettingAreaWidget* settings)
    : QLineEdit(parent),
    original(original),
    settings(settings)
{
#ifdef __APPLE__
    // disable the input cursor on OSX, focus should be clear
    // enough with the default focus frame
    setReadOnly(true);
#endif
    setAttribute(Qt::WA_InputMethodEnabled, false);
    setAttribute(Qt::WA_MacShowFocusRect, true);
    InitSignalHandler();
    qslotResetKey();

    setObjectName("lineEdit_HotkeyCombos");
}

void AFQHotkeyEdit::qslotHandleNewKey(obs_key_combination_t new_key)
{
    if (new_key == key || obs_key_combination_is_empty(new_key))
        return;

    key = new_key;

    changed = true;
    emit qsignalKeyChanged(key);

    RenderKey();
}

void AFQHotkeyEdit::qslotReloadKeyLayout()
{
    RenderKey();
}

void AFQHotkeyEdit::qslotResetKey()
{
    key = original;

    changed = false;
    emit qsignalKeyChanged(key);

    RenderKey();
}

void AFQHotkeyEdit::qslotClearKey()
{
    key = { 0, OBS_KEY_NONE };

    changed = true;
    emit qsignalKeyChanged(key);

    RenderKey();
}

void AFQHotkeyEdit::UpdateDuplicationState()
{
    if (!dupeIcon && !hasDuplicate)
        return;

    if (!dupeIcon)
        CreateDupeIcon();

    if (dupeIcon->isVisible() != hasDuplicate) {
        dupeIcon->setVisible(hasDuplicate);
        update();
    }
}

void AFQHotkeyEdit::InitSignalHandler()
{
    layoutChanged = {
           obs_get_signal_handler(), "hotkey_layout_change",
           [](void* this_, calldata_t*) {
               auto edit = static_cast<AFQHotkeyEdit*>(this_);
               QMetaObject::invokeMethod(edit, "ReloadKeyLayout");
           },
           this };
}

void AFQHotkeyEdit::CreateDupeIcon()
{
    AFIconContext& icon = AFIconContext::GetSingletonInstance();
    dupeIcon = addAction(icon.GetHotkeyConflictIcon(),
        ActionPosition::TrailingPosition);

    dupeIcon->setToolTip(QT_UTF8(Str("Basic.Settings.Hotkeys.DuplicateWarning")));
    QObject::connect(dupeIcon, &QAction::triggered,
        [=] { emit qsignalSearchKey(key); });
    dupeIcon->setVisible(false);
}

QVariant AFQHotkeyEdit::inputMethodQuery(Qt::InputMethodQuery query) const
{
    if (query == Qt::ImEnabled) {
        return false;
    }
    else {
        return QLineEdit::inputMethodQuery(query);
    }
}

void AFQHotkeyEdit::keyPressEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat())
        return;

    obs_key_combination_t new_key;

    switch (event->key()) {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Alt:
    case Qt::Key_Meta:
        new_key.key = OBS_KEY_NONE;
        break;

#ifdef __APPLE__
    case Qt::Key_CapsLock:
        // kVK_CapsLock == 57
        new_key.key = obs_key_from_virtual_key(57);
        break;
#endif

    default:
        new_key.key =
            obs_key_from_virtual_key(event->nativeVirtualKey());
}

    new_key.modifiers =
        TranslateQtKeyboardEventModifiers(event->modifiers());

    qslotHandleNewKey(new_key);
}

#ifdef __APPLE__
void AFQHotkeyEdit::keyReleaseEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat())
        return;

    if (event->key() != Qt::Key_CapsLock)
        return;

    obs_key_combination_t new_key;

    // kVK_CapsLock == 57
    new_key.key = obs_key_from_virtual_key(57);
    new_key.modifiers =
        TranslateQtKeyboardEventModifiers(event->modifiers());

    qslotHandleNewKey(new_key);
}
#endif

void AFQHotkeyEdit::mousePressEvent(QMouseEvent* event)
{
    obs_key_combination_t new_key;

    switch (event->button()) {
    case Qt::NoButton:
    case Qt::LeftButton:
    case Qt::RightButton:
    case Qt::AllButtons:
    case Qt::MouseButtonMask:
        return;

    case Qt::MiddleButton:
        new_key.key = OBS_KEY_MOUSE3;
        break;

#define MAP_BUTTON(i, j)                        \
	case Qt::ExtraButton##i:                \
		new_key.key = OBS_KEY_MOUSE##j; \
		break;
        MAP_BUTTON(1, 4)
            MAP_BUTTON(2, 5)
            MAP_BUTTON(3, 6)
            MAP_BUTTON(4, 7)
            MAP_BUTTON(5, 8)
            MAP_BUTTON(6, 9)
            MAP_BUTTON(7, 10)
            MAP_BUTTON(8, 11)
            MAP_BUTTON(9, 12)
            MAP_BUTTON(10, 13)
            MAP_BUTTON(11, 14)
            MAP_BUTTON(12, 15)
            MAP_BUTTON(13, 16)
            MAP_BUTTON(14, 17)
            MAP_BUTTON(15, 18)
            MAP_BUTTON(16, 19)
            MAP_BUTTON(17, 20)
            MAP_BUTTON(18, 21)
            MAP_BUTTON(19, 22)
            MAP_BUTTON(20, 23)
            MAP_BUTTON(21, 24)
            MAP_BUTTON(22, 25)
            MAP_BUTTON(23, 26)
            MAP_BUTTON(24, 27)
#undef MAP_BUTTON
    }

    new_key.modifiers =
        TranslateQtKeyboardEventModifiers(event->modifiers());

    qslotHandleNewKey(new_key);
}

void AFQHotkeyEdit::RenderKey()
{
    DStr str;
    obs_key_combination_to_str(key, str);

    setText(QT_UTF8(str));
}

AFHotkeyWidget::AFHotkeyWidget(QWidget* parent, obs_hotkey_id id, std::string name,
    AFQHotkeySettingAreaWidget* settings,
    const std::vector<obs_key_combination_t>& combos) :
    QWidget(parent),
    ui(new Ui::AFHotkeyWidget),
    m_hotkeyId(id),
    m_sName(name),
    m_OBSSignalBindingsChanged(obs_get_signal_handler(),
        "hotkey_bindings_changed",
        &AFHotkeyWidget::_BindingsChanged, this),
    m_HotkeySettingsDialog(settings)
{
    ui->setupUi(this);
    auto layout = new QVBoxLayout;
    layout->setSpacing(10);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    SetKeyCombinations(combos);
}

AFHotkeyWidget::~AFHotkeyWidget()
{
    delete ui;
}

void AFHotkeyWidget::HandleChangedBindings(obs_hotkey_id id_)
{
    if (m_bIgnoreChangedBindings || m_hotkeyId != id_)
        return;

    std::vector<obs_key_combination_t> bindings;
    auto LoadBindings = [&](obs_hotkey_binding_t* binding) {
        if (obs_hotkey_binding_get_hotkey_id(binding) != m_hotkeyId)
            return;

        auto get_combo = obs_hotkey_binding_get_key_combination;
        bindings.push_back(get_combo(binding));
    };
    using LoadBindings_t = decltype(&LoadBindings);

    obs_enum_hotkey_bindings(
        [](void* data, size_t, obs_hotkey_binding_t* binding) {
            auto LoadBindings = *static_cast<LoadBindings_t>(data);
            LoadBindings(binding);
            return true;
        },
        static_cast<void*>(&LoadBindings));

    while (m_Edits.size() > 0)
        _RemoveEdit(m_Edits.size() - 1, false);

    SetKeyCombinations(bindings);
}

void AFHotkeyWidget::SetKeyCombinations(const std::vector<obs_key_combination_t>& combos)
{
    if (combos.empty())
        _AddEdit({ 0, OBS_KEY_NONE });

    for (auto combo : combos)
        _AddEdit(combo);
}

void AFHotkeyWidget::Apply()
{
    for (auto& edit : m_Edits) {
        edit->original = edit->key;
        edit->changed = false;
    }

    m_bChanged = false;

    //for (auto& revertButton : m_RevertButtons)
    //    revertButton->setEnabled(false);
}

void AFHotkeyWidget::GetCombinations(std::vector<obs_key_combination_t>& combinations) const
{
    combinations.clear();
    for (auto& edit : m_Edits)
        if (!obs_key_combination_is_empty(edit->key))
            combinations.emplace_back(edit->key);
}

void AFHotkeyWidget::Save()
{
    std::vector<obs_key_combination_t> combinations;
    Save(combinations);
}

void AFHotkeyWidget::Save(std::vector<obs_key_combination_t>& combinations)
{
    GetCombinations(combinations);
    Apply();

    auto AtomicUpdate = [&]() {
        m_bIgnoreChangedBindings = true;

        obs_hotkey_load_bindings(m_hotkeyId, combinations.data(),
            combinations.size());

        m_bIgnoreChangedBindings = false;
    };
    using AtomicUpdate_t = decltype(&AtomicUpdate);

    obs_hotkey_update_atomic(
        [](void* d) { (*static_cast<AtomicUpdate_t>(d))(); },
        static_cast<void*>(&AtomicUpdate));
}

void AFHotkeyWidget::Clear() {
    for (QPointer<AFQHotkeyEdit> edit : m_Edits) {
        if (edit.data()->key.key != OBS_KEY_NONE) {
            edit.data()->qslotClearKey();
        }
    }
}

bool AFHotkeyWidget::Changed() const
{
    return m_bChanged ||
        std::any_of(begin(m_Edits), end(m_Edits),
            [](AFQHotkeyEdit* edit) { return edit->changed; });
}

size_t AFHotkeyWidget::GetEditCount()
{
    return m_Edits.size();
}

void AFHotkeyWidget::enterEvent(QEnterEvent* event)
{
    if (!m_Label)
        return;

    event->accept();
    m_Label->HighlightPair(true);
}

void AFHotkeyWidget::leaveEvent(QEvent* event)
{
    if (!m_Label)
        return;

    event->accept();
    m_Label->HighlightPair(false);
}

void AFHotkeyWidget::_AddEdit(obs_key_combination combo, int idx)
{
    auto edit = new AFQHotkeyEdit(parentWidget(), combo, m_HotkeySettingsDialog);
    edit->setProperty("editType", "hotkeyWidgetEdit");
    edit->setToolTip(m_sToolTip);
    edit->setFixedSize(346, 40);

    //auto revert = new QPushButton;
    //revert->setProperty("themeID", "revertIconHotKey");
    //revert->setToolTip(QT_UTF8("Revert"));
    //revert->setFixedSize(QSize(18, 18));
    //revert->setEnabled(false);

    auto clear = new QPushButton;
    clear->setProperty("themeID", "clearIconHotkey");
    clear->setToolTip(QT_UTF8("Clear"));
    clear->setFixedSize(QSize(18, 18));
    clear->setEnabled(!obs_key_combination_is_empty(combo));

    QObject::connect(
        edit, &AFQHotkeyEdit::qsignalKeyChanged,
        [=](obs_key_combination_t new_combo) {
            clear->setEnabled(
                !obs_key_combination_is_empty(new_combo));
            //revert->setEnabled(edit->original != new_combo);
        });

    auto add = new QPushButton;
    add->setProperty("themeID", "addIconHotkey");
    add->setToolTip(QT_UTF8("Add"));
    add->setFixedSize(QSize(18, 18));

    auto remove = new QPushButton;
    remove->setProperty("themeID", "removeIconHotkey");
    remove->setToolTip(QT_UTF8("Remove"));
    remove->setFixedSize(QSize(18, 18));
    remove->setEnabled(m_RemoveButtons.size() > 0);

    auto CurrentIndex = [&, remove] {
        auto res = std::find(begin(m_RemoveButtons), end(m_RemoveButtons),
            remove);
        return std::distance(begin(m_RemoveButtons), res);
    };

    QObject::connect(add, &QPushButton::clicked, [&, CurrentIndex] {
            _AddEdit({ 0, OBS_KEY_NONE }, CurrentIndex() + 1);
            emit qsignalAddRemoveEdit();
        });

    QObject::connect(remove, &QPushButton::clicked, [&, CurrentIndex] { 
            _RemoveEdit(CurrentIndex()); 
            emit qsignalAddRemoveEdit();
        });

    QWidget* SubWidget = new QWidget(this);
    SubWidget->setObjectName("hotkeywidget");

    QHBoxLayout* subLayout = new QHBoxLayout;
    subLayout->setContentsMargins(0, 0, 12, 0);
    subLayout->setSpacing(12);
    subLayout->addWidget(edit);
    //subLayout->addWidget(revert);
    subLayout->addWidget(clear);
    subLayout->addWidget(add);
    subLayout->addWidget(remove);
    SubWidget->setLayout(subLayout);

    if (m_RemoveButtons.size() == 1)
        m_RemoveButtons.front()->setEnabled(true);

    if (idx != -1) {
        //m_RevertButtons.insert(begin(m_RevertButtons) + idx, revert);
        m_RemoveButtons.insert(begin(m_RemoveButtons) + idx, remove);
        m_Edits.insert(begin(m_Edits) + idx, edit);
    }
    else {
        //m_RevertButtons.emplace_back(revert);
        m_RemoveButtons.emplace_back(remove);
        m_Edits.emplace_back(edit);
    }

    _Layout()->insertWidget(idx, SubWidget);

    //QObject::connect(revert, &QPushButton::clicked, edit,
    //    &AFQHotkeyEdit::qslotResetKey);
    QObject::connect(clear, &QPushButton::clicked, edit,
        &AFQHotkeyEdit::qslotClearKey);

    QObject::connect(edit, &AFQHotkeyEdit::qsignalKeyChanged,
        [&](obs_key_combination) { emit qsignalKeyChanged(); });
    QObject::connect(edit, &AFQHotkeyEdit::qsignalSearchKey,
        [=](obs_key_combination combo) {
            emit qsignalSearchKey(combo);
        });
}

void AFHotkeyWidget::_RemoveEdit(size_t idx, bool signal)
{
    auto& edit = *(begin(m_Edits) + idx);
    if (!obs_key_combination_is_empty(edit->original) && signal) {
        m_bChanged = true;
    }

    //m_RevertButtons.erase(begin(m_RevertButtons) + idx);
    m_RemoveButtons.erase(begin(m_RemoveButtons) + idx);
    m_Edits.erase(begin(m_Edits) + idx);

    auto item = _Layout()->itemAt(static_cast<int>(idx))->widget();
    QLayoutItem* child = nullptr;
    while ((child = item->layout()->takeAt(0))) {
        delete child->widget();
        delete child;
    }
    delete item;

    if (m_RemoveButtons.size() == 1)
        m_RemoveButtons.front()->setEnabled(false);

    emit qsignalKeyChanged();
}

void AFHotkeyWidget::_BindingsChanged(void* data, calldata_t* param)
{
    auto widget = static_cast<AFHotkeyWidget*>(data);
    auto key = static_cast<obs_hotkey_t*>(calldata_ptr(param, "key"));

    QMetaObject::invokeMethod(widget, "HandleChangedBindings",
        Q_ARG(obs_hotkey_id, obs_hotkey_get_id(key)));
}
