#pragma once

#include <QDialog>
#include <QAction>

#include "obs.hpp"

#include "ui_basic-filters.h"

#include "UIComponent/CTopBaseWindow.h"
#include "UIComponent/CCustomMenu.h"

class OBSPropertiesView;

namespace Ui {
	class AFQBasicFilters;
}

class AFQBasicFilters : public AFTTopBaseDialog
{
	Q_OBJECT

public:
	AFQBasicFilters(QWidget* parent, OBSSource source_);
	~AFQBasicFilters();

	inline void UpdateSource(obs_source_t* target) {
		if(m_obsSource == target)
			UpdateFilters();
	}

private slots:
	void AddFilter(OBSSource filter, bool focus = true);
	void RemoveFilter(OBSSource filter);
	void ReorderFilters();
	void RenameAsyncFilter();
	void RenameEffectFilter();
	void ResetFilters();
	void RenameFiltersTitle(QString title);

	void qslotAddAsyncFilterClicked();
	void qslotRemoveAsyncFilterClicked();
	void qslotMoveUpAsyncFilterClicked();
	void qslotMoveDownAsyncFilterClicked();
	void qslotAsyncFiltersCurrentRowChanged(int row);
	void qslotAsyncFiltersGotFocus();
	void qslotAsyncFiltersCustomContextMenuRequested(const QPoint &pos);

	void qslotAddEffectFilterClicked();
	void qslotRemoveEffectFilterClicked();
	void qslotMoveUpEffectFilterClicked();
	void qslotMoveDownEffectFilterClicked();
	void qslotEffectFiltersCurrentRowChanged(int row);
	void qslotEffectFiltersGotFocus();
	void qslotEffectFiltersCustomContextMenuRequested(const QPoint& pos);

	void qslotRenameFilterTriggered();
	void qslotRemoveFilterTriggered();
	void qslotMoveUpFilterTriggered();
	void qslotMoveDownFilterTriggered();

	void FiltersMoved(const QModelIndex& srcParent, int srcIdxStart,
					  int srcIdxEnd, const QModelIndex& dstParent,
					  int dstIdx);


	void CopyFilter();
	void PasteFilter();

protected:
	virtual void closeEvent(QCloseEvent* event) override;
	virtual void showEvent(QShowEvent* event) override;

private:
	void SetSignalSlotUI();

	AFQCustomMenu* CreateAddFilterPopupMenu(bool async);

	inline OBSSource GetFilter(int row, bool async);

	void UpdateFilters();
	void UpdateSplitter();
	void UpdateSplitter(bool show_splitter_frame);
	void UpdatePropertiesView(int row, bool async);
	void UpdateItemFontColor(int row, bool async);

	void AddNewFilter(const char* id);
	void ReorderFilter(QListWidget* list, obs_source_t* filter, size_t idx);

	static void OBSSourceFilterAdded(void* param, calldata_t* data);
	static void OBSSourceFilterRemoved(void* param, calldata_t* data);
	static void OBSSourceReordered(void* param, calldata_t* data);
	static void SourceRemoved(void* param, calldata_t* data);
	static void SourceRenamed(void* param, calldata_t* data);
	static void UpdateProperties(void* data, calldata_t* params);
	static void DrawPreview(void* data, uint32_t cx, uint32_t cy);
	
	void CustomContextMenu(const QPoint& pos, bool async);
	void EditItem(QListWidgetItem* item, bool async);
	void DuplicateItem(QListWidgetItem* item);

	void FilterNameEdited(QWidget* editor, QListWidget* list);
	void delete_filter(OBSSource filter);

private:
	std::unique_ptr<Ui::AFQBasicFilters> ui;

	OBSSource m_obsSource;
	OBSPropertiesView* m_pPropsView = nullptr;

	OBSSignal m_signalAdd;
	OBSSignal m_signalRemove;
	OBSSignal m_signalReorder;
	OBSSignal m_signalRemoveSource;
	OBSSignal m_signalRenameSource;

	OBSSignal m_signalUpdateProperties;

	QAction* m_actionRenameFilter = nullptr;
	QAction* m_actionRemoveFilter = nullptr;
	QAction* m_actionMoveUp = nullptr;
	QAction* m_actionMoveDown = nullptr;

	bool m_editActive = false;
	bool m_isAsync = false;
};