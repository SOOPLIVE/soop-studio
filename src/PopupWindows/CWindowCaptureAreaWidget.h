#pragma once
#include <QObject>
#include <QEvent>
#include <QKeyEvent>

class EscKeyFilter : public QObject
{
	Q_OBJECT
public:
	explicit EscKeyFilter(QObject* parent = nullptr) : QObject(parent) {}

signals:
	void escapeKeyPressed();

protected:
	bool eventFilter(QObject* watched, QEvent* event) override
	{
		if (event->type() == QEvent::KeyPress) {
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent->key() == Qt::Key_Escape) {
				emit escapeKeyPressed();
				return true;
			}
		}
		return QObject::eventFilter(watched, event);
	}
};

#include "obs.hpp"

// ONLY Windows OS
#include <Windows.h>

#include <QWidget>
#include <QApplication>
#include <QPainter>
#include <QScreen>
#include <QMouseEvent>
#include <QPixmap>
#include <QRect>
#include <QPoint>
#include <QShowEvent>

struct window_area_info {
	HWND window;
	std::string id;
	std::string name;
	std::string className;
	std::string executableName;
};

class WindowCaptureAreaWidget : public QWidget 
{
	Q_OBJECT

	using properties_delete_t = decltype(&obs_properties_destroy);
	using properties_t =
		std::unique_ptr<obs_properties_t, properties_delete_t>;

public:
	static void launch(obs_source_t* source, QWidget* parent = nullptr);
	~WindowCaptureAreaWidget() override;

signals:
	void selectionOperationFinished();

private:
	explicit WindowCaptureAreaWidget(QScreen* screen, const QVector<window_area_info>& all_windows_info, obs_source_t* source, QWidget* parent = nullptr);	

protected:
	void paintEvent(QPaintEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;

	void keyPressEvent(QKeyEvent* event) override;

private:
	static QList<WindowCaptureAreaWidget*> s_activeInstances;

	OBSWeakSourceAutoRelease m_weakSource;
	std::unique_ptr<obs_properties_t, decltype(&obs_properties_destroy)> m_props;

	QScreen* m_screen = nullptr;
	QPixmap m_desktopPixmap;

	QVector<window_area_info> m_allWindowsInfo;

	QRect m_rcHighlightedRect;
	int m_idx = -1;
	QRect m_targetWindowScreenRect;
	int m_targetIdx = -1;
	bool m_isDragging = false;
	QPoint m_dragStartPosition;
	QRect m_currentDragRect;
	QString m_dragRectSizeString;
	bool m_bMouseInside = false;
	QPoint m_mousePos;
};