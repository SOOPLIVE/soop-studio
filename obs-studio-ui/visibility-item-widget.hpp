#pragma once

#include <QWidget>
#include <QStyledItemDelegate>
#include <obs.hpp>

#include "UIComponent/CElidedSlideLabel.h"

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QCheckBox;
class AFQElidedSlideLabel;

class VisibilityItemWidget : public QWidget {
	Q_OBJECT

private:
	OBSSource source;
	AFQElidedSlideLabel*label = nullptr;
	QCheckBox *vis = nullptr;

	OBSSignal enabledSignal;
	OBSSignal renamedSignal;

	bool active = false;
	bool selected = false;

	static void OBSSourceEnabled(void *param, calldata_t *data);
	static void OBSSourceRenamed(void* param, calldata_t* data);

signals:
	void qsignalHoverWidget(QString);
	void qsignalLeaveWidget();

private slots:
	void SourceEnabled(bool enabled);
	void SourceRenamed(QString name);

protected:
	virtual void enterEvent(QEnterEvent* event) override;
	virtual void leaveEvent(QEvent* event) override;

public:
	VisibilityItemWidget(obs_source_t *source);

	void SetColor(const QColor &color);
	QColor  GetItemFontColor(bool active, bool selected);
	void	SetItemState(bool active, bool selected);
	void	SetItemActive(bool active);
	void	SetItemSelected(bool selected);
};

class VisibilityItemDelegate : public QStyledItemDelegate {
	Q_OBJECT

public:
	VisibilityItemDelegate(QObject *parent = nullptr);

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
	bool eventFilter(QObject *object, QEvent *event) override;
};

void SetupVisibilityItem(QListWidget *list, QListWidgetItem *item, obs_source_t *source);
