#include "CAutoConfigStartPage.h"
#include "ui_auto-config-start-page.h"

#include "CAutoConfigWizard.h"
#include "include/qt-wrapper.h"

AFQAutoConfigStartPage::AFQAutoConfigStartPage(AFQAutoConfigWizard *parent) :
	QWizardPage(parent),
    ui(new Ui::AFQAutoConfigStartPage)
{
    ui->setupUi(this);
    m_qWiz = parent;

	setTitle(QT_UTF8("Basic.AutoConfig.StartPage"));
	setSubTitle(QT_UTF8("Basic.AutoConfig.StartPage.SubTitle"));
}

AFQAutoConfigStartPage::~AFQAutoConfigStartPage()
{
    delete ui;
}

int AFQAutoConfigStartPage::nextId() const
{
	return m_qWiz->type == AFQAutoConfigWizard::Type::VirtualCam
		? AFQAutoConfigWizard::TestPage
		: AFQAutoConfigWizard::VideoPage;
}

void AFQAutoConfigStartPage::on_prioritizeStreaming_clicked()
{
    m_qWiz->type = AFQAutoConfigWizard::Type::Streaming;
}

void AFQAutoConfigStartPage::on_prioritizeRecording_clicked()
{
    m_qWiz->type = AFQAutoConfigWizard::Type::Recording;
}

void AFQAutoConfigStartPage::PrioritizeVCam()
{
	m_qWiz->type = AFQAutoConfigWizard::Type::VirtualCam;
}
