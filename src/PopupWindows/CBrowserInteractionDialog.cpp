#include "CBrowserInteractionDialog.h"
#include "ui_browser-interaction-dialog.h"

#include <QKeyEvent>
#include <QCloseEvent>
#include <QScreen>
#include <QWindow>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#endif

#include "qt-wrapper.h"
#include "Common/MathMiscUtils.h"
#include "Common/EventFilterUtils.h"

#include "CoreModel/Locale/CLocaleTextManager.h"

AFQBrowserInteraction::AFQBrowserInteraction(QWidget* parent, OBSSource source) :
	AFQRoundedDialogBase((QDialog*)parent),
	ui(new Ui::AFQBrowserInteraction),
	m_obsSource(source),
	m_signalRemoved(obs_source_get_signal_handler(m_obsSource), "remove",
					AFQBrowserInteraction::SourceRemoved, this),
	m_signalRenamed(obs_source_get_signal_handler(m_obsSource), "rename",
					AFQBrowserInteraction::SourceRenamed, this),
	m_eventFilter(BuildEventFilter()),
	m_props(obs_source_properties(m_obsSource), obs_properties_destroy)
{
	Qt::WindowFlags flags = windowFlags();
	Qt::WindowFlags helpFlag = Qt::WindowContextHelpButtonHint;
	setWindowFlags(flags & (~helpFlag));

	ui->setupUi(this);

	ui->preview->setMouseTracking(true);
	ui->preview->setFocusPolicy(Qt::StrongFocus);
	ui->preview->installEventFilter(m_eventFilter.get());

	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();
	const char* name = obs_source_get_name(source);
	QString title = locale.Str("Basic.InteractionWindow");

	qslotSetWindowTitle((title.arg(QT_UTF8(name))));

	connect(ui->preview, &AFQTDisplay::qsignalDisplayCreated,
			this, &AFQBrowserInteraction::qslotDrawCallback);

	connect(ui->refreshButton, &QPushButton::clicked,
			this, &AFQBrowserInteraction::qslotRefreshBrowser);

	connect(ui->closeButton, &QPushButton::clicked,
			this, &AFQBrowserInteraction::qslotCloseButtonClicked);

	this->SetWidthFixed(true);

#ifdef _WIN32
	m_dummyInteraction = new CDummyInteraction(this, m_obsSource);
	if(m_dummyInteraction)
		ui->verticalLayout->addWidget(m_dummyInteraction->GetWidget());
#endif
}

AFQBrowserInteraction::~AFQBrowserInteraction()
{
	ui->preview->removeEventFilter(m_eventFilter.get());

#ifdef _WIN32
	delete m_dummyInteraction;
	m_dummyInteraction = nullptr;
#endif

	m_obsSource = nullptr;

	delete ui;
}

void AFQBrowserInteraction::qslotDrawCallback()
{
	obs_display_add_draw_callback(ui->preview->GetDisplay(),
							      AFQBrowserInteraction::DrawPreview,
								  this);
}

void AFQBrowserInteraction::qslotRefreshBrowser()
{
	if (!m_obsSource) {
		return;
	}

	obs_property_t* p = obs_properties_get(m_props.get(), "refreshnocache");
	obs_property_button_clicked(p, m_obsSource.Get());
}


void AFQBrowserInteraction::qslotCloseButtonClicked()
{
	qsignalClearPopup(m_obsSource);
}

void AFQBrowserInteraction::qslotSetWindowTitle(QString title)
{
	setWindowTitle(title);

	ui->titleLabel->setText(title);
}

void AFQBrowserInteraction::closeEvent(QCloseEvent* event)
{
	QDialog::closeEvent(event);
	if (!event->isAccepted())
		return;

	//config_set_int(App()->GlobalConfig(), "InteractionWindow", "cx",
	//	width());
	//config_set_int(App()->GlobalConfig(), "InteractionWindow", "cy",
	//	height());

	obs_display_remove_draw_callback(ui->preview->GetDisplay(),
									 AFQBrowserInteraction::DrawPreview,
									 this);
}

bool AFQBrowserInteraction::nativeEvent(const QByteArray&, void* message,
										qintptr*)
{
#ifdef _WIN32
	const MSG& msg = *static_cast<MSG*>(message);
	switch (msg.message) {
	case WM_MOVE:
		for (AFQTDisplay* const display :
		findChildren<AFQTDisplay*>()) {
			display->OnMove();
		}
		break;
	case WM_DISPLAYCHANGE:
		for (AFQTDisplay* const display :
		findChildren<AFQTDisplay*>()) {
			display->OnDisplayChange();
		}
	}
#else
	UNUSED_PARAMETER(message);
#endif

	return false;
}


void AFQBrowserInteraction::SourceRemoved(void* data, calldata_t* params)
{
	QMetaObject::invokeMethod(static_cast<AFQBrowserInteraction*>(data),
							  "close");
}

void AFQBrowserInteraction::SourceRenamed(void* data, calldata_t* params)
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

	const char* name = calldata_string(params, "new_name");
	QString title = locale.Str("Basic.InteractionWindow");
	title = title.arg(QT_UTF8(name));

	QMetaObject::invokeMethod(static_cast<AFQBrowserInteraction*>(data),
							  "qslotSetWindowTitle", Q_ARG(QString, title));
}

void AFQBrowserInteraction::DrawPreview(void* data, uint32_t cx, uint32_t cy)
{
	AFQBrowserInteraction* window = static_cast<AFQBrowserInteraction*>(data);

	if (!window->m_obsSource)
		return;

	uint32_t sourceCX = std::max(obs_source_get_width(window->m_obsSource), 1u);
	uint32_t sourceCY = std::max(obs_source_get_height(window->m_obsSource), 1u);

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

static int TranslateQtKeyboardEventModifiers(QInputEvent* event,
	bool mouseEvent)
{
	int obsModifiers = INTERACT_NONE;

	if (event->modifiers().testFlag(Qt::ShiftModifier))
		obsModifiers |= INTERACT_SHIFT_KEY;
	if (event->modifiers().testFlag(Qt::AltModifier))
		obsModifiers |= INTERACT_ALT_KEY;
#ifdef __APPLE__
	// Mac: Meta = Control, Control = Command
	if (event->modifiers().testFlag(Qt::ControlModifier))
		obsModifiers |= INTERACT_COMMAND_KEY;
	if (event->modifiers().testFlag(Qt::MetaModifier))
		obsModifiers |= INTERACT_CONTROL_KEY;
#else
	if (event->modifiers().testFlag(Qt::ControlModifier))
		obsModifiers |= INTERACT_CONTROL_KEY;
#endif

	if (!mouseEvent) {
		if (event->modifiers().testFlag(Qt::KeypadModifier))
			obsModifiers |= INTERACT_IS_KEY_PAD;
	}

	return obsModifiers;
}

static int TranslateQtMouseEventModifiers(QMouseEvent* event)
{
	int modifiers = TranslateQtKeyboardEventModifiers(event, true);

	if (event->buttons().testFlag(Qt::LeftButton))
		modifiers |= INTERACT_MOUSE_LEFT;
	if (event->buttons().testFlag(Qt::MiddleButton))
		modifiers |= INTERACT_MOUSE_MIDDLE;
	if (event->buttons().testFlag(Qt::RightButton))
		modifiers |= INTERACT_MOUSE_RIGHT;

	return modifiers;
}

bool AFQBrowserInteraction::GetSourceRelativeXY(int mouseX, int mouseY, int& relX,
	int& relY)
{
	float pixelRatio = devicePixelRatioF();
	int mouseXscaled = (int)roundf(mouseX * pixelRatio);
	int mouseYscaled = (int)roundf(mouseY * pixelRatio);

	QSize size = GetPixelSize(ui->preview);

	uint32_t sourceCX = std::max(obs_source_get_width(m_obsSource), 1u);
	uint32_t sourceCY = std::max(obs_source_get_height(m_obsSource), 1u);

	int x, y;
	float scale;

	GetScaleAndCenterPos(sourceCX, sourceCY, size.width(), size.height(), x,
		y, scale);

	if (x > 0) {
		relX = int(float(mouseXscaled - x) / scale);
		relY = int(float(mouseYscaled / scale));
	}
	else {
		relX = int(float(mouseXscaled / scale));
		relY = int(float(mouseYscaled - y) / scale);
	}

	// Confirm mouse is inside the source
	if (relX < 0 || relX > int(sourceCX))
		return false;
	if (relY < 0 || relY > int(sourceCY))
		return false;

	return true;
}

bool AFQBrowserInteraction::HandleMouseClickEvent(QMouseEvent* event)
{
	bool mouseUp = event->type() == QEvent::MouseButtonRelease;
	int clickCount = 1;
	if (event->type() == QEvent::MouseButtonDblClick)
		clickCount = 2;

	struct obs_mouse_event mouseEvent = {};

	mouseEvent.modifiers = TranslateQtMouseEventModifiers(event);

	int32_t button = 0;

	switch (event->button()) {
	case Qt::LeftButton:
		button = MOUSE_LEFT;
		break;
	case Qt::MiddleButton:
		button = MOUSE_MIDDLE;
		break;
	case Qt::RightButton:
		button = MOUSE_RIGHT;
		break;
	default:
		blog(LOG_WARNING, "unknown button type %d", event->button());
		return false;
	}


	QPoint pos = event->pos();
	bool insideSource = GetSourceRelativeXY(pos.x(), pos.y(), mouseEvent.x,
		mouseEvent.y);

	if (mouseUp || insideSource)
		obs_source_send_mouse_click(m_obsSource, &mouseEvent, button,
			mouseUp, clickCount);

#ifdef _WIN32
	if(m_dummyInteraction)
		SetFocus(m_dummyInteraction->GetHwnd());
#endif
    
	return true;
}

bool AFQBrowserInteraction::HandleMouseMoveEvent(QMouseEvent* event)
{
	struct obs_mouse_event mouseEvent = {};

	bool mouseLeave = event->type() == QEvent::Leave;

	if (!mouseLeave) {
		mouseEvent.modifiers = TranslateQtMouseEventModifiers(event);
		QPoint pos = event->pos();
		mouseLeave = !GetSourceRelativeXY(pos.x(), pos.y(),
			mouseEvent.x, mouseEvent.y);
	}

	obs_source_send_mouse_move(m_obsSource, &mouseEvent, mouseLeave);

	return true;
}

bool AFQBrowserInteraction::HandleMouseWheelEvent(QWheelEvent* event)
{
	struct obs_mouse_event mouseEvent = {};

	mouseEvent.modifiers = TranslateQtKeyboardEventModifiers(event, true);

	int xDelta = 0;
	int yDelta = 0;

	const QPoint angleDelta = event->angleDelta();
	if (!event->pixelDelta().isNull()) {
		if (angleDelta.x())
			xDelta = event->pixelDelta().x();
		else
			yDelta = event->pixelDelta().y();
	}
	else {
		if (angleDelta.x())
			xDelta = angleDelta.x();
		else
			yDelta = angleDelta.y();
	}

	const QPointF position = event->position();
	const int x = position.x();
	const int y = position.y();

	if (GetSourceRelativeXY(x, y, mouseEvent.x, mouseEvent.y)) {
		obs_source_send_mouse_wheel(m_obsSource, &mouseEvent, xDelta,
			yDelta);
	}

	return true;
}

bool AFQBrowserInteraction::HandleFocusEvent(QFocusEvent* event)
{
	bool focus = event->type() == QEvent::FocusIn;

	obs_source_send_focus(m_obsSource, focus);

	return true;
}

bool AFQBrowserInteraction::HandleKeyEvent(QKeyEvent* event)
{
	struct obs_key_event keyEvent;

	QByteArray text = event->text().toUtf8();
	keyEvent.modifiers = TranslateQtKeyboardEventModifiers(event, false);
	keyEvent.text = text.data();
	keyEvent.native_modifiers = event->nativeModifiers();
	keyEvent.native_scancode = event->nativeScanCode();
	keyEvent.native_vkey = event->nativeVirtualKey();

	bool keyUp = event->type() == QEvent::KeyRelease;

	obs_source_send_key_click(m_obsSource, &keyEvent, keyUp);

	return true;
}

OBSEventFilter* AFQBrowserInteraction::BuildEventFilter()
{
	return new OBSEventFilter([this](QObject*, QEvent* event) {
		switch (event->type()) {
			case QEvent::MouseButtonPress:
			case QEvent::MouseButtonRelease:
			case QEvent::MouseButtonDblClick:
				return this->HandleMouseClickEvent(
					static_cast<QMouseEvent*>(event));
			case QEvent::MouseMove:
			case QEvent::Enter:
			case QEvent::Leave:
				return this->HandleMouseMoveEvent(
					static_cast<QMouseEvent*>(event));

			case QEvent::Wheel:
				return this->HandleMouseWheelEvent(
					static_cast<QWheelEvent*>(event));
			case QEvent::FocusIn:
			case QEvent::FocusOut:
				return this->HandleFocusEvent(
					static_cast<QFocusEvent*>(event));
#if _WIN32
				// WIN32 Use m_dummyInteraction Window KeyEvent Message
#else
			case QEvent::KeyPress:
			case QEvent::KeyRelease:
				return this->HandleKeyEvent(
					static_cast<QKeyEvent*>(event));
#endif
		default:
			return false;
		}
		});
}


////////////////////////////////////////////////////////////////////////////
//
// For Dummy Browser Interaction 
#ifdef _WIN32
CDummyInteraction::CDummyInteraction(QWidget* parent, OBSSource source) :
	m_parent(parent),
	m_obsSource(source)
{
	// Using the WINAPI to create a window.
	_CreateInterationWindow();

	// Convert HWND to QWidget
	if (m_hwnd) {
		m_qWindow = QWindow::fromWinId((WId)m_hwnd);
		m_widgetContainter = QWidget::createWindowContainer(m_qWindow, m_parent);

		m_widgetContainter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
		m_widgetContainter->setMouseTracking(true);
		m_widgetContainter->setFocusPolicy(Qt::StrongFocus);
	}
}

CDummyInteraction::~CDummyInteraction() 
{
	if (m_qWindow) {
		delete m_qWindow;
		m_qWindow = nullptr;
	}

	m_obsSource = nullptr;

	DestroyWindow(m_hwnd);
	m_hwnd = NULL;

	m_parent = nullptr;
}

void CDummyInteraction::_CreateInterationWindow() 
{
	HINSTANCE hInstance = GetModuleHandle(nullptr);

	WNDCLASS wc = {};
	wc.lpfnWndProc = CDummyInteraction::WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"CDummyInteraction";
	RegisterClass(&wc);

	m_hwnd = CreateWindowEx(
		0, L"CDummyInteraction", L"DummyInteractionWindow",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 450, 120,
		nullptr, nullptr, hInstance, nullptr
	);

	::SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	::SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);
}

LRESULT CDummyInteraction::BrowserInteractionProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	if (uMsg == WM_CREATE) {
		return 0;
	}
	else if (uMsg == WM_CLOSE) {
		PostQuitMessage(0);
		return 0;
	}
	else if (uMsg == WM_DESTROY) {
		return 0;
	}
	else if (WM_KEYFIRST <= uMsg && WM_KEYLAST >= uMsg) {

		struct obs_key_event keyEvent;

		// Convert obs_key_event to WINAPI Message Param and match it.
		// Not use keyEvent.text
		keyEvent.native_vkey = wParam;
		keyEvent.native_scancode = lParam;
		keyEvent.native_modifiers = uMsg;

		obs_source_send_key_click(m_obsSource, &keyEvent, true);
		
		return 0;
	}
	else
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
#endif
