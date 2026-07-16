#include "CCustomFontDialog.h"

#include "ui_custom-font-dialog.h"

#include <QListView>

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
	AFTTopBaseDialog((QDialog*)parent),
	ui(new Ui::AFQCustomFontDialog)
{
	ui->setupUi(this);

#ifdef _WIN32
	ui->titleFrame->setProperty("MoveInAllArea", true);
#elif defined(__APPLE__)
    setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    ui->titleFrame->hide();
#endif

	m_pFontFrame = new QFontDialog(initial, this);
	m_pFontFrame->setWindowFlags(Qt::Widget);

	connect(m_pFontFrame, &QDialog::accepted, this, &AFQCustomFontDialog::qslotAccept);
	connect(m_pFontFrame, &QDialog::rejected, this, &AFQCustomFontDialog::qslotReject);
	connect(ui->closeButton, &QPushButton::clicked , this, &AFQCustomFontDialog::qslotReject);
	
	ui->labelTitle->setText(title);
	ui->layoutFontPicker->addWidget((QWidget*)m_pFontFrame);

	QList<QListView*> listViews = m_pFontFrame->findChildren<QListView*>();
	for (QListView* listView : listViews) {
		NoFocusListTempDelegate* delegate = new NoFocusListTempDelegate(m_pFontFrame);
		listView->setItemDelegate(delegate);
	}

	SetWidthResizeEnabled(false);
}

AFQCustomFontDialog::~AFQCustomFontDialog()
{

}

QFont AFQCustomFontDialog::getFont(bool* ok, const QFont& initial, QWidget* parent,
								  const QString& title,
								  QFontDialog::FontDialogOptions options)
{
	AFQCustomFontDialog dlg(ok, initial, title, parent, options);

	dlg.m_pFontFrame->setOptions(options);
	dlg.m_pFontFrame->setCurrentFont(initial);
	if (!title.isEmpty())
		dlg.setWindowTitle(title);

	int ret = (dlg.exec() || (options & QFontDialog::NoButtons));
	if (ok)
		*ok = !!ret;
	if (ret) {
		return dlg.m_pFontFrame->selectedFont();
	}
	else {
		return initial;
	}
}

void AFQCustomFontDialog::qslotAccept()
{
	this->accept();
}

void AFQCustomFontDialog::qslotReject()
{
	this->reject();
}
