#include "CAutoConfigWizard.h"
#include "CAutoConfigStartPage.h"
#include "CAutoConfigVideoPage.h"

#define wiz reinterpret_cast<AFQAutoConfigWizard *>(wizard())

AFQAutoConfigWizard::AFQAutoConfigWizard(QWidget *parent)
    : QWizard{parent}
{
	//EnableThreadedMessageBoxes(true);

	/*calldata_t cd = { 0 };
	calldata_set_int(&cd, "seconds", 5);

	proc_handler_t* ph = obs_get_proc_handler();
	proc_handler_call(ph, "twitch_ingests_refresh", &cd);
	calldata_free(&cd);

	OBSBasic* main = reinterpret_cast<OBSBasic*>(parent);
	main->EnableOutputs(false);

	installEventFilter(CreateShortcutFilter());

	std::string serviceType;
	GetServiceInfo(serviceType, serviceName, server, key);*/
#if defined(_WIN32) || defined(__APPLE__)
	setWizardStyle(QWizard::ModernStyle);
#endif
	//streamPage = new AutoConfigStreamPage();

	setPage(StartPage, new AFQAutoConfigStartPage(this));
	setPage(VideoPage, new AFQAutoConfigVideoPage(this));
	//setPage(StreamPage, streamPage);
	//setPage(TestPage, new AutoConfigTestPage());
	/*setWindowTitle(QTStr("Basic.AutoConfig"));
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	baseResolutionCX = ovi.base_width;
	baseResolutionCY = ovi.base_height;*/

	/* ----------------------------------------- */
	/* check to see if Twitch's "auto" available */

	/*OBSDataAutoRelease twitchSettings = obs_data_create();

	obs_data_set_string(twitchSettings, "service", "Twitch");

	obs_properties_t* props = obs_get_service_properties("rtmp_common");
	obs_properties_apply_settings(props, twitchSettings);

	obs_property_t* p = obs_properties_get(props, "server");
	const char* first = obs_property_list_item_string(p, 0);
	twitchAuto = strcmp(first, "auto") == 0;

	obs_properties_destroy(props);*/

	/* ----------------------------------------- */
	/* load service/servers                      */

	/*customServer = serviceType == "rtmp_custom";

	QComboBox* serviceList = streamPage->ui->service;

	if (!serviceName.empty()) {
		serviceList->blockSignals(true);

		int count = serviceList->count();
		bool found = false;

		for (int i = 0; i < count; i++) {
			QString name = serviceList->itemText(i);

			if (name == serviceName.c_str()) {
				serviceList->setCurrentIndex(i);
				found = true;
				break;
			}
		}

		if (!found) {
			serviceList->insertItem(0, serviceName.c_str());
			serviceList->setCurrentIndex(0);
		}

		serviceList->blockSignals(false);
	}*/

	/*streamPage->UpdateServerList();
	streamPage->UpdateKeyLink();
	streamPage->UpdateMoreInfoLink();
	streamPage->lastService.clear();*/

	//if (!customServer) {
	//	QComboBox* serverList = streamPage->ui->server;
	//	int idx = serverList->findData(QString(server.c_str()));
	//	if (idx == -1)
	//		idx = 0;

	//	serverList->setCurrentIndex(idx);
	//}
	//else {
	//	streamPage->ui->customServer->setText(server.c_str());
	//	int idx = streamPage->ui->service->findData(
	//		QVariant((int)ListOpt::Custom));
	//	streamPage->ui->service->setCurrentIndex(idx);
	//}

	//if (!key.empty())
	//	streamPage->ui->key->setText(key.c_str());

	//int bitrate =
	//	config_get_int(main->Config(), "SimpleOutput", "VBitrate");
	//streamPage->ui->bitrate->setValue(bitrate);
	//streamPage->ServiceChanged();

	//TestHardwareEncoding();
	//if (!hardwareEncodingAvailable) {
	//	delete streamPage->ui->preferHardware;
	//	streamPage->ui->preferHardware = nullptr;
	//}
	//else {
	//	/* Newer generations of NVENC have a high enough quality to
	//	 * bitrate ratio that if NVENC is available, it makes sense to
	//	 * just always prefer hardware encoding by default */
	//	bool preferHardware = nvencAvailable || appleAvailable ||
	//		os_get_physical_cores() <= 4;
	//	streamPage->ui->preferHardware->setChecked(preferHardware);
	//}

	setOptions(QWizard::WizardOptions());
	setButtonText(QWizard::FinishButton,
		"Basic.AutoConfig.ApplySettings");
	setButtonText(QWizard::BackButton, "Back");
	setButtonText(QWizard::NextButton, "Next");
	setButtonText(QWizard::CancelButton, "Cancel");
}
