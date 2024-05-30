#pragma once

#include <QApplication>
#include <QMessageBox>
#include <QWidget>
#include <QWindow>
#include <QThread>
#include <QStyle>
#include <obs.hpp>

#include <functional>
#include <memory>
#include <vector>

#define QT_UTF8(str) QString::fromUtf8(str, -1)
#define QT_TO_UTF8(str) str.toUtf8().constData()
#define MAX_LABEL_LENGTH 80


class QDataStream;
class QComboBox;
class QWidget;
class QLayout;
class QString;
struct gs_window;
class QLabel;
class QToolBar;
class QIcon;

class AFCMessageBox {
public:
	static QMessageBox::StandardButton
		question(QWidget* parent, const QString& title, const QString& text,
			QMessageBox::StandardButtons buttons =
			QMessageBox::StandardButtons(QMessageBox::Yes |
				QMessageBox::No),
			QMessageBox::StandardButton defaultButton =
			QMessageBox::NoButton);
	static void information(QWidget* parent, const QString& title,
		const QString& text);
	static void warning(QWidget* parent, const QString& title,
		const QString& text, bool enableRichText = false);
	static void critical(QWidget* parent, const QString& title,
		const QString& text);
};

void AFErrorBox(QWidget* parent, const char* msg, ...);
void setCenterPositionNotUseParent(QWidget* target, QWidget* base);

bool QTToGSWindow(QWindow* window, gs_window& gswindow);

uint32_t TranslateQtKeyboardEventModifiers(Qt::KeyboardModifiers mods);

QDataStream&
operator<<(QDataStream& out,
	const std::vector<std::shared_ptr<OBSSignal>>& signal_vec);
QDataStream& operator>>(QDataStream& in,
	std::vector<std::shared_ptr<OBSSignal>>& signal_vec);
QDataStream& operator<<(QDataStream& out, const OBSScene& scene);
QDataStream& operator>>(QDataStream& in, OBSScene& scene);
QDataStream& operator<<(QDataStream& out, const OBSSource& source);
QDataStream& operator>>(QDataStream& in, OBSSource& source);

QThread* CreateQThread(std::function<void()> func);

void ExecuteFuncSafeBlock(std::function<void()> func);
void ExecuteFuncSafeBlockMsgBox(std::function<void()> func,
	const QString& title, const QString& text);

/* allows executing without message boxes if starting up, otherwise with a
 * message box */
void EnableThreadedMessageBoxes(bool enable);
void ExecThreadedWithoutBlocking(std::function<void()> func,
	const QString& title, const QString& text);
//

class SignalBlocker {
	QWidget* widget;
	bool blocked;

public:
	inline explicit SignalBlocker(QWidget* widget_) : widget(widget_)
	{
		blocked = widget->blockSignals(true);
	}

	inline ~SignalBlocker() { widget->blockSignals(blocked); }
};

void DeleteLayout(QLayout* layout);

static inline Qt::ConnectionType WaitConnection()
{
	return QThread::currentThread() == qApp->thread()
		? Qt::DirectConnection
		: Qt::BlockingQueuedConnection;
}

bool LineEditCanceled(QEvent* event);
bool LineEditChanged(QEvent* event);

void SetComboItemEnabled(QComboBox* c, int idx, bool enabled);

void setThemeID(QWidget* widget, const QString& themeID);

QString SelectDirectory(QWidget* parent, QString title, QString path);
QString SaveFile(QWidget* parent, QString title, QString path,
	QString extensions);
QString OpenFile(QWidget* parent, QString title, QString path,
	QString extensions);
QStringList OpenFiles(QWidget* parent, QString title, QString path,
	QString extensions);

void TruncateLabel(QLabel* label, QString newText,
	int length = MAX_LABEL_LENGTH);
void TruncateTextToLabelWidth(QLabel* label, QString newText);

void RefreshToolBarStyling(QToolBar* toolBar);

void LoadIconFromABSPath(const char* iconPath, QIcon& iconVariable);

template<typename KeyType, typename ValueType>
void ClearWidgetsInQMap(QMap<KeyType, ValueType*>& map)
{
	auto it = map.begin();
	while (it != map.end()) {
		auto next = it;
		++next;

		ValueType* v = (*it);
		if (v) {
			v->close();
			v = nullptr;
		}
		it = next;
	}
	map.clear();
}

#define STYLESHEET_RESET_BUTTON     QString("QPushButton {"     \
                                            "border: 1px solid rgba(255, 255, 255, 10%);" \
                                            "color: rgba(255, 255, 255, 80%);" \
                                            "font-size: 15px;" \
                                            "font-style: normal;" \
                                            "font-weight: 500;" \
                                            "line-height: normal;" \
                                            "background:transparent; }" \
                                            "QPushButton:hover {"     \
                                            "border: 1px solid #66686C; }" \
                                            "QPushButton:pressed {"     \
                                            "border: 1px solid rgba(255, 255, 255, 6%);" \
                                            "color: rgba(255, 255, 255, 16%); }"    \
                                            "QPushButton:disabled {"    \
                                            "color: rgba(255, 255, 255, 20%);"    \
                                            "}"    \
                                            )

void ChangeStyleSheet(QWidget* target, QString styleSheet);

void updateStyleSheet(QWidget* widget);
