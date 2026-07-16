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

#pragma once

#include <QApplication>
#include <QMessageBox>
#include <QWidget>
#include <QWindow>
#include <QThread>
#include <QStyle>
#include <QScrollArea>
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
class QLabel;
class QToolBar;

class OBSMessageBox : QObject {
	Q_OBJECT
public:
	static QMessageBox::StandardButton question(
		QWidget *parent, const QString &title, const QString &text,
		QMessageBox::StandardButtons buttons = QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No),
		QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
	static void information(QWidget *parent, const QString &title, const QString &text);
	static void warning(QWidget *parent, const QString &title, const QString &text, bool enableRichText = false);
	static void critical(QWidget *parent, const QString &title, const QString &text);
};

void OBSErrorBox(QWidget *parent, const char *msg, ...);

QPoint setCenterPositionNotUseParent(QWidget* target, QWidget* base);
void setTopRightPositionNotUseParent(QWidget* target, QWidget* base);
void EnsureDialogVisible(QDialog* dialog);	// Ensures the dialog stays within the monitor bounds

bool QTToGSWindow(QWindow* window, gs_window& gswindow);

uint32_t TranslateQtKeyboardEventModifiers(Qt::KeyboardModifiers mods);

QDataStream &operator<<(QDataStream &out, const std::vector<std::shared_ptr<OBSSignal>> &signal_vec);
QDataStream &operator>>(QDataStream &in, std::vector<std::shared_ptr<OBSSignal>> &signal_vec);
QDataStream &operator<<(QDataStream &out, const OBSScene &scene);
QDataStream &operator>>(QDataStream &in, OBSScene &scene);
QDataStream &operator<<(QDataStream &out, const OBSSource &source);
QDataStream &operator>>(QDataStream &in, OBSSource &source);

QThread *CreateQThread(std::function<void()> func);

void ExecuteFuncSafeBlock(std::function<void()> func);
void ExecuteFuncSafeBlockMsgBox(std::function<void()> func, const QString &title, const QString &text);

/* allows executing without message boxes if starting up, otherwise with a
 * message box */
void EnableThreadedMessageBoxes(bool enable);
void ExecThreadedWithoutBlocking(std::function<void()> func, const QString &title, const QString &text);

void DeleteLayout(QLayout *layout);

static inline Qt::ConnectionType WaitConnection()
{
	return QThread::currentThread() == qApp->thread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection;
}

bool LineEditCanceled(QEvent *event);
bool LineEditChanged(QEvent *event);

bool IsDateBeforeToday(const QString& date);

void SetComboItemEnabled(QComboBox *c, int idx, bool enabled);

void setThemeID(QWidget* widget, const QString& themeID);
//void setClasses(QWidget *widget, const QString &newClasses);

QString SelectDirectory(QWidget *parent, QString title, QString path);
QString SaveFile(QWidget *parent, QString title, QString path, QString extensions);
QString OpenFile(QWidget *parent, QString title, QString path, QString extensions);
QStringList OpenFiles(QWidget *parent, QString title, QString path, QString extensions);

QString EncodeRFC3986(const QString& text);

void TruncateLabel(QLabel *label, QString newText, int length = MAX_LABEL_LENGTH);
void TruncateTextToLabelWidth(QLabel* label, QString newText, int space = 0);

void RefreshToolBarStyling(QToolBar *toolBar);

void LoadIconFromABSPath(const char* iconPath, QIcon& iconVariable);

void SplitString(const std::string& inputStr, std::vector<std::string>& outputVec, char delimiter = ',');

void RemoveChar(std::string& str, char charToRemove);

template<typename KeyType, typename ValueType>
void ClearWidgetsInQMap(QMap<KeyType, ValueType*>& map)
{
	auto it = map.begin();
	while(it != map.end()) {
		auto next = it;
		++next;

		ValueType* v = (*it);
		if(v) {
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

void SetPropAndPolishStyleSheet(QWidget* widget, const char* name, const QVariant& value);
void PolishStyleSheet(QWidget* widget);

void RemoveAllChildInLayout(QLayout* layout, bool deleteChild = true);

QPixmap* SetTransparentImage(QString imagePath, qreal transparentValue);

void ShowTooltip(QWidget* widget);
#define SAFE_DELETE_WIDGET(widget)	if((widget)){ widget->close(); delete (widget); (widget) = nullptr; }


// for scrollArea Content 
enum class DragDirection { Top, Bottom, None };

// Used in eventFilter to hide the scrollbar
bool IsScrollBarNeeded(QScrollArea* scrollArea, QWidget* widget, Qt::Orientation orientation);
void SetScrollBarTransparent(QScrollArea* scrollArea, QWidget* widget,
	bool transparent, Qt::Orientation orientation = Qt::Orientation::Vertical);

// for text
std::string createRequestCefScript(const std::string& key, const std::string& action, const std::string& data);
std::string EscapeForJavaScript(const std::string& input);

void SetSvgToLabel(QLabel* label, const QString& svgPath, const QSize& size);

void soop_url_encode(const char* input, char* output);