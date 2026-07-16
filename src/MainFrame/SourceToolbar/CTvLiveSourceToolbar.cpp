#include "CTvLiveSourceToolbar.h"
#include "ui_tvlive-source-toolbar.h"

#include <QPushButton>

#include "MainFrame/CMainFrame.h"
#include "MainFrame/Output/COutput.h"

#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/SOOPSource/CSoopMediaSourceManager.h"
#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Auth/SBroadInfo.h"

AFQTvLiveSourceToolbar::AFQTvLiveSourceToolbar(QWidget* parent, OBSSource source) :
	CBaseSourceToolbar(parent, source),
	ui(new Ui::AFQTvLiveSourceToolbar)
{
	ui->setupUi(this);

	connect(ui->pushButton_Refresh, &QPushButton::clicked,
		this, &AFQTvLiveSourceToolbar::_qslotRefreshButtonClicked);
}

AFQTvLiveSourceToolbar::~AFQTvLiveSourceToolbar()
{
	delete ui;
}

void AFQTvLiveSourceToolbar::SetContextBarSize(int contextBarSize)
{
	// add button text space
	if (2 == contextBarSize)
		ui->pushButton_Refresh->setText(" " + QTStr("Toolbar.RefreshMedia"));
	else
		ui->pushButton_Refresh->setText("");
}

void AFQTvLiveSourceToolbar::_qslotRefreshButtonClicked()
{
	QString id = obs_source_get_id(GetSource());
	if (0 != id.compare("soop_tv_cable_source"))
		return;

	int cpNo = SOOP_SRC_MANAGER.GetTvLiveCPNo();
	const int categoryNo = 390000 + cpNo;
	if (0 != cpNo) {
		if (AFOutputUtil::IsStreamActive())
		{
			if (!MAIN_OUTPUT->IsStreamingOnlySoop()) {
				QString exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage2")
					.arg(obs_source_get_display_name(id.toStdString().c_str()));
				AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this, "", exceptionMsg);
				return;
			}

			AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
			if (!broadInfo) {
				return;
			}

			if (broadInfo->CategoryNumber() != categoryNo)
			{
				std::list<int> categorys;
				categorys.push_back(categoryNo);

				AFQCateChangeDialog dlg(MAINFRAME, id.toStdString().c_str());
				dlg.AddAllowedCategoryInfo(categorys);

				if (QDialog::Accepted != dlg.exec())
					return;

				int selectedCategoryNum = dlg.GetSelectedCategory();
				broadInfo->SetCategory(selectedCategoryNum);
				AUTH_CONTEXT.SendSoopBroadInfoSetting();
				MAINFRAME->RefreshBroadInfoDockUI(false);
			}

		}
		SOOP_SRC_MANAGER.RequestTvLiveOneTimeUrlWithGeoBlock(cpNo);
	}
}