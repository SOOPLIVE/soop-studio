#include "CMessageBox.h"

#include <QVBoxLayout>
#include <QFont>
#include <QFontMetrics>

#include "qt-wrapper.h"

#define MESSAGE_TITLE_HEIGHT 30
#define MESSAGE_BUTTONBOX_HEIGHT 40
#define MESSAGE_TEXT_WIDTH 370
#define MESSAGE_BOX_WIDTH 410

AFQMessageBox::AFQMessageBox(QDialogButtonBox::StandardButtons buttons, 
							 QWidget *parent, 
							 const QString& title, 
							 const QString& text, 
							 bool useWordWrap,
							 const QString& buttonText)
							 : AFQRoundedDialogBase{parent}
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	setMinimumWidth(MESSAGE_BOX_WIDTH);
	setMinimumHeight(200);
	setObjectName("dialog_MessageBox");

	QVBoxLayout* layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);

	QFrame* titleFrame = new QFrame;
	titleFrame->setFixedHeight(MESSAGE_TITLE_HEIGHT);

	QHBoxLayout* titleLayout = new QHBoxLayout;
	titleLayout->setContentsMargins(18, 6, 8, 6);
	titleFrame->setLayout(titleLayout);

	/*QLabel* LabelTitle = new QLabel(this);
	LabelTitle->setObjectName("label_MessageBoxTitle");
	LabelTitle->setText(title);
	LabelTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);*/


	/*QPushButton* closeButton = new QPushButton(this);
	closeButton->setFixedSize(24, 24);
	closeButton->setObjectName("pushButton_CloseMessageBox");*/

	//titleLayout->addWidget(LabelTitle);
	//titleLayout->addWidget(closeButton);

	layout->addWidget(titleFrame);


	QVBoxLayout* textLayout = new QVBoxLayout;
	textLayout->setContentsMargins(20, 10, 20, 10);

	QLabel* textLabel = new QLabel(this);
	textLabel->setObjectName("label_MessageBoxText");
	textLabel->setText(text);
	textLabel->setAlignment(Qt::AlignCenter);

	textLayout->addWidget(textLabel, 0, Qt::AlignCenter);

	layout->addLayout(textLayout);
	layout->addSpacerItem(new QSpacerItem(0, 10, QSizePolicy::Expanding, QSizePolicy::Fixed));

	QDialogButtonBox* buttonbox = new QDialogButtonBox(buttons);
	buttonbox->setCenterButtons(true);

	if (buttonbox->standardButtons() & QDialogButtonBox::Ok)
	{
		buttonbox->button(QDialogButtonBox::Ok)->setFixedSize(128, MESSAGE_BUTTONBOX_HEIGHT);
		if (buttonText != "")
			buttonbox->button(QDialogButtonBox::Ok)->setText(buttonText);
		connect(buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	}

	if (buttonbox->standardButtons() & QDialogButtonBox::Cancel)
	{
		buttonbox->button(QDialogButtonBox::Cancel)->setFixedSize(128, MESSAGE_BUTTONBOX_HEIGHT);
		connect(buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	}

	//connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

	layout->addWidget(buttonbox);
	layout->addSpacerItem(new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Fixed));

	QFont font = textLabel->font();
	QFontMetrics fm(font);
	int textWidth = fm.horizontalAdvance(textLabel->text());

	if (useWordWrap && textWidth > MESSAGE_TEXT_WIDTH) {
		textLabel->setWordWrap(true);
		int height = (layout->hasHeightForWidth())
					 ? layout->totalHeightForWidth(MESSAGE_BOX_WIDTH)
					 : layout->totalMinimumSize().height();
		height += 50; // layout margin (top, bottom)
		height += MESSAGE_BUTTONBOX_HEIGHT;
		height += MESSAGE_TITLE_HEIGHT;

		setMinimumSize(QSize(MESSAGE_BOX_WIDTH, height));
		resize(MESSAGE_BOX_WIDTH, height);
	}

	setLayout(layout);
}


int AFQMessageBox::ShowMessage(QDialogButtonBox::StandardButtons buttons,
						       QWidget* parent, 
							   const QString& title, 
						       const QString& text, 
							   bool useWordWrap, 
							   bool isNotParent)
{
	QWidget* dialogParent = (isNotParent ? nullptr : parent);

	AFQMessageBox mb(buttons, dialogParent, title, text, useWordWrap);

	if (isNotParent) {
		mb.adjustSize();
		setCenterPositionNotUseParent(&mb, parent);
	}

	mb.exec();
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

	mb.exec();
	return mb.result();
}
