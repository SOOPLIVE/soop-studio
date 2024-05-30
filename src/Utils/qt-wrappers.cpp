#include "qt-wrapper.h"


#include <graphics/graphics.h>
#include <util/threading.h>
#include <QWidget>
#include <QLayout>
#include <QComboBox>
#include <QMessageBox>
#include <QDataStream>
#include <QKeyEvent>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QLabel>
#include <QPushButton>
#include <QToolBar>
#include <QIcon>

#if !defined(_WIN32) && !defined(__APPLE__)
#include <obs-nix-platform.h>
#endif

#ifdef ENABLE_WAYLAND
#include <qpa/qplatformnativeinterface.h>
#endif

#include "platform/platform.hpp"

// [copy-obs]change : name change AFErrorBoxva -> AFErrorBoxva
static inline void AFErrorBoxva(QWidget *parent, const char *msg, va_list args)
{
	char full_message[4096];
	vsnprintf(full_message, sizeof(full_message), msg, args);

	QMessageBox::critical(parent, "Error", full_message);
}

// [copy-obs]change : name change OBSErrorBox -> AFErrorBox
void AFErrorBox(QWidget *parent, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	AFErrorBoxva(parent, msg, args);
	va_end(args);
}

void setCenterPositionNotUseParent(QWidget* target, QWidget* base)
{
	if (!target || !base)
		return;

	QRect targetRect = base->frameGeometry();
	QSize newSize = target->size();

	int newX = targetRect.x() + (targetRect.width() - newSize.width()) / 2;
	int newY = targetRect.y() + (targetRect.height() - newSize.height()) / 2;

	target->move(newX, newY);
}

// [copy-obs]change : name change AFCMessageBox -> AFCMessageBox
QMessageBox::StandardButton
AFCMessageBox::question(QWidget *parent, const QString &title,
			const QString &text,
			QMessageBox::StandardButtons buttons,
			QMessageBox::StandardButton defaultButton)
{
	QMessageBox mb(QMessageBox::Question, title, text,
		       QMessageBox::NoButton, parent);
	mb.setDefaultButton(defaultButton);

	if (buttons & QMessageBox::Ok) {
		QPushButton *button = mb.addButton(QMessageBox::Ok);
		// [copy-obs]remove
		//button->setText(QTStr("OK"));
		//
	}
#define add_button(x)                                               \
	if (buttons & QMessageBox::x) {                             \
		QPushButton *button = mb.addButton(QMessageBox::x); \
	}
		// [copy-obs]remove
		//button->setText(QTStr(#x));       \       
	//}
	add_button(Open);
	add_button(Save);
	add_button(Cancel);
	add_button(Close);
	add_button(Discard);
	add_button(Apply);
	add_button(Reset);
	add_button(Yes);
	add_button(No);
	add_button(Abort);
	add_button(Retry);
	add_button(Ignore);
#undef add_button
	return (QMessageBox::StandardButton)mb.exec();
}

void AFCMessageBox::information(QWidget *parent, const QString &title,
				const QString &text)
{
	QMessageBox mb(QMessageBox::Information, title, text,
		       QMessageBox::NoButton, parent);
	// [copy-obs]remove
	//mb.addButton(QTStr("OK"), QMessageBox::AcceptRole);
	//
	mb.exec();
}

void AFCMessageBox::warning(QWidget *parent, const QString &title,
			    const QString &text, bool enableRichText)
{
	QMessageBox mb(QMessageBox::Warning, title, text, QMessageBox::NoButton,
		       parent);
	if (enableRichText)
		mb.setTextFormat(Qt::RichText);
	// [copy-obs]remove
	//mb.addButton(QTStr("OK"), QMessageBox::AcceptRole);
	//
	mb.exec();
}

void AFCMessageBox::critical(QWidget *parent, const QString &title,
			     const QString &text)
{
	QMessageBox mb(QMessageBox::Critical, title, text,
		       QMessageBox::NoButton, parent);
	// [copy-obs]remove
	//mb.addButton(QTStr("OK"), QMessageBox::AcceptRole);
	//
	mb.exec();
}

bool QTToGSWindow(QWindow *window, gs_window &gswindow)
{
	bool success = true;

#ifdef _WIN32
	gswindow.hwnd = (HWND)window->winId();
#elif __APPLE__
	gswindow.view = (id)window->winId();
#else
	switch (obs_get_nix_platform()) {
	case OBS_NIX_PLATFORM_X11_EGL:
		gswindow.id = window->winId();
		gswindow.display = obs_get_nix_platform_display();
		break;
#ifdef ENABLE_WAYLAND
	case OBS_NIX_PLATFORM_WAYLAND: {
		QPlatformNativeInterface *native =
			QGuiApplication::platformNativeInterface();
		gswindow.display =
			native->nativeResourceForWindow("surface", window);
		success = gswindow.display != nullptr;
		break;
	}
#endif
	default:
		success = false;
		break;
	}
#endif
	return success;
}

uint32_t TranslateQtKeyboardEventModifiers(Qt::KeyboardModifiers mods)
{
	int obsModifiers = INTERACT_NONE;

	if (mods.testFlag(Qt::ShiftModifier))
		obsModifiers |= INTERACT_SHIFT_KEY;
	if (mods.testFlag(Qt::AltModifier))
		obsModifiers |= INTERACT_ALT_KEY;
#ifdef __APPLE__
	// Mac: Meta = Control, Control = Command
	if (mods.testFlag(Qt::ControlModifier))
		obsModifiers |= INTERACT_COMMAND_KEY;
	if (mods.testFlag(Qt::MetaModifier))
		obsModifiers |= INTERACT_CONTROL_KEY;
#else
	// Handle windows key? Can a browser even trap that key?
	if (mods.testFlag(Qt::ControlModifier))
		obsModifiers |= INTERACT_CONTROL_KEY;
	if (mods.testFlag(Qt::MetaModifier))
		obsModifiers |= INTERACT_COMMAND_KEY;

#endif

	return obsModifiers;
}

QDataStream &operator<<(QDataStream &out,
			const std::vector<std::shared_ptr<OBSSignal>> &)
{
	return out;
}

QDataStream &operator>>(QDataStream &in,
			std::vector<std::shared_ptr<OBSSignal>> &)
{
	return in;
}

QDataStream &operator<<(QDataStream &out, const OBSScene &scene)
{
	return out << QString(obs_source_get_uuid(obs_scene_get_source(scene)));
}

QDataStream &operator>>(QDataStream &in, OBSScene &scene)
{
	QString uuid;

	in >> uuid;

	OBSSourceAutoRelease source = obs_get_source_by_uuid(QT_TO_UTF8(uuid));
	scene = obs_scene_from_source(source);

	return in;
}

QDataStream &operator<<(QDataStream &out, const OBSSource &source)
{
	return out << QString(obs_source_get_uuid(source));
}

QDataStream &operator>>(QDataStream &in, OBSSource &source)
{
	QString uuid;

	in >> uuid;

	OBSSourceAutoRelease source_ = obs_get_source_by_uuid(QT_TO_UTF8(uuid));
	source = source_;

	return in;
}

void DeleteLayout(QLayout *layout)
{
	if (!layout)
		return;

	for (;;) {
		QLayoutItem *item = layout->takeAt(0);
		if (!item)
			break;

		QLayout *subLayout = item->layout();
		if (subLayout) {
			DeleteLayout(subLayout);
		} else {
			delete item->widget();
			delete item;
		}
	}

	delete layout;
}

class QuickThread : public QThread {
public:
	explicit inline QuickThread(std::function<void()> func_) : func(func_)
	{
	}

private:
	virtual void run() override { func(); }

	std::function<void()> func;
};

QThread *CreateQThread(std::function<void()> func)
{
	return new QuickThread(func);
}

volatile long insideEventLoop = 0;

void ExecuteFuncSafeBlock(std::function<void()> func)
{
	QEventLoop eventLoop;

	auto wait = [&]() {
		func();
		QMetaObject::invokeMethod(&eventLoop, "quit",
					  Qt::QueuedConnection);
	};

	os_atomic_inc_long(&insideEventLoop);
	QScopedPointer<QThread> thread(CreateQThread(wait));
	thread->start();
	eventLoop.exec();
	thread->wait();
	os_atomic_dec_long(&insideEventLoop);
}

void ExecuteFuncSafeBlockMsgBox(std::function<void()> func,
				const QString &title, const QString &text)
{
	QMessageBox dlg;
	dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowCloseButtonHint);
	dlg.setWindowTitle(title);
	dlg.setText(text);
	dlg.setStandardButtons(QMessageBox::StandardButtons());

	auto wait = [&]() {
		func();
		QMetaObject::invokeMethod(&dlg, "accept", Qt::QueuedConnection);
	};

	os_atomic_inc_long(&insideEventLoop);
	QScopedPointer<QThread> thread(CreateQThread(wait));
	thread->start();
	dlg.exec();
	thread->wait();
	os_atomic_dec_long(&insideEventLoop);
}

static bool enable_message_boxes = false;

void EnableThreadedMessageBoxes(bool enable)
{
	enable_message_boxes = enable;
}

void ExecThreadedWithoutBlocking(std::function<void()> func,
				 const QString &title, const QString &text)
{
	if (!enable_message_boxes)
		ExecuteFuncSafeBlock(func);
	else
		ExecuteFuncSafeBlockMsgBox(func, title, text);
}

bool LineEditCanceled(QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = reinterpret_cast<QKeyEvent *>(event);
		return keyEvent->key() == Qt::Key_Escape;
	}

	return false;
}

bool LineEditChanged(QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = reinterpret_cast<QKeyEvent *>(event);

		switch (keyEvent->key()) {
		case Qt::Key_Tab:
		case Qt::Key_Backtab:
		case Qt::Key_Enter:
		case Qt::Key_Return:
			return true;
		}
	} else if (event->type() == QEvent::FocusOut) {
		return true;
	}

	return false;
}

void SetComboItemEnabled(QComboBox *c, int idx, bool enabled)
{
	QStandardItemModel *model =
		dynamic_cast<QStandardItemModel *>(c->model());
	QStandardItem *item = model->item(idx);
	item->setFlags(enabled ? Qt::ItemIsSelectable | Qt::ItemIsEnabled
			       : Qt::NoItemFlags);
}

void setThemeID(QWidget *widget, const QString &themeID)
{
	if (widget->property("themeID").toString() != themeID) {
		widget->setProperty("themeID", themeID);

		/* force style sheet recalculation */
		QString qss = widget->styleSheet();
		widget->setStyleSheet("/* */");
		widget->setStyleSheet(qss);
	}
}

QString SelectDirectory(QWidget *parent, QString title, QString path)
{
	QString dir = QFileDialog::getExistingDirectory(
		parent, title, path,
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	return dir;
}

QString SaveFile(QWidget *parent, QString title, QString path,
		 QString extensions)
{
	QString file =
		QFileDialog::getSaveFileName(parent, title, path, extensions);

	return file;
}

QString OpenFile(QWidget *parent, QString title, QString path,
		 QString extensions)
{
	QString file =
		QFileDialog::getOpenFileName(parent, title, path, extensions);

	return file;
}

QStringList OpenFiles(QWidget *parent, QString title, QString path,
		      QString extensions)
{
	QStringList files =
		QFileDialog::getOpenFileNames(parent, title, path, extensions);

	return files;
}

static void SetLabelText(QLabel *label, const QString &newText)
{
	if (label->text() != newText)
		label->setText(newText);
}

void TruncateLabel(QLabel *label, QString newText, int length)
{
	if (newText.length() < length) {
		label->setToolTip(QString());
		SetLabelText(label, newText);
		return;
	}

	label->setToolTip(newText);
	newText.truncate(length);
	newText += "...";

	SetLabelText(label, newText);
}

void TruncateTextToLabelWidth(QLabel* label, QString newText)
{
	QFontMetrics fontMetrics(label->font());

	QRect newTextRect = fontMetrics.boundingRect(newText);
	if (newTextRect.width() > label->width()) {
		QString elidedText = fontMetrics.elidedText(newText, Qt::ElideRight, label->width() - fontMetrics.averageCharWidth() * 5);
		label->setText(elidedText);
		label->setToolTip(newText);
	}
	else
		label->setText(newText);
}

void RefreshToolBarStyling(QToolBar *toolBar)
{
	for (QAction *action : toolBar->actions()) {
		QWidget *widget = toolBar->widgetForAction(action);

		if (!widget)
			continue;

		widget->style()->unpolish(widget);
		widget->style()->polish(widget);
	}
}

void LoadIconFromABSPath(const char* iconPath, QIcon& iconVariable)
{
    std::string imgPath;
    GetDataFilePath(iconPath, imgPath);
    iconVariable = QIcon(imgPath.data());
}

void ChangeStyleSheet(QWidget* target, QString styleSheet)
{
	if (target)
		target->setStyleSheet(styleSheet);
}

void updateStyleSheet(QWidget* widget) {

	if (!widget)
		return;

	widget->style()->unpolish(widget);
	widget->style()->polish(widget);
}
