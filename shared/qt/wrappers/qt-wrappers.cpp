/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "moc_qt-wrappers.cpp"

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
#include <QTooltip>
#include <QPainter>
#include <QSvgRenderer>
#include <QScrollBar>

#include "platform/platform.hpp"
#include <sstream>

static inline void OBSErrorBoxva(QWidget *parent, const char *msg, va_list args)
{
	char full_message[8192];
	vsnprintf(full_message, sizeof(full_message), msg, args);

	QMessageBox::critical(parent, "Error", full_message);
}

void OBSErrorBox(QWidget *parent, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	OBSErrorBoxva(parent, msg, args);
	va_end(args);
}

QPoint setCenterPositionNotUseParent(QWidget* target, QWidget* base)
{
	if(!target || !base)
		return QPoint();

	QRect targetRect = base->frameGeometry();
	QSize newSize = target->size();

	int newX = targetRect.x() + (targetRect.width() - newSize.width()) / 2;
	int newY = targetRect.y() + (targetRect.height() - newSize.height()) / 2;

	target->move(newX, newY);

	return QPoint(newX, newY);
}

void setTopRightPositionNotUseParent(QWidget* target, QWidget* base)
{
	if(!target || !base)
		return;

	QRect baseRect = base->frameGeometry();
	QSize newSize = target->size();

	int newX = baseRect.x() + baseRect.width();
	int newY = baseRect.y();

	target->move(newX, newY);
}

void EnsureDialogVisible(QDialog* dialog)
{
	if(!dialog) return;

	QRect dialogGeometry = dialog->frameGeometry();
	QRect screenGeometry = dialog->screen()->availableGeometry();

	QPoint newPos = dialogGeometry.topLeft();

	// left pos set
	if(dialogGeometry.left() < screenGeometry.left())
		newPos.setX(screenGeometry.left());

	// right pos set
	if(dialogGeometry.right() > screenGeometry.right())
		newPos.setX(screenGeometry.right() - dialogGeometry.width());

	// top pos set
	if(dialogGeometry.top() < screenGeometry.top())
		newPos.setY(screenGeometry.top());

	// bottom pos set
	if(dialogGeometry.bottom() > screenGeometry.bottom())
		newPos.setY(screenGeometry.bottom() - dialogGeometry.height());

	dialog->move(newPos);
}

bool QTToGSWindow(QWindow* window, gs_window& gswindow)
{
	bool success = true;

#ifdef _WIN32
	gswindow.hwnd = (HWND)window->winId();
#elif __APPLE__
	gswindow.view = (id)window->winId();
#else
	switch(obs_get_nix_platform()) {
		case OBS_NIX_PLATFORM_X11_EGL:
			gswindow.id = window->winId();
			gswindow.display = obs_get_nix_platform_display();
			break;
#ifdef ENABLE_WAYLAND
		case OBS_NIX_PLATFORM_WAYLAND: {
			QPlatformNativeInterface* native =
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

QMessageBox::StandardButton OBSMessageBox::question(QWidget *parent, const QString &title, const QString &text,
						    QMessageBox::StandardButtons buttons,
						    QMessageBox::StandardButton defaultButton)
{
	QMessageBox mb(QMessageBox::Question, title, text, QMessageBox::NoButton, parent);
	mb.setDefaultButton(defaultButton);

	if (buttons & QMessageBox::Ok) {
		QPushButton *button = mb.addButton(QMessageBox::Ok);
		button->setText(tr("OK"));
	}
#define add_button(x)                                               \
	if (buttons & QMessageBox::x) {                             \
		QPushButton *button = mb.addButton(QMessageBox::x); \
		button->setText(tr(#x));                            \
	}
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

void OBSMessageBox::information(QWidget *parent, const QString &title, const QString &text)
{
	QMessageBox mb(QMessageBox::Information, title, text, QMessageBox::NoButton, parent);
	mb.addButton(tr("OK"), QMessageBox::AcceptRole);
	mb.exec();
}

void OBSMessageBox::warning(QWidget *parent, const QString &title, const QString &text, bool enableRichText)
{
	QMessageBox mb(QMessageBox::Warning, title, text, QMessageBox::NoButton, parent);
	if (enableRichText)
		mb.setTextFormat(Qt::RichText);
	mb.addButton(tr("OK"), QMessageBox::AcceptRole);
	mb.exec();
}

void OBSMessageBox::critical(QWidget *parent, const QString &title, const QString &text)
{
	QMessageBox mb(QMessageBox::Critical, title, text, QMessageBox::NoButton, parent);
	mb.addButton(tr("OK"), QMessageBox::AcceptRole);
	mb.exec();
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

QDataStream &operator<<(QDataStream &out, const std::vector<std::shared_ptr<OBSSignal>> &)
{
	return out;
}

QDataStream &operator>>(QDataStream &in, std::vector<std::shared_ptr<OBSSignal>> &)
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
	explicit inline QuickThread(std::function<void()> func_) : func(func_) {}

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
		QMetaObject::invokeMethod(&eventLoop, "quit", Qt::QueuedConnection);
	};

	os_atomic_inc_long(&insideEventLoop);
	QScopedPointer<QThread> thread(CreateQThread(wait));
	thread->start();
	eventLoop.exec();
	thread->wait();
	os_atomic_dec_long(&insideEventLoop);
}

void ExecuteFuncSafeBlockMsgBox(std::function<void()> func, const QString &title, const QString &text)
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

void ExecThreadedWithoutBlocking(std::function<void()> func, const QString &title, const QString &text)
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

bool IsDateBeforeToday(const QString& date)
{
	QDate readDate = QDateTime::fromString(date, "yyyy-MM-dd").date();

	if(readDate.isValid())
	{
		QDate currentDate = QDateTime::currentDateTime().date();
		if(readDate < currentDate)
			return true;
		else
			return false;
	}

	return true;
}

void SetComboItemEnabled(QComboBox *c, int idx, bool enabled)
{
	QStandardItemModel *model = dynamic_cast<QStandardItemModel *>(c->model());
	QStandardItem *item = model->item(idx);
	item->setFlags(enabled ? Qt::ItemIsSelectable | Qt::ItemIsEnabled : Qt::NoItemFlags);
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

//void setClasses(QWidget *widget, const QString &newClasses)
//{
//	if (widget->property("class").toString() != newClasses) {
//		widget->setProperty("class", newClasses);
//
//		/* force style sheet recalculation */
//		QString qss = widget->styleSheet();
//		widget->setStyleSheet("/* */");
//		widget->setStyleSheet(qss);
//	}
//}

QString SelectDirectory(QWidget *parent, QString title, QString path)
{
	QString dir = QFileDialog::getExistingDirectory(parent, title, path,
							QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	return dir;
}

QString SaveFile(QWidget *parent, QString title, QString path, QString extensions)
{
	QString file = QFileDialog::getSaveFileName(parent, title, path, extensions);

	return file;
}

QString OpenFile(QWidget *parent, QString title, QString path, QString extensions)
{
	QString file = QFileDialog::getOpenFileName(parent, title, path, extensions);

	return file;
}

QStringList OpenFiles(QWidget *parent, QString title, QString path, QString extensions)
{
	QStringList files = QFileDialog::getOpenFileNames(parent, title, path, extensions);

	return files;
}

QString EncodeRFC3986(const QString& text)
{
	QByteArray encoded = QUrl::toPercentEncoding(text, "", "._~-");
	return QString(encoded);
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

void TruncateTextToLabelWidth(QLabel* label, QString newText, int space)
{
	QFontMetrics fontMetrics(label->font());

	if(fontMetrics.horizontalAdvance(newText) > label->width() - space) {
		QString elidedText = fontMetrics.elidedText(newText, Qt::ElideRight, label->width() - space);
		label->setText(elidedText);
		label->setToolTip(newText);
	} else {
		label->setText(newText);
		label->setToolTip("");
	}
}

void RefreshToolBarStyling(QToolBar *toolBar)
{
	for (QAction *action : toolBar->actions()) {
		QWidget *widget = toolBar->widgetForAction(action);

		if (!widget)
			continue;

		PolishStyleSheet(widget);
	}
}

void LoadIconFromABSPath(const char* iconPath, QIcon& iconVariable)
{
	std::string imgPath;
	GetDataFilePath(iconPath, imgPath);
	iconVariable = QIcon(imgPath.data());
}

void SplitString(const std::string& inputStr, std::vector<std::string>& outputVec, char delimiter)
{
	outputVec.clear();

	std::stringstream ss(inputStr);
	std::string item;

	while(getline(ss, item, delimiter)) {
		outputVec.push_back(item);
	}
}

void RemoveChar(std::string& str, char charToRemove)
{
	str.erase(std::remove(str.begin(), str.end(), charToRemove), str.end());
}

void ChangeStyleSheet(QWidget* target, QString styleSheet)
{
	if(target)
		target->setStyleSheet(styleSheet);
}

void SetPropAndPolishStyleSheet(QWidget* widget, const char* name, const QVariant& value)
{
	widget->setProperty(name, value);
	PolishStyleSheet(widget);
}

void PolishStyleSheet(QWidget* widget) {

	if(!widget)
		return;

	widget->style()->unpolish(widget);
	widget->style()->polish(widget);
}

void RemoveAllChildInLayout(QLayout* layout, bool deleteChild)
{
	while(QLayoutItem* item = layout->takeAt(0))
	{
		if(QWidget* widget = item->widget())
		{
			layout->removeWidget(widget);
			if(deleteChild)
			{
				if(widget)
				{
					widget->setParent(nullptr);
					delete widget;
				}
			}
		} else if(QSpacerItem* spacer = item->spacerItem())
		{
			layout->removeItem(spacer);
			delete spacer;
		}
	}
}

QPixmap* SetTransparentImage(QString imagePath, qreal transparentValue)
{
	QPixmap pixmap(imagePath);
	if(pixmap.isNull())
		return nullptr;

	QPixmap* transparentPixmap = new QPixmap(pixmap.size());
	if(!transparentPixmap)
		return nullptr;

	qreal transparent = transparentValue;
	if(transparentValue < 0.0 || transparentValue > 1.0) {
		transparent = 1.0;
	}

	transparentPixmap->fill(Qt::transparent);

	QPainter painter(transparentPixmap);
	painter.setOpacity(transparent);
	painter.drawPixmap(0, 0, pixmap);
	painter.end();

	return transparentPixmap;
}

void ShowTooltip(QWidget* widget)
{
	if(widget)
	{
		QVariant tooltipVariant = widget->property("tooltip").toString();
		if(tooltipVariant.isValid())
		{
			QString tooltipString = tooltipVariant.toString();

			QFont font = QToolTip::font();
			QFontMetrics fm(font);
			QRect tooltipRect = fm.boundingRect(tooltipString);

			QVariant edgeVariant = widget->property("tooltipEdge");
			Qt::Edge edgeTooltip = Qt::RightEdge;

			if(edgeVariant.isValid())
				if(edgeVariant.canConvert<Qt::Edge>())
					edgeTooltip = edgeVariant.value<Qt::Edge>();

			QPoint adjustPos = QPoint(0, 0);
			QPoint basicadjustPos = QPoint(widget->width() + 5, -(tooltipRect.height()/2 - 3));

			switch(edgeTooltip)
			{
				case Qt::TopEdge:
					basicadjustPos = QPoint(0, -(widget->height() / 2 + 5));
					break;
				case Qt::BottomEdge:
					basicadjustPos = QPoint(0, widget->height()/2 + 5);
					break;
				case Qt::LeftEdge:
					basicadjustPos = QPoint(-(widget->width() + 5), -5);
					break;
			}

			QVariant tooltipAdjustPos = widget->property("tooltipAdjustPos");

			if(tooltipAdjustPos.isValid())
				if(tooltipAdjustPos.canConvert<QPoint>())
					adjustPos = tooltipAdjustPos.value<QPoint>();


			QPoint globalPos = widget->mapToGlobal(QPoint(0, 0)) + basicadjustPos + adjustPos;
			QToolTip::showText(globalPos, tooltipString, widget);
		}
	}
}

bool IsScrollBarNeeded(QScrollArea* scrollArea, QWidget* widget, Qt::Orientation orientation)
{
	if(!scrollArea || !widget)
		return false;

	if(Qt::Orientation::Vertical == orientation)
		return scrollArea->height() <= widget->height();
	else
		return scrollArea->width() <= widget->width();

	return false;
}

void SetScrollBarTransparent(QScrollArea* scrollArea, QWidget* widget,
	bool transparent, Qt::Orientation orientation)
{
	if(!scrollArea || !widget)
		return;

	bool transparent_ = transparent;
	QScrollBar* scrollbar = nullptr;
	if(Qt::Orientation::Vertical == orientation) {
		scrollbar = scrollArea->verticalScrollBar();
	} else {
		scrollbar = scrollArea->horizontalScrollBar();
	}

	if(scrollbar) {
		if(!transparent && !IsScrollBarNeeded(scrollArea, widget, orientation))
			transparent_ = true;

		if(scrollbar->property("transparent").toBool() != transparent_)
		{
			scrollbar->setProperty("transparent", transparent_);
			PolishStyleSheet(scrollbar);
		}
	}
}

std::string createRequestCefScript(const std::string& key,
									const std::string& action,
									const std::string& data)
{
	return "requestCefQuery('" + key + "', '" + action + "', '" + data + "');";
}

std::string EscapeForJavaScript(const std::string& input)
{
	std::string escapedJson;
	escapedJson.reserve(input.size() * 2);

	for(char c : input) {
		switch(c) {
			case '"': escapedJson += "\\\""; break;
			case '\'': escapedJson += "\\'"; break;
			case '\\': escapedJson += "\\\\"; break;
			case '\n': escapedJson += "\\n"; break;
			default: escapedJson += c; break;
		}
	}
	return escapedJson;
}

void SetSvgToLabel(QLabel* label, const QString& svgPath, const QSize& size)
{
	QPixmap pixmap(size);
	pixmap.fill(Qt::transparent);

	QSvgRenderer renderer(svgPath);
	QPainter painter(&pixmap);
	renderer.render(&painter);

	label->setPixmap(pixmap);
}

void soop_url_encode(const char* input, char* output)
{
    int opt_inx, ipt_inx;

    for(ipt_inx = 0, opt_inx = 0; input[ipt_inx]; ipt_inx++, opt_inx++)
    {
        int char_val = input[ipt_inx];
        if(char_val < 0) char_val += 256;
        if(
            char_val <= 0x1F ||
            char_val == 0x7F ||
            char_val >= 0x80 ||
            char_val == ' ' ||
            char_val == '{' ||
            char_val == '}' ||
            char_val == '[' ||
            char_val == ']' ||
            char_val == '|' ||
            char_val == '\\' ||
            char_val == '^' ||
            char_val == '~' ||
            char_val == '`' ||
            char_val == '#' ||
            char_val == ';' ||
            char_val == '/' ||
            char_val == '?' ||
            char_val == '@' ||
            char_val == '=' ||
            char_val == '+' ||
            char_val == '&')
        {
            output[opt_inx] = '%';

            int UpperBit = char_val / 0x10;

            if(UpperBit >= 0 && UpperBit <= 9)
                output[++opt_inx] = UpperBit + '0';
            else
                output[++opt_inx] = UpperBit + 'A' - 10;

            int LowerBit = char_val % 0x10;
            if(LowerBit >= 0 && LowerBit <= 9)
                output[++opt_inx] = LowerBit + '0';
            else
                output[++opt_inx] = LowerBit + 'A' - 10;
        } else
            output[opt_inx] = char_val;
    }

    output[opt_inx] = 0;
}