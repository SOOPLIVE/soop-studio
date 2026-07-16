#pragma once

#include "CoreModel/Source/CSource.h"

#include "MainFrame/CMainFrame.h"

#include "UIComponent/CSceneBottomButton.h"

class CMainSceneSource : public QObject 
{
	Q_OBJECT

public:
	CMainSceneSource(QObject* parent);
	~CMainSceneSource();

public slots:
	void qslotAddSourceMenu();
	void qslotShowSelectSourcePopup();
	void qslotAddSource(QString sourceID);

	// Source Context Menu qslot 
	void qslotActionCopySource();
	void qslotActionPasteSource();	// Auto-paste setup: use "dup" if duplication is possible; otherwise use "ref".
	void qslotActionPasteRefSource();
	void qslotActionPasteDupSource();
	void qslotActionRenameSource();
	void qslotActionRemoveSource();
	void qslotActionCopyTransform();
	void qslotActionPasteTransform();
	void qslotActionResetTransform();
	void qslotActionRotate90CW();
	void qslotActionRotate90CCW();
	void qslotActionRotate180();
	void qslotFlipHorizontal();
	void qslotFlipVertical();
	void qslotFitToScreen();
	void qslotStretchToScreen();
	void qslotCenterToScreen();
	void qslotVerticalCenter();
	void qslotHorizontalCenter();
	void qslotActionEditTransform();
	void qslotActionShowInteractionPopup();
	void qslotActionShowProperties();
	void qslotOpenSourceFilters();
	void qslotCopySourceFilters();
	void qslotPasteSourceFilters();
	void qslotSetSplitEffectFilter();
	void qslotSetScaleFilter();
	void qslotBlendingMethod();
	void qslotBlendingMode();
	void qslotSetDeinterlaceingMode();
	void qslotSetDeinterlacingOrder();
	void qslotSourceListItemColorChange();
	void qslotResizeOutputSizeOfSource();
	void qslotActionScaleWindow();
	void qslotActionScaleCanvas();
	void qslotActionScaleOutput();

	void qslotActionRemoveScene();
	void qslotActionRenameScene();

	void qslotAddSceneTriggered();
	void qslotSceneButtonClicked(OBSScene scene);
	void qslotSceneButtonDoubleClicked(OBSScene scene);
	void qslotSceneButtonDotClicked(); // Not Used

	

signals:
	void qsignalScreenShotFinished();

public:
	void AddNewSource(QString sourceId, bool addOnProgramMode = false);
	void CreateSourcePopupMenu(int idx, bool preview = false);
	void CreateDefaultScene(bool firstStart);

	void Screenshot(OBSSource source = nullptr);
	void ScreenshotSource(OBSSource source = nullptr, bool internalSave = false);
	void UpdateEditMenu();
	void ClearClipboard();
	void AddSceneBottomButton(AFQSceneBottomButton* button);
	void ClearSceneBottomButtons();
	void SetSceneBottomButtonStyleSheet(OBSSource scene);
	size_t SizeCopiedSources() const { return m_clipboard.size(); }

private:
	AFQCustomMenu* _AddBackgroundColorMenu(AFQCustomMenu* menu,
										   QWidgetAction* widgetAction,
										   AFQColorSelect* select,
										   obs_sceneitem_t* item);
	AFQCustomMenu* _AddScaleFilteringMenu(AFQCustomMenu* menu, obs_sceneitem_t* item);
	AFQCustomMenu* _AddBlendingModeMenu(AFQCustomMenu* menu, obs_sceneitem_t* item);
	AFQCustomMenu* _AddBlendingMethodMenu(AFQCustomMenu* menu, obs_sceneitem_t* item);
	AFQCustomMenu* _AddDeinterlacingMenu(AFQCustomMenu* menu, obs_source_t* source);

	void _AddSourceMenuButton(const char* id, QWidget* popup);

	QColor _GetSourceListBackgroundColor(int preset);

private:
	QPointer<QObject>			m_screenshotData;
	std::deque<SourceCopyInfo>  m_clipboard;
	bool						m_hasCopiedTransform = false;

	// MainFrame Bottom Scene Button
	std::vector<AFQSceneBottomButton*>  m_sceneButtons;

	// Use Source Context Menu
	QPointer<QWidgetAction>     m_widgetActionColor;
	QPointer<AFQColorSelect>    m_widgetColorSelect;

	QPointer<AFQCustomMenu>		m_menuScaleFiltering;
	QPointer<AFQCustomMenu>		m_menuBlendingMode;
	QPointer<AFQCustomMenu>		m_menuBlendingMethodMode;
	QPointer<AFQCustomMenu>		m_menuDeinterlace;

};
//
extern void RemoveSceneAndReleaseNested(obs_source_t* source);