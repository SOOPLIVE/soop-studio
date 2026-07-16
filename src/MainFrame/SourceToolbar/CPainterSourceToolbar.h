#pragma once

#include <QApplication>
#include <QFrame>
#include <QLayout>
#include <QTimer>
#include <QKeyEvent>
#include <QFocusEvent>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"


namespace Ui {
	class AFQPainterSourceToolbar;
}

class CMenuStyleFrame : public QFrame {
	Q_OBJECT
public:
	explicit CMenuStyleFrame(QWidget* parent = nullptr)
		: QFrame(parent)
	{
		setWindowFlags(Qt::Popup);
		setFrameShape(QFrame::StyledPanel);
		setAttribute(Qt::WA_DeleteOnClose);
		setContentsMargins(0, 0, 0, 0);

		setStyleSheet("QFrame { border: 1px solid #111111; }");

		layout = new QHBoxLayout(this);
		layout->setSpacing(0);
		setFocusPolicy(Qt::StrongFocus);
	}

	void setMargins(int l, int t, int r, int b) {
		layout->setContentsMargins(l, t, r, b);
	}
	void addWidget(QWidget* w) {
		layout->addWidget(w);
	}

protected:
	void focusOutEvent(QFocusEvent* e) override {
		QFrame::focusOutEvent(e);
		QTimer::singleShot(0, this, [this]() {
			QWidget* fw = QApplication::focusWidget();
			if (!fw || (!this->isAncestorOf(fw) && fw != this)) {
				close();
			}
			});
	}

	void keyPressEvent(QKeyEvent* e) override {
		if (e->key() == Qt::Key_Escape) { close(); return; }
		QFrame::keyPressEvent(e);
	}

private:
	QHBoxLayout* layout = nullptr;
};

class AFQPainterSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT

public:
	explicit AFQPainterSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQPainterSourceToolbar();

private slots:
	void qslotToolClicked(int id);
	void qslotUndoDraw();
	void qslotRedoDraw();
	void qslotResetPainter();

	void qslotShowThicknessControl(int checked);
	void qslotShowToolColorControl(int checked);

private:
	QIcon getToolColorIcon(int r, int g, int b, int buttonSize, int circleSize);
	void setToolColorButton(int r, int g, int b);

	QPoint popupPosUnderCenter(QWidget* widget, QWidget* popup);

private:
	Ui::AFQPainterSourceToolbar* ui;

	CMenuStyleFrame* thicknessPopup = nullptr;
	CMenuStyleFrame* colorPopup = nullptr;
};