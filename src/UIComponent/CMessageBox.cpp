#include "CMessageBox.h"

#include <QVBoxLayout>
#include <QFont>
#include <QFontMetrics>

#include "qt-wrappers.hpp"

#include "Application/CApplication.h"
#include "Blocks/CBlockManager.h"
#include "platform/platform.hpp"

#define MESSAGE_TITLE_HEIGHT 30
#define MESSAGE_BUTTONBOX_HEIGHT 40
#define MESSAGE_TEXT_WIDTH 370
#define MESSAGE_BOX_WIDTH 410

//void AFQMessageBox::accept()
//{
//	QDialog::accept();
//	close();
//}

AFQMessageBox::AFQMessageBox(QDialogButtonBox::StandardButtons buttons,
	QWidget* parent,
	const QString& title,
	const QString& text,
	bool useWordWrap,
	const QString& buttonText,
	const QString& topText,
	int fixedWidth, 
	int fixedHeight,
	QString buttonType,
	QString checkBoxText)
	: AFTTopBaseDialog{ parent }
{
	UNUSED_PARAMETER(title); //Title Frame Need

	setMinimumWidth(MESSAGE_BOX_WIDTH);

	if (fixedWidth > 0)
	{
		resize(fixedWidth, height());
		setFixedWidth(fixedWidth);
		SetWidthResizeEnabled(false);
	}
	else
	{
		setMinimumWidth(MESSAGE_BOX_WIDTH);
		SetWidthResizeEnabled(true);
	}

	if (fixedHeight > 0)
	{
		resize(width(), fixedHeight);
		setFixedHeight(fixedHeight);
		SetHeightResizeEnabled(false);
	}
	else
	{
		setMinimumHeight(200);
		SetHeightResizeEnabled(true);
	}

	setObjectName("dialog_MessageBox");

	QVBoxLayout* layout = new QVBoxLayout;
	layout->setContentsMargins(20, 30, 20, 20);
	layout->setSpacing(30);

	QVBoxLayout* textLayout = new QVBoxLayout;
	textLayout->setContentsMargins(0, 0, 0, 0);
	textLayout->setSpacing(10);

	if (topText != "")
	{
		QLabel* LabelTitle = new QLabel(this);
		LabelTitle->setObjectName("label_MessageBoxTitle");
		LabelTitle->setText(topText);
		LabelTitle->setAlignment(Qt::AlignCenter);
		textLayout->addWidget(LabelTitle, 0, Qt::AlignCenter);
	}

	QLabel* textLabel = new QLabel(this);
	textLabel->setText(text);
	textLabel->setAlignment(Qt::AlignCenter);

	if (topText != "")
		textLabel->setObjectName("label_SmallMessageBoxText");
	else
		textLabel->setObjectName("label_MessageBoxText");


	QRegularExpression linkRegex(
		R"(<a\b[^>]*\bhref\s*=\s*['"][^'"]+['"][^>]*>)",
		QRegularExpression::CaseInsensitiveOption);

	if (linkRegex.match(text).hasMatch()) {
		textLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
		textLabel->setOpenExternalLinks(true);
		textLabel->setProperty("showHandCursor", true);
	}

	textLayout->addWidget(textLabel, 0, Qt::AlignCenter);

	layout->addLayout(textLayout);

	QWidget* checkWidget = new QWidget(this);
	checkWidget->setFixedHeight(22);
	QHBoxLayout* checkLayout = new QHBoxLayout();
	checkLayout->setContentsMargins(0, 0, 0, 0);
	checkLayout->setAlignment(Qt::AlignCenter);

	if (checkBoxText != "")
	{
		m_checkBox = new QCheckBox(this);
		m_checkBox->setText(checkBoxText);
		m_checkBox->setFixedHeight(22);
		checkLayout->addWidget(m_checkBox);
		checkWidget->setLayout(checkLayout);
		layout->addWidget(checkWidget);
	}

	QWidget* buttonWidget = new QWidget(this);
	buttonWidget->setFixedHeight(MESSAGE_BUTTONBOX_HEIGHT);

	QHBoxLayout* buttonLayout = new QHBoxLayout();
	buttonLayout->setContentsMargins(0, 0, 0, 0);
	buttonLayout->setSpacing(10);
	buttonLayout->setAlignment(Qt::AlignCenter);

	if (buttons & QDialogButtonBox::Cancel)
	{
		QPushButton* cancelbutton = new QPushButton(this);
		cancelbutton->setText(QTStr("Cancel"));
		cancelbutton->setFixedSize(100, 40);
		cancelbutton->setProperty("pushButtonTheme", "type4");
		connect(cancelbutton, &QPushButton::clicked, this, &QDialog::reject);
		buttonLayout->addWidget(cancelbutton);
		buttonType = "type2";
	}

	if (buttonType == "")
		buttonType = "type4";

	QPushButton* okButton = new QPushButton(this);

	if (buttonText.isEmpty())
		okButton->setText(QTStr("OK"));
	else
		okButton->setText(buttonText);
	okButton->setFixedSize(100, 40);
	okButton->setDefault(true);
	okButton->setProperty("pushButtonTheme", buttonType);
	connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
	buttonLayout->addWidget(okButton);
	buttonWidget->setLayout(buttonLayout);

	layout->addWidget(buttonWidget);

	//QFont font = textLabel->font();
	//QFontMetrics fm(font);
	//int textWidth = fm.horizontalAdvance(textLabel->text());

	//if (useWordWrap && textWidth > MESSAGE_TEXT_WIDTH) {
	//	textLabel->setWordWrap(true);
	//	int height = (layout->hasHeightForWidth())
	//				 ? layout->totalHeightForWidth(MESSAGE_BOX_WIDTH)
	//				 : layout->totalMinimumSize().height();
	//	height += 50; // layout margin (top, bottom)
	//	height += MESSAGE_BUTTONBOX_HEIGHT;
	//	height += MESSAGE_TITLE_HEIGHT;

	//	setMinimumSize(QSize(MESSAGE_BOX_WIDTH, height));
	//	resize(MESSAGE_BOX_WIDTH, height);
	//}

	setLayout(layout);
}


int AFQMessageBox::ShowMessage(QDialogButtonBox::StandardButtons buttons,
						       QWidget* parent, 
							   const QString& title, 
						       const QString& text, 
							   bool useWordWrap, 
							   bool isNotParent,
							   const QString& topText, int fixedWidth, int fixedHeight, QString buttonType, QString checkBoxText, bool* checkBoxResult)
{
	QWidget* dialogParent = (isNotParent ? nullptr : parent);

	AFQMessageBox mb(buttons, dialogParent, title, text, useWordWrap, "", topText, fixedWidth, fixedHeight, buttonType, checkBoxText);


	if (isNotParent) {
		mb.adjustSize();
		setCenterPositionNotUseParent(&mb, parent);
	}

	AFQBlockManager::ApplyMoveInAllArea(&mb);

	if(MAINFRAME && IsAlwaysOnTop(MAINFRAME))
		SetAlwaysOnTop(&mb, true);

	mb.adjustSize();

	mb.exec();
	if (checkBoxResult)
		*checkBoxResult = mb.IsCheckBoxChecked();

	return mb.result();
}

int AFQMessageBox::ShowMessageWithButtonText(QDialogButtonBox::StandardButtons buttons,
							   QWidget* parent, 
							   const QString& title, 
							   const QString& text, 
							   QString changeButtonText,
							   bool useWordWrap, 
							   bool isNotParent)
{
	QWidget* dialogParent = (isNotParent ? nullptr : parent);

	AFQMessageBox mb(buttons, dialogParent, title, text, useWordWrap, changeButtonText);

	if (isNotParent) {
		mb.adjustSize();
		setCenterPositionNotUseParent(&mb, parent);
	}
	AFQBlockManager::ApplyMoveInAllArea(&mb);
	mb.exec();
	return mb.result();
}

void AFQMessageBox::ShowModalessOnButtonAlert(QWidget* parent, const QString& text, int fixedWidth, int fixedHeight)
{
	AFTTopBaseWidget* alert = new AFTTopBaseWidget(parent);

	alert->SetHeightResizeEnabled(false);
	alert->SetWidthResizeEnabled(false);

	alert->setWindowFlags(Qt::Window | Qt::Tool);
	alert->setAttribute(Qt::WA_DeleteOnClose);
	alert->setFixedSize(fixedWidth, fixedHeight);

	alert->setObjectName("dialog_MessageBox");

	QLabel* info = new QLabel(alert);
	info->setProperty("labelType", "alertText");
	info->setAlignment(Qt::AlignCenter);
	info->setText(text);

	QWidget* infoWidget = new QWidget(alert);
	QHBoxLayout* infoLayout = new QHBoxLayout(infoWidget);
	infoLayout->setSpacing(0);
	infoLayout->setContentsMargins(10, 30, 10, 30);
	infoLayout->addWidget(info);
	infoWidget->setLayout(infoLayout);

	QWidget* buttonWidget = new QWidget(alert);
	buttonWidget->setFixedHeight(60);
	QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
	buttonLayout->setContentsMargins(0, 0, 0, 20);
	buttonLayout->addStretch(1);
	buttonLayout->setSpacing(10);

	QPushButton* button = new QPushButton(buttonWidget);
	button->setText(QTStr("OK"));
	button->setFixedSize(100, 40);
	buttonLayout->addWidget(button);
	button->setProperty("pushButtonTheme", "type2");
	connect(button, &QPushButton::clicked, alert, &AFTTopBaseWidget::close);

	buttonLayout->addStretch(1);
	buttonWidget->setLayout(buttonLayout);

	QVBoxLayout* mainLayout = new QVBoxLayout(alert);
	mainLayout->setSpacing(0);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->addWidget(infoWidget);
	mainLayout->addWidget(buttonWidget);
	alert->setLayout(mainLayout);

	AFQBlockManager::ApplyMoveInAllArea(alert);

	alert->show();
	alert->resize(fixedWidth, fixedHeight);
	alert->raise();
	alert->activateWindow();
}

void AFQMessageBox::ShowModalessOneButtonMessage(QDialogButtonBox::StandardButtons buttons, QWidget* parent, const QString& title, const QString& text, bool useWordWrap, bool isNotParent, const QString& topText, int fixedWidth, int fixedHeight, QString buttonType)
{
	QWidget* dialogParent = (isNotParent ? nullptr : parent);

	AFQMessageBox mb(buttons, dialogParent, title, text, useWordWrap, "", topText, fixedWidth, fixedHeight, buttonType);


	if (isNotParent) {
		mb.adjustSize();
		setCenterPositionNotUseParent(&mb, parent);
	}

	AFQBlockManager::ApplyMoveInAllArea(&mb);

	if (MAINFRAME && IsAlwaysOnTop(MAINFRAME))
		SetAlwaysOnTop(&mb, true);

	mb.adjustSize();

	mb.show();
	mb.raise();
	mb.activateWindow();
}


void AFQMessageBox::showEvent(QShowEvent* event)
{
	int minHeight = minimumSize().height();
	QSize boxSize = QSize(width(), minHeight);
	resize(boxSize);
}
