#include "CSavvyWidget.h"
#include "ui_savvy-widget.h"

#include <QSvgWidget>
#include <QFileDialog>
#include <curl/curl.h>
#include <iostream>
#include <fstream>

#include "platform/platform.hpp"
#include "MainFrame/SceneSource/CMainSceneSource.h"

AFQSavvyWidget::AFQSavvyWidget(QWidget *parent) :
	AFTTopBaseDialog(parent),
    ui(new Ui::AFQSavvyWidget)
{
    ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);

	setWindowTitle("SAVYG");

	SetWidthResizeEnabled(false);
	SetHeightResizeEnabled(false);

	AFQBlockManager::ApplyMoveInAllArea(this);

	_InitSelectSavvy();
}

AFQSavvyWidget::~AFQSavvyWidget()
{
    delete ui;
}

void AFQSavvyWidget::qslotStartSignature()
{
	emit qsignalStartSignatureTriggered(true);
	close();
}

void AFQSavvyWidget::qslotStartReaction()
{
	emit qsignalStartReactionTriggered();
	close();
}

void AFQSavvyWidget::closeEvent(QCloseEvent* event)
{
	emit qsignalSavvyClosedTriggered(false);
}

void AFQSavvyWidget::_InitSelectSavvy()
{
	std::string absPath;
	bool foundIcon = GetDataFilePath("assets/savvy/signature.png", absPath);
	QPixmap signature(QString::fromStdString(absPath));

	ui->label_SignaturePicture->setPixmap(signature);

	foundIcon = GetDataFilePath("assets/savvy/reaction.png", absPath);
	QPixmap reaction(QString::fromStdString(absPath));
	ui->label_ReactionPicture->setPixmap(reaction);

	setFixedSize(600, 398);
	connect(ui->widget_Reaction, &AFQHoverWidget::qsignalMouseClick,
		this, &AFQSavvyWidget::qslotStartReaction);

	connect(ui->widget_Signature, &AFQHoverWidget::qsignalMouseClick,
		this, &AFQSavvyWidget::qslotStartSignature);

	ui->pushButton_CloseButton->setProperty("buttonType", "closeButton");
	connect(ui->pushButton_CloseButton, &QPushButton::clicked,
		this, &AFQSavvyWidget::close);
}

