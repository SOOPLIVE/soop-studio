#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

#include <QDialog>

#include "UIComponent/CRoundedDialogBase.h"

#include "obs.hpp"

class CDummyInteraction;
class OBSEventFilter;

namespace Ui {
	class AFQBrowserInteraction;
}

class AFQBrowserInteraction : public AFQRoundedDialogBase
{
	Q_OBJECT

public:
	explicit AFQBrowserInteraction(QWidget* parent, OBSSource source);
	~AFQBrowserInteraction();

private slots:
	void qslotDrawCallback();
	void qslotRefreshBrowser();
	void qslotCloseButtonClicked();
	void qslotSetWindowTitle(QString title);

signals:
	void qsignalClearPopup(OBSSource source);

protected:

	virtual void closeEvent(QCloseEvent* event) override;
	virtual bool nativeEvent(const QByteArray& eventType, void* message,
						     qintptr* result) override;

private:
	static void SourceRemoved(void* data, calldata_t* params);
	static void SourceRenamed(void* data, calldata_t* params);
	static void DrawPreview(void* data, uint32_t cx, uint32_t cy);

	bool GetSourceRelativeXY(int mouseX, int mouseY, int& x, int& y);

	bool HandleMouseClickEvent(QMouseEvent* event);
	bool HandleMouseMoveEvent(QMouseEvent* event);
	bool HandleMouseWheelEvent(QWheelEvent* event);
	bool HandleFocusEvent(QFocusEvent* event);
	bool HandleKeyEvent(QKeyEvent* event);

	OBSEventFilter* BuildEventFilter();

private:
	Ui::AFQBrowserInteraction* ui;

	//
	OBSSource m_obsSource;
	OBSSignal m_signalRemoved;
	OBSSignal m_signalRenamed;
	std::unique_ptr<OBSEventFilter> m_eventFilter;

	using properties_delete_t = decltype(&obs_properties_destroy);
	using properties_t =
		std::unique_ptr<obs_properties_t, properties_delete_t>;

	properties_t m_props;

#ifdef _WIN32
	CDummyInteraction* m_dummyInteraction = nullptr;
#endif
};

#ifdef _WIN32
class CDummyInteraction {
public:
	CDummyInteraction(QWidget* parent, OBSSource source);
	~CDummyInteraction();

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		CDummyInteraction* pThis = nullptr;
		if (uMsg == WM_CREATE) {
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			pThis = static_cast<CDummyInteraction*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
		}
		else {
			pThis = reinterpret_cast<CDummyInteraction*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		if (pThis) {
			return pThis->BrowserInteractionProc(hwnd, uMsg, wParam, lParam);
		}

		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	LRESULT  BrowserInteractionProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND	 GetHwnd() { return m_hwnd; }
	QWidget* GetWidget() { return m_widgetContainter; }

private:
	void _CreateInterationWindow();

private:
	HWND	  m_hwnd = NULL;
	OBSSource m_obsSource = nullptr;

	QWidget* m_parent = nullptr;
	QWindow* m_qWindow = nullptr;
	QWidget* m_widgetContainter = nullptr;
};
#endif
