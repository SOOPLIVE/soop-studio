#include "CCustomFontDialog.h"

#include "ui_custom-font-dialog.h"

#include <QListView>

// exist code : QStyledItemDelegate
#include <QStyledItemDelegate>
#include <QPainter>
class NoFocusListTempDelegate : public QStyledItemDelegate {
public:
	using QStyledItemDelegate::QStyledItemDelegate;

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
		QStyleOptionViewItem optionNoFocus = option;
		optionNoFocus.state &= ~QStyle::State_HasFocus;
		QStyledItemDelegate::paint(painter, optionNoFocus, index);
	}
};

AFQCustomFontDialog::AFQCustomFontDialog(bool* ok, const QFont& initial, const QString& title,
									   QWidget* parent, QFontDialog::FontDialogOptions options) :
	AFQRoundedDialogBase((QDialog*)parent),
	ui(new Ui::AFQCustomFontDialog)
{
	ui->setupUi(this);

	m_fontFrame = new QFontDialog(initial, this);
	m_fontFrame->setWindowFlags(Qt::Widget);

	connect(m_fontFrame, &QDialog::accepted, this, &AFQCustomFontDialog::qSlotAccept);
	connect(m_fontFrame, &QDialog::rejected, this, &AFQCustomFontDialog::qSlotAccept);
	connect(ui->closeButton, &QPushButton::clicked , this, &AFQCustomFontDialog::qSlotAccept);
	
	ui->labelTitle->setText(title);
	ui->layoutFontPicker->addWidget((QWidget*)m_fontFrame);

	QList<QListView*> listViews = m_fontFrame->findChildren<QListView*>();
	for (QListView* listView : listViews) {
		NoFocusListTempDelegate* delegate = new NoFocusListTempDelegate(m_fontFrame);
		listView->setItemDelegate(delegate);
	}

	setStyleSheet("AFQCustomFontDialog { border:1px solid #111 }");

	this->SetWidthFixed(true);
}

AFQCustomFontDialog::~AFQCustomFontDialog()
{

}

QFont AFQCustomFontDialog::getFont(bool* ok, const QFont& initial, QWidget* parent,
								  const QString& title,
								  QFontDialog::FontDialogOptions options)
{
	AFQCustomFontDialog dlg(ok, initial, title, parent, options);

	dlg.m_fontFrame->setOptions(options);
	dlg.m_fontFrame->setCurrentFont(initial);
	if (!title.isEmpty())
		dlg.setWindowTitle(title);

	int ret = (dlg.exec() || (options & QFontDialog::NoButtons));
	if (ok)
		*ok = !!ret;
	if (ret) {
		return dlg.m_fontFrame->selectedFont();;
	}
	else {
		return initial;
	}
}

void AFQCustomFontDialog::qSlotAccept()
{
	this->accept();
}

void AFQCustomFontDialog::qSlotReject()
{
	this->reject();
}