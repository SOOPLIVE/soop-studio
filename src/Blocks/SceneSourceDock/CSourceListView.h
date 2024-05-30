#pragma once

#include <deque>

#include <QAbstractListModel>
#include <QListView>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QStaticText>
#include <QTimer>

#include <qt-wrapper.h>

#include "CoreModel/Source/CSource.h"

#include "Blocks/SceneSourceDock/CSceneSourceDockWidget.h"
#include "PopupWindows/CBasicTransform.h"
#include "UIComponent/CCustomMenu.h"

// --
class AFQVisibleCheckBox : public QCheckBox
{
    Q_OBJECT
public:
    explicit AFQVisibleCheckBox(QWidget* parent = nullptr) : QCheckBox(parent) {};
};

class AFQLockedCheckBox : public QCheckBox
{
    Q_OBJECT
public:
    explicit AFQLockedCheckBox(QWidget *parent = nullptr) : QCheckBox(parent) {};
};

class SourceTreeSubItemCheckBox : public QCheckBox {
    Q_OBJECT
};


class AFQSourceListView;

class AFQSourceViewItem : public QWidget
{
    enum class Type {
        Unknown,
        Item,
        Group,
        SubItem
    };

    Q_OBJECT

    friend class AFQSourceListView;
    friend class AFQSourceViewModel;
    friend class AFQSourceItem;

public:
    explicit AFQSourceViewItem(AFQSourceListView* sourceListView, OBSSceneItem sceneItem);

signals:

public slots:

private slots:
    void Clear();

    void EnterEditMode();
    void ExitEditMode(bool save);

    void ClickedItemVisible(bool val);
    void ClickedItemLocked(bool val);

    void ExpandClicked(bool checked);
    void VisibilityChanged(bool visible);
    void LockedChanged(bool locked);

    void Select();
    void DeSelect();

public:
    void DisconnectSignals();
    void ReconnectSignals();
    void Update(bool force);

    void SetSelected(bool select, bool refresh = true);
    void SetBackgroundColor(const QColor& color, bool refresh = true);
    void SetFontColor(const QColor& color, bool refresh = true);
    void RefreshSourceListItemColor();

    bool IsEditing();
    QColor GetBackgroundColor();

protected:
    virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
    virtual void enterEvent(QEnterEvent* event) override;
    virtual void leaveEvent(QEvent* event) override;
    virtual void paintEvent(QPaintEvent* event) override;

    virtual bool eventFilter(QObject* object, QEvent* event) override;

private:
    QLabel* _CreateIconLabel(const char* id);
    QLabel* _CreateNameLabel(const char* name);
    AFQVisibleCheckBox* _CreateVisibleCheckBox();
    AFQLockedCheckBox*  _CreateLockedCheckBox();

    void ExitEditModeInternal(bool save);

    // For Signal
    static void CallbackRemoveItem(void* data, calldata_t* cd);
    static void CallbackItemVisible(void* data, calldata_t* cd);
    static void CallbackItemSelect(void* data, calldata_t* cd);
    static void CallbackItemDeSelect(void* data, calldata_t* cd);
    static void CallbackItemLocked(void* data, calldata_t* cd);
    static void CallbackItemReorderGroup(void* data, calldata_t* cd);
    static void CallbackRenamedSource(void* data, calldata_t* cd);
    static void CallbackRemoveSource(void* data, calldata_t* cd);

private:
    // UI Widget
    QHBoxLayout* m_layoutBox = nullptr;

    QSpacerItem* groupSpacer = nullptr;
    QCheckBox*   groupExpand = nullptr;
    QLabel* m_labelIcon = nullptr;
    QLabel* m_labelName = nullptr;
    AFQVisibleCheckBox* m_checkBoxVisible = nullptr;
    AFQLockedCheckBox*  m_checkBoxLocked = nullptr;

    QLineEdit* m_editorName = nullptr;

    AFQSourceListView*  m_sourceListView;

    // var
    Type m_type;
    std::string m_strNewName;
    OBSSceneItem m_sceneItem;

    OBSSignal m_signalSceneRemove;
    OBSSignal m_signalItemRemove;
    OBSSignal m_signalGroupReorder;
    OBSSignal m_signalSelect;
    OBSSignal m_signalDeSelect;
    OBSSignal m_signalVisible;
    OBSSignal m_signalLocked;
    OBSSignal m_signalRename;
    OBSSignal m_signalRemove;

    QIcon   m_iconSource;
    QIcon   m_iconVisible;
    QIcon   m_iconLocked;

    QColor  m_colorFont = QColor(255, 255, 255, 255);
    QColor  m_colorBackground = QColor(24,27,32,84);

    bool    m_isVisible = true;
    bool    m_isSelected = false;
    bool    m_isHovered = false;
};

class AFQSourceViewModel : public QAbstractListModel
{
    Q_OBJECT

    friend class AFQSourceListView;
    friend class AFQSourceViewItem;

public:
    explicit AFQSourceViewModel(AFQSourceListView* parent);

    // override QAbstractListModel
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual Qt::DropActions supportedDropAction() const;

    void Clear();
    void SceneChanged();
    void ReorderItems();

    void Add(obs_sceneitem_t* item);
    void Remove(obs_sceneitem_t* item);

    OBSSceneItem Get(int idx);
    QString GetNewGroupName();

    void GroupSelectedItems(QModelIndexList& indices);
    void UngroupSelectedGroups(QModelIndexList& indices);

    void ExpandGroup(obs_sceneitem_t* item);
    void CollapseGroup(obs_sceneitem_t* item);

    void UpdateGroupState(bool update);

private:
    QVector<OBSSceneItem>   m_vSourceList;
    AFQSourceListView*      m_sourceListView = nullptr;

    bool m_hasGroups = false;
};
// --

#include <QStyledItemDelegate>
#include <QPainter>
class NoFocusAndIndicatorColorDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate; 

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QStyleOptionViewItem optionNoFocus = option;
        optionNoFocus.state &= ~QStyle::State_HasFocus;
        QStyledItemDelegate::paint(painter, optionNoFocus, index);

        QPen pen(Qt::SolidLine);
        pen.setColor(Qt::white);
        painter->setPen(pen);
    }
};

class AFQSourceListView : public QListView
{
#pragma region QT Field
    Q_OBJECT

signals:
    void            qsignalClickedSource(unsigned int, unsigned int);
    void            qsignalDisClickedSource(unsigned int, unsigned int);
    void            qsignalSwapSourceSignal(uint, uint, uint);
    void            qsignalHoverLayoutSignal(uint, uint);
    void            qsignalLeaveLayoutSignal(uint, uint);

    void            qSignalCheckSourceClicked(bool clicked);

public slots:

    inline void ReorderItems() { GetStm()->ReorderItems(); }
    inline void RefreshItems() { GetStm()->SceneChanged(); }

    bool Edit(int idx);
    void Remove(OBSSceneItem item);

    void ShowContextMenu(const QPoint& pos);

    void GroupSelectedItems();
    void UngroupSelectedGroups();
    void NewGroupEdit(int idx);

    void UpdateNoSourcesMessage();

private slots:
    void qSlotHideScrollBar();

#pragma endregion QT Field


#pragma region class initializer, destructor
public:
    explicit AFQSourceListView(QWidget* parent = nullptr);
#pragma endregion class initializer, destructor


#pragma region public func
public:
    inline void Add(obs_sceneitem_t* item) { GetStm()->Add(item); }
    inline OBSSceneItem Get(int idx) { return GetStm()->Get(idx); }
    void Clear();

    int GetTopSelectedSourceItem();

    void ResetWidgets();
    void UpdateWidget(const QModelIndex& idx, obs_sceneitem_t* item);
    void UpdateWidgets(bool force = false);

    void SelectItem(obs_sceneitem_t* sceneitem, bool select);

    bool MultipleBaseSelected() const;
    bool GroupsSelected() const;

    bool IgnoreReorder() { return m_isIgnoreReorder; }

    // front_api
    void RefreshSourceItem();

#pragma endregion public func

#pragma region private func
private:
    void _ShowVerticalScrollBar();
#pragma endregion private func

public:
    inline AFQSourceViewItem* GetItemWidget(int idx)
    {
        QWidget* widget = indexWidget(GetStm()->createIndex(idx, 0));
        return reinterpret_cast<AFQSourceViewItem*>(widget);
    }

    inline AFQSourceViewItem* GetItemWidgetFromSceneItem(obs_sceneitem_t* sceneItem)
    {
        int i = 0;
        AFQSourceViewItem* sourceItem = GetItemWidget(i);
        OBSSceneItem item = Get(i);
        int64_t id = obs_sceneitem_get_id(sceneItem);
        while (sourceItem && obs_sceneitem_get_id(item) != id) {
            i++;
            sourceItem = GetItemWidget(i);
            item = Get(i);
        }
        if (sourceItem)
            return sourceItem;

        return nullptr;
    }

    inline int GetItemIdx(AFQSourceViewItem* item)
    {
        for (int row = 0; row < GetStm()->rowCount(); ++row) {
            AFQSourceViewItem* itemValue = GetItemWidget(row);
            if (itemValue == item) {
                return row;
            }
        }
        return -1;
    }

    void   RegisterShortCut(QAction* removeSourceAction);

private:
    inline AFQSourceViewModel* GetStm() const { return reinterpret_cast<AFQSourceViewModel*>(model()); }

protected:
    virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
    virtual void dragMoveEvent(QDragMoveEvent* event) override;
    virtual void dropEvent(QDropEvent* event) override;
    virtual void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;
    virtual void paintEvent(QPaintEvent* event);
    virtual void wheelEvent(QWheelEvent* event) override;

#pragma region public member var
public:
#pragma endregion public member var

#pragma region private member var
private:
    QVector<AFQSourceViewItem*> m_prevSelectedSourceItem;

    AFQCustomMenu* m_contextMenu = nullptr;

    bool m_textPrepared = false;
    QStaticText m_textNoSources;

    OBSData m_undoSceneData;

    QTimer* m_timerScrollVisible = nullptr;

    bool    m_isIgnoreReorder = false;

#pragma endregion private member var

private:
    friend class AFQSourceViewModel;
    friend class AFQSourceViewItem;
    friend class AFSourceData;
};