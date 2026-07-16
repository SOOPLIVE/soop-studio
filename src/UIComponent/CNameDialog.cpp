#include "CNameDialog.h"

#include <QVBoxLayout>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"
#include "Application/CApplication.h"

#include "MainFrame/CMainFrame.h"

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
	: AFTTopBaseDialog((QDialog*)parent)
{

	SetWidthResizeEnabled(false);
	SetHeightResizeEnabled(false);

	installEventFilter(CreateShortcutFilter());
	setModal(true);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	setObjectName("dialog_NamedDialog");
		
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 10);
	layout->setSpacing(0);

#ifdef _WIN32
	//5pix top spacing
    QFrame* titleFrame = new QFrame(this);
    titleFrame->setObjectName("titleFrame");
    titleFrame->setProperty("MoveInAllArea", true);
    titleFrame->setFixedHeight(35);

    QHBoxLayout* titleLayout = new QHBoxLayout(titleFrame);
    titleLayout->setContentsMargins(12, 3, 8, 10);

    m_pLabelTitle = new QLabel(this);
    m_pLabelTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_pLabelTitle->setProperty("labelType", "labelTitle");

    QPushButton* closeButton = new QPushButton(this);
    closeButton->setFixedSize(24, 24);
    closeButton->setIconSize(QSize(11, 11));
    closeButton->setProperty("buttonType", "closeButton");

    titleLayout->addWidget(m_pLabelTitle);
    titleLayout->addWidget(closeButton);

	titleFrame->setLayout(titleLayout);
    layout->addWidget(titleFrame);
    
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
#elif defined(__APPLE__)
    setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
#endif
	QWidget* editWidget = new QWidget(this);
	editWidget->setFixedSize(478, 112);
	QVBoxLayout* editLayout = new QVBoxLayout();
	editLayout->setContentsMargins(30, 20, 30, 30);
	editLayout->setSpacing(40);

	m_pLabel = new QLabel(this);
	m_pLabel->setText("-");
	m_pLabel->setObjectName("label_MessageBoxText");
	m_pLabel->setProperty("labelType", "labelTitle");
	m_pLabel->setFixedHeight(30);
	editLayout->addWidget(m_pLabel, 0, Qt::AlignCenter);

	m_pUserText = new QLineEdit(this);
	m_pUserText->setFixedSize(418, 40);
	editLayout->addWidget(m_pUserText, 0, Qt::AlignCenter);

	//m_pCheckbox = new QCheckBox(this);
	//editLayout->addWidget(m_pCheckbox, 0, Qt::AlignCenter);

	editWidget->setLayout(editLayout);

	layout->addWidget(editWidget);

	QPushButton* okButton = new QPushButton(this);
	okButton->setText(QTStr("OK"));
	okButton->setFixedSize(100, 40);
	okButton->setProperty("pushButtonTheme", "type2");
	PolishStyleSheet(okButton);

	QPushButton* cancelButton = new QPushButton(this);
	cancelButton->setText(QTStr("Cancel"));
	cancelButton->setFixedSize(100, 40);
	cancelButton->setProperty("pushButtonTheme", "type4");
	PolishStyleSheet(cancelButton);

	connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
	connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

	QWidget* buttonWidget = new QWidget(this);
	buttonWidget->setFixedSize(478, 70);
	QHBoxLayout* buttonBoxLayout = new QHBoxLayout(buttonWidget);
	buttonBoxLayout->setContentsMargins(0, 0, 0, 0);
	buttonBoxLayout->setSpacing(10);
	buttonBoxLayout->addSpacerItem(new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Fixed));
	buttonBoxLayout->addWidget(cancelButton);
	buttonBoxLayout->addWidget(okButton);
	buttonBoxLayout->addSpacerItem(new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Fixed));
	buttonWidget->setLayout(buttonBoxLayout);

	layout->addWidget(buttonWidget);

	//setLayout(layout);

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
	dialog.adjustSize();

	if (isNotParent) {
		setCenterPositionNotUseParent(&dialog, parent);
	}

#ifdef _WIN32
	dialog.m_pLabelTitle->setText(title);
#endif
	//dialog.m_pCheckbox->setHidden(true);
	dialog.m_pLabel->setText(text);
	dialog.m_pUserText->setMaxLength(maxSize);
	dialog.m_pUserText->setText(placeHolder);
	dialog.m_pUserText->setFocus();
	dialog.m_pUserText->selectAll();

	AFQBlockManager::ApplyMoveInAllArea(&dialog);
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
	//dialog.m_pCheckbox->setText(optionLabel);
	//dialog.m_pCheckbox->setChecked(optionChecked);

	if (dialog.exec() != DialogCode::Accepted) {
		return false;
	}

	userTextInput = dialog.m_pUserText->text().toUtf8().constData();
	CleanWhitespace(userTextInput);
	return true;
}

void AFQNameDialog::showEvent(QShowEvent* event)
{
#ifdef _WIN32
	setFixedSize(480, 260);
#elif defined(__APPLE__)
	setFixedSize(480, 220);
#endif
}
