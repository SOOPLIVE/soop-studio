#include "CNameDialog.h"

#include <QVBoxLayout>

#include "qt-wrapper.h"
#include "platform/platform.hpp"

#include "Application/CApplication.h"

static bool IsWhitespace(char ch)
{
	return ch == ' ' || ch == '\t';
}

static void CleanWhitespace(std::string& str)
{
	while (str.size() && IsWhitespace(str.back()))
		str.erase(str.end() - 1);
	while (str.size() && IsWhitespace(str.front()))
		str.erase(str.begin());
}

AFQNameDialog::AFQNameDialog(QWidget* parent)
	: AFQRoundedDialogBase((QDialog*)parent)
{
	installEventFilter(CreateShortcutFilter());
	setModal(true);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	setFixedWidth(410);
	setMinimumHeight(246);
	setObjectName("dialog_NamedDialog");

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	QFrame* titleFrame = new QFrame(this);
	titleFrame->setFixedHeight(38);

	QHBoxLayout* titleLayout = new QHBoxLayout(this);
	titleLayout->setContentsMargins(18, 6, 8, 6);
	titleFrame->setLayout(titleLayout);

	m_pLabelTitle = new QLabel(this);
	m_pLabelTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	m_pLabelTitle->setProperty("labelType", "labelTitle");

	QPushButton* closeButton = new QPushButton(this);
	closeButton->setFixedSize(24, 24);
	closeButton->setIconSize(QSize(11, 11));
	//closeButton->setIcon(QIcon(":/image/resource/close.svg"));
	//closeButton->setProperty("buttonType", "popupTitleBarButton");
	closeButton->setProperty("buttonType", "closeButton");

	titleLayout->addWidget(m_pLabelTitle);
	titleLayout->addWidget(closeButton);

	layout->addWidget(titleFrame);

	m_pLabel = new QLabel(this);
	m_pLabel->setText("-");
	layout->addWidget(m_pLabel, 0, Qt::AlignCenter);
	layout->addSpacerItem(new QSpacerItem(0, 10, QSizePolicy::Expanding, QSizePolicy::Fixed));

	m_pUserText = new QLineEdit(this);
	m_pUserText->setFixedSize(350, 40);
	layout->addWidget(m_pUserText, 0, Qt::AlignCenter);
	layout->addSpacerItem(new QSpacerItem(0, 24, QSizePolicy::Expanding, QSizePolicy::Fixed));

	m_pCheckbox = new QCheckBox(this);
	layout->addWidget(m_pCheckbox, 0, Qt::AlignCenter);

	QDialogButtonBox* buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonbox->setCenterButtons(true);
	buttonbox->button(QDialogButtonBox::Ok)->setFixedSize(128,40);
	buttonbox->button(QDialogButtonBox::Cancel)->setFixedSize(128, 40);

	connect(buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

	layout->addWidget(buttonbox);
	layout->addSpacerItem(new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Fixed));

	setLayout(layout);

	this->SetWidthFixed(true);
}

AFQNameDialog::~AFQNameDialog()
{

}

bool AFQNameDialog::AskForName(QWidget* parent, const QString& title,
	const QString& text, std::string& userTextInput,
	const QString& placeHolder, int maxSize, bool isNotParent)
{
	if (maxSize <= 0 || maxSize > 32767)
		maxSize = 170;

	QWidget* dialogParent = (isNotParent ? nullptr : parent);

	AFQNameDialog dialog(dialogParent);
	dialog.setWindowTitle(title);

	if (isNotParent) {
		dialog.adjustSize();
		setCenterPositionNotUseParent(&dialog, parent);
	}

	dialog.m_pLabelTitle->setText(title);
	dialog.m_pCheckbox->setHidden(true);
	dialog.m_pLabel->setText(text);
	dialog.m_pUserText->setMaxLength(maxSize);
	dialog.m_pUserText->setText(placeHolder);
	dialog.m_pUserText->setFocus();
	dialog.m_pUserText->selectAll();

	if (dialog.exec() != DialogCode::Accepted) {
		return false;
	}
	userTextInput = dialog.m_pUserText->text().toUtf8().constData();
	CleanWhitespace(userTextInput);
	return true;
}

bool AFQNameDialog::AskForNameWithOption(QWidget* parent, const QString& title,
	const QString& text,
	std::string& userTextInput,
	const QString& optionLabel,
	bool& optionChecked,
	const QString& placeHolder)
{
	AFQNameDialog dialog(parent);
	dialog.setWindowTitle(title);

	dialog.m_pLabel->setText(text);
	dialog.m_pUserText->setMaxLength(170);
	dialog.m_pUserText->setText(placeHolder);
	dialog.m_pCheckbox->setText(optionLabel);
	dialog.m_pCheckbox->setChecked(optionChecked);

	if (dialog.exec() != DialogCode::Accepted) {
		return false;
	}

	userTextInput = dialog.m_pUserText->text().toUtf8().constData();
	CleanWhitespace(userTextInput);
	optionChecked = dialog.m_pCheckbox->isChecked();
	return true;
}
