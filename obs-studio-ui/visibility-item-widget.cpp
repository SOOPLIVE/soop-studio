#include "visibility-item-widget.hpp"

#include "source-label.hpp"

#include <qt-wrappers.hpp>
#include <QListWidget>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QLabel>
#include <QKeyEvent>
#include <QCheckBox>

VisibilityItemWidget::VisibilityItemWidget(obs_source_t *source_)
    : source(source_),
    enabledSignal(obs_source_get_signal_handler(source), "enable", OBSSourceEnabled, this),
    renamedSignal(obs_source_get_signal_handler(source), "rename", OBSSourceRenamed, this)
{
	const char* name = obs_source_get_name(source);
	bool enabled = obs_source_enabled(source);

	vis = new QCheckBox();
	vis->setProperty("class", "checkbox-icon indicator-visibility");
	vis->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	vis->setFixedSize(QSize(24, 24));
	vis->setIconSize(QSize(24, 24));
	vis->setChecked(enabled);
	vis->hide();

	label = new AFQElidedSlideLabel(this);
	label->setText(QT_UTF8(name));
	label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	label->setStyleSheet(QString("font-size: 14px;"));

	QHBoxLayout *itemLayout = new QHBoxLayout();
	itemLayout->addWidget(label);
	itemLayout->addWidget(vis);
	itemLayout->setContentsMargins(0, 0, 0, 0);

	setLayout(itemLayout);

	SetItemActive(enabled);

	connect(vis, &QCheckBox::clicked, [this](bool visible) {
		obs_source_set_enabled(source, visible);
		SetItemActive(visible);
	});

	connect(this, &VisibilityItemWidget::qsignalHoverWidget, label, &AFQElidedSlideLabel::qslotHoverButton);
	connect(this, &VisibilityItemWidget::qsignalLeaveWidget, label, &AFQElidedSlideLabel::qslotLeaveButton);
}

void VisibilityItemWidget::OBSSourceEnabled(void *param, calldata_t *data)
{
	VisibilityItemWidget *window = reinterpret_cast<VisibilityItemWidget *>(param);
	bool enabled = calldata_bool(data, "enabled");

	QMetaObject::invokeMethod(window, "SourceEnabled", Q_ARG(bool, enabled));
}

void VisibilityItemWidget::OBSSourceRenamed(void* param, calldata_t* data)
{
	VisibilityItemWidget* window = reinterpret_cast<VisibilityItemWidget*>(param);
	const char* name = calldata_string(data, "new_name");

	QMetaObject::invokeMethod(window, "SourceRenamed", Q_ARG(QString, QT_UTF8(name)));
}

void VisibilityItemWidget::SourceEnabled(bool enabled)
{
	if (vis->isChecked() != enabled)
		vis->setChecked(enabled);
}

void VisibilityItemWidget::SourceRenamed(QString name)
{
	if(label && name != label->text())
		label->setText(name);
}
//
void VisibilityItemWidget::enterEvent(QEnterEvent* event)
{
	if(vis)
		vis->show();

	emit qsignalHoverWidget(QString());
}
void VisibilityItemWidget::leaveEvent(QEvent* event)
{
	if(vis)
		vis->hide();

	emit qsignalLeaveWidget();
}

//
void VisibilityItemWidget::SetColor(const QColor &color)
{
	QPalette pal = vis->palette();
	pal.setColor(QPalette::WindowText, color);
	vis->setPalette(pal);

	label->setStyleSheet(QString("font-size: 14px; color: %1;").arg(color.name()));
}

QColor VisibilityItemWidget::GetItemFontColor(bool active, bool selected)
{
	QColor color;
	if(active && selected)
		color = QColor(252, 252, 253);
	else if(!active && selected)
		color = QColor(117, 123, 138);
	else if(active && !selected)
		color = QColor(213, 215, 220);
	else if(!active && !selected)
		color = QColor(117, 123, 138);

	return color;
}

void VisibilityItemWidget::SetItemState(bool active_, bool selected_)
{
	if(active_ == active && selected_ == selected)
		return;

	QColor color = GetItemFontColor(active_, selected_);
	SetColor(color);

	active = active_;
	selected = selected_;
}

void VisibilityItemWidget::SetItemActive(bool active_)
{
	if(active_ == active)
		return;

	QColor color = GetItemFontColor(active_, selected);
	SetColor(color);

	active = active_;
}

void VisibilityItemWidget::SetItemSelected(bool selected_)
{
	if(selected_ == selected)
		return;

	QColor color = GetItemFontColor(active, selected_);
	SetColor(color);

	selected = selected_;
}

VisibilityItemDelegate::VisibilityItemDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void VisibilityItemDelegate::paint(QPainter *painter,
								   const QStyleOptionViewItem &option,
								   const QModelIndex &index) const
{
	/*QStyledItemDelegate::paint(painter, option, index);

	QObject *parentObj = parent();
	QListWidget *list = qobject_cast<QListWidget *>(parentObj);
	if (!list)
		return;

	QListWidgetItem *item = list->item(index.row());
	VisibilityItemWidget *widget = qobject_cast<VisibilityItemWidget *>(list->itemWidget(item));
	if (!widget)
		return;

	bool selected = option.state.testFlag(QStyle::State_Selected);
	bool active = option.state.testFlag(QStyle::State_Active);

	QPalette palette = list->palette();
#if defined(_WIN32) || defined(__APPLE__)
	QPalette::ColorGroup group = active ? QPalette::Active : QPalette::Inactive;
#else
	QPalette::ColorGroup group = QPalette::Active;
#endif

#ifdef _WIN32
	QPalette::ColorRole highlightRole = QPalette::WindowText;
#else
	QPalette::ColorRole highlightRole = QPalette::HighlightedText;
#endif

	QPalette::ColorRole role;

	if (selected && active)
		role = highlightRole;
	else
		role = QPalette::WindowText;

	widget->SetColor(palette.color(group, role), active, selected);*/

	QStyleOptionViewItem optionNoFocus = option;
	optionNoFocus.state &= ~QStyle::State_HasFocus;
	QStyledItemDelegate::paint(painter, optionNoFocus, index);
}

bool VisibilityItemDelegate::eventFilter(QObject *object, QEvent *event)
{
	QWidget *editor = qobject_cast<QWidget *>(object);
	if (!editor)
		return false;

	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

		switch(keyEvent->key())
		{
			case Qt::Key_Escape:
			{
				QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
				if(lineEdit) {
					lineEdit->undo();
				}
			}
			break;

			case Qt::Key_Tab:
			case Qt::Key_Backtab:
				return false;
		}
	}

	return QStyledItemDelegate::eventFilter(object, event);
}

void SetupVisibilityItem(QListWidget *list, QListWidgetItem *item, obs_source_t *source)
{
	VisibilityItemWidget *baseWidget = new VisibilityItemWidget(source);

	list->setItemWidget(item, baseWidget);
}
