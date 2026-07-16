#include "CSignaturePopup.h"
#include "ui_signature-popup.h"

#include <QSvgWidget>
#include <QFileDialog>
#include <curl/curl.h>
#include <iostream>
#include <fstream>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"
#include "display-helpers.hpp"

#include "MainFrame/SceneSource/CMainSceneSource.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Auth/CAuthManager.h"

#include "UIComponent/CMessageAlert.h"

static QVariant propertyListToQVariant(obs_property_t* prop, size_t idx)
{
	obs_combo_format format = obs_property_list_format(prop);

	QVariant var;
	if (format == OBS_COMBO_FORMAT_INT) {
		long long val = obs_property_list_item_int(prop, idx);
		var = QVariant::fromValue<long long>(val);
	}
	else if (format == OBS_COMBO_FORMAT_FLOAT) {
		double val = obs_property_list_item_float(prop, idx);
		var = QVariant::fromValue<double>(val);
	}
	else if (format == OBS_COMBO_FORMAT_STRING) {
		var = QByteArray(obs_property_list_item_string(prop, idx));
	}
	else if (format == OBS_COMBO_FORMAT_BOOL) {
		bool val = obs_property_list_item_bool(prop, idx);
		var = QVariant::fromValue<bool>(val);
	}
	return var;
}

static size_t _string_write(char* ptr, size_t size, size_t nmemb, std::string& str)
{
	size_t total = size * nmemb;
	if (total)
		str.append(ptr, total);

	return total;
}

size_t WriteFileCallback(void* contents, size_t size, size_t nmemb, void* userp) {
	QFile* file = static_cast<QFile*>(userp);
	file->write(static_cast<char*>(contents), size * nmemb);
	return size * nmemb;
}

AFQSavvyCamWidget::AFQSavvyCamWidget(QWidget* parent) : AFQTDisplay(parent)
{
	setMouseTracking(true);
}

void AFQSavvyCamWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		QPoint point = event->pos();
		if (point.x() >= 262 && point.x() <= 310 && point.y() >= 122 && point.y() <= 170) {
			emit qsignalMouseOnButton(2);
			emit qsignalLeftMousePressed();
		}
	}
	AFQTDisplay::mousePressEvent(event);
}

void AFQSavvyCamWidget::mouseMoveEvent(QMouseEvent* event)
{
	QPoint point = event->pos();
	if (point.x() >= 262 && point.x() <= 310 && point.y() >= 122 && point.y() <= 170)
		m_mouseHoverOnButton = true;
	else
		m_mouseHoverOnButton = false;

	emit qsignalMouseOnButton(m_mouseHoverOnButton);
}

AFQDragDropImageWidget::AFQDragDropImageWidget(QWidget* parent) : QWidget(parent)
{
	setAcceptDrops(true);
}

void AFQDragDropImageWidget::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasUrls())
	{
		event->acceptProposedAction();
	}
}

void AFQDragDropImageWidget::dropEvent(QDropEvent* event)
{
	QList<QUrl> urls = event->mimeData()->urls();
	if (urls.isEmpty())
		return;

	QString filePath = urls.first().toLocalFile();
	qDebug() << "파일 드롭됨: " << filePath;

	qsignalFileUploaded(filePath);
}

AFQSignaturePopup::AFQSignaturePopup(bool reactionAble, QWidget *parent) :
	AFTTopBaseDialog(parent),
    ui(new Ui::AFQSignaturePopup)
{
    ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);
	m_reactionAble = reactionAble;

	setWindowTitle("SAVVY");

	SetWidthResizeEnabled(false);
	SetHeightResizeEnabled(false);

	AFQBlockManager::ApplyMoveInAllArea(this);

	_InitSignatureWidget();
}

AFQSignaturePopup::~AFQSignaturePopup()
{
    delete ui;
}

void AFQSignaturePopup::qslotStartReaction()
{
}

void AFQSignaturePopup::qslotStartSignature()
{
}

void AFQSignaturePopup::qslotCreateSignatureFromUrlTriggered()
{
	QThread* thread = new QThread;

	const char* slot_ = "qslotReceiveSignatureJob";
	QObject* receiver_ = this;

	AFChannelData* pSoopChannel = nullptr;
	AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, pSoopChannel);
	std::string id_ = pSoopChannel->pAuthData->channelID;

	connect(thread, &QThread::started, [this, id_, slot_, thread]()
		{
			CURL* curl;
			CURLcode res;
			curl = curl_easy_init();
			if (curl) {

				std::string url = QString::fromUtf8(URL_SAVVY_SIGNATURE_IMAGE).toStdString();
				curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				//curl_easy_setopt(curl, CURLOPT_PROXY, "");
				curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
				curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
				curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "UTF-8");
				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, "Content-Type: multipart/form-data; boundary=----Boundary1234567890; charset=UTF-8");
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

				std::string responseString;

				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _string_write);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

				curl_mime* mime;
				curl_mimepart* part;
				mime = curl_mime_init(curl);
				part = curl_mime_addpart(mime);
				curl_mime_name(part, "file");
				curl_mime_filedata(part, m_imagePath.toUtf8().constData());
				part = curl_mime_addpart(mime);
				curl_mime_name(part, "streamer_id");
				curl_mime_data(part, id_.c_str(), CURL_ZERO_TERMINATED);
				part = curl_mime_addpart(mime);
				curl_mime_name(part, "text");
				curl_mime_data(part, m_message.c_str(), CURL_ZERO_TERMINATED);
				part = curl_mime_addpart(mime);
				curl_mime_name(part, "number");
				curl_mime_data(part, m_balloonCount.c_str(), CURL_ZERO_TERMINATED);
				part = curl_mime_addpart(mime);
				curl_mime_name(part, "bg_index");
				curl_mime_data(part, m_selectedBackground.c_str(), CURL_ZERO_TERMINATED);
				part = curl_mime_addpart(mime);
				curl_mime_name(part, "style_index");
				curl_mime_data(part, m_selectedStyle.c_str(), CURL_ZERO_TERMINATED);
				curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
				res = curl_easy_perform(curl);

				if (res != CURLE_OK) {
					std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
				}
				else {
					QByteArray responseData = QByteArray::fromStdString(responseString);

					QMetaObject::invokeMethod(this, slot_,
						Qt::QueuedConnection,
						Q_ARG(QByteArray, responseData));
				}

				curl_mime_free(mime);
				curl_slist_free_all(headers);
			}
			curl_easy_cleanup(curl);
			thread->quit();
		});

	connect(thread, &QThread::finished, thread, &QThread::deleteLater);

	thread->start();
}

void AFQSignaturePopup::qslotReceiveSignatureJob(const QByteArray& responseData)
{
	bool success = false;

	std::string jsonString = responseData.toStdString();
	std::string err;
	err.clear();

	emit qsignalSignatureJobComplete(success);

}

void AFQSignaturePopup::qslotStartSignatureStatusCheck(bool start)
{
	if (start)
		QTimer::singleShot(SAVVY_CHECK_STATUS_TIME, this, &AFQSignaturePopup::qslotCheckSignatureStatus);
}

void AFQSignaturePopup::qslotCheckSignatureStatus()
{
	QThread* thread = new QThread;

	const char* slot_ = "qslotReceiveSignatureStatus";
	QObject* receiver_ = this;

	AFChannelData* pSoopChannel = nullptr;
	AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, pSoopChannel);
	std::string id_ = pSoopChannel->pAuthData->channelID;
	std::string url_ = SOOP_SIGNATURE_STATUS_URL + m_jobID;

	connect(thread, &QThread::started, [this, url_, slot_, thread]()
		{
			CURL* curl;
			CURLcode res;
			std::string responseString;

			curl = curl_easy_init();
			if (curl) {
				curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());
				//curl_easy_setopt(curl, CURLOPT_PROXY, "");
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _string_write);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

				curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
				curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

				res = curl_easy_perform(curl);

				if (res != CURLE_OK) {
				}
				else {
					QByteArray responseData = QByteArray::fromStdString(responseString);

					QMetaObject::invokeMethod(this, slot_,
						Qt::QueuedConnection,
						Q_ARG(QByteArray, responseData));
				}
			}
			curl_easy_cleanup(curl);
			thread->quit();
		});

	connect(thread, &QThread::finished, thread, &QThread::deleteLater);

	thread->start();
}

void AFQSignaturePopup::qslotReceiveSignatureStatus(const QByteArray& responseData)
{
	std::string jsonString = responseData.toStdString();
	std::string err;
	err.clear();
}


void AFQSignaturePopup::qslotStartSignatureTriggered()
{
	ui->stackedWidget_Signature->setCurrentWidget(ui->page_SignatureUpload);
	ui->stackedWidget_UploadImage->setCurrentWidget(ui->page_SignatureImageStart);
	ui->stackedWidget_SignatureCam->setCurrentWidget(ui->page_SignatureCamDisplay);

	_ChangeCamUploadedButtonStyle(false);
	_ChangeImageUploadedButtonStyle(false);

	config_set_bool(APPCONFIG, "General", "SkipSavvyStartPage", ui->checkBox_SkipStart->isChecked());
	config_save_safe(APPCONFIG, "tmp", nullptr);
}

void AFQSignaturePopup::qslotUploadImageLoaded(bool success)
{
	ui->pushButton_CloseButton->setEnabled(true);
	_ChangeImageUploadedButtonStyle(success);

	if (success)
		ui->stackedWidget_UploadImage->setCurrentWidget(ui->page_SignatureImageFinish);
	else
		ui->stackedWidget_UploadImage->setCurrentWidget(ui->page_SignatureImageFailed);
}

void AFQSignaturePopup::qslotUploadImageTriggered()
{
	_ChangeImageUploadedButtonStyle(false);
	ui->pushButton_CloseButton->setEnabled(false);

	QString strFileName = QFileDialog::getOpenFileName(this, "", QDir::homePath(), "Images (*.jpg *.jpeg *.png);");

	if (strFileName.isEmpty()) {
		ui->pushButton_CloseButton->setEnabled(true);
		return;
	}

	qslotShowUploadedImage(strFileName);
}

void AFQSignaturePopup::qslotUploadCreateTriggered()
{
	ui->lineEdit_SignatureBalloonCount->setText("1");
	ui->lineEdit_SignatureMessage->setText("");

	m_backGroundButtons[0]->TriggerClick();
	m_styleButtons[0]->TriggerClick();

	ui->stackedWidget_Signature->setCurrentWidget(ui->page_SignatureData);
}

void AFQSignaturePopup::qslotShowUploadedImage(const QString& filePath)
{
	bool success = false;

	if (isValidImageFile(filePath))
	{
		_StartLoadingMovie(false);
		ui->stackedWidget_UploadImage->setCurrentWidget(ui->page_SignatureImageUpload);

		QFileInfo fileInfo(filePath);
		QString extension = fileInfo.suffix().toLower();


		if (CONFIG_CONTEXT.CheckSavvyTempFolder())
		{
			QString savvyFile = m_savvyFolderPath + "/savvy." + extension;

			bool fileRemoved = true;
			if (QFile::exists(savvyFile))
			{
				if (!QFile::remove(savvyFile))
				{
					fileRemoved = false;
				}
			}

			if (fileRemoved)
			{
				if (QFile::copy(filePath, savvyFile))
				{
					QPixmap pixmap(savvyFile);
					ui->label_SignatureUploadImage->setPixmap(pixmap.scaled(ui->label_SignatureUploadImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
					m_imagePath = savvyFile;
					success = true;
				}
			}
		}


		QTimer::singleShot(SAVVY_UPLOAD_TIME, this, [=] {
			emit qsignalSignatureUpload(success);
			});
	}
	else
	{
		ui->pushButton_CloseButton->setEnabled(true);
		ui->stackedWidget_UploadImage->setCurrentWidget(ui->page_SignatureImageFailed);
	}
}

void AFQSignaturePopup::qslotSignatureStyleSelectTriggered()
{
	AFQSavvyStyleButton* styleButton = reinterpret_cast<AFQSavvyStyleButton*>(sender());
	m_selectedStyle = styleButton->StyleNum();

	foreach(AFQSavvyStyleButton * button, m_styleButtons)
	{
		if (button == styleButton)
			button->CheckStyle(true);
		else
			button->CheckStyle(false);
	}
}

void AFQSignaturePopup::qslotSignatureBackgroundSelectTriggered()
{
	AFQSavvyStyleButton* backgroundButton = reinterpret_cast<AFQSavvyStyleButton*>(sender());
	m_selectedBackground = backgroundButton->StyleNum();

	foreach(AFQSavvyStyleButton * button, m_backGroundButtons)
	{
		if (button == backgroundButton)
			button->CheckStyle(true);
		else
			button->CheckStyle(false);
	}
}

void AFQSignaturePopup::qslotSignatureBalloonEditing(const QString& text)
{
	QLineEdit* senderWidget = qobject_cast<QLineEdit*>(sender());
	bool ok;
	int count = text.toInt(&ok);
	if (ok)
	{
		if (count > SAVVY_MAX_BALLOON_COUNT)
		{
			senderWidget->setText(QString::number(SAVVY_MAX_BALLOON_COUNT));
			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
				"", QTStr("Savvy.Signature.BalloonLimit"), false, true, "", 0, 0, "type1");
		}
	}
	else
	{
		senderWidget->setText("");
		senderWidget->setModified(false);
	}
}

void AFQSignaturePopup::qslotSignatureCreateTriggered()
{
	QString text = ui->lineEdit_SignatureBalloonCount->text();
	bool ok;
	int count = text.toInt(&ok);
	if (ok)
	{
		if (count > SAVVY_MAX_BALLOON_COUNT)
		{
			ui->lineEdit_SignatureBalloonCount->setText(QString::number(SAVVY_MAX_BALLOON_COUNT));
			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
				"", QTStr("Savvy.Signature.BalloonLimit"), false, true, "", 0, 0, "type1");
			return;
		}
		else if (count == 0)
		{
			ui->lineEdit_SignatureBalloonCount->setText(QString::number(1));
			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
				"", QTStr("Savvy.Signature.BalloonZero"), false, true, "", 0, 0, "type1");
			return;
		}
	}
	else
	{
		ui->lineEdit_SignatureBalloonCount->setText("");
		return;
	}


	ui->pushButton_CloseButton->setEnabled(false);
	int BalloonCount = ui->lineEdit_SignatureBalloonCount->text().toInt();
	m_balloonCount = std::to_string(BalloonCount);
	m_message = ui->lineEdit_SignatureMessage->text().toStdString();

	if (m_pLoadingMovie)
		_StartLoadingMovie(true);

	ui->stackedWidget_Signature->setCurrentWidget(ui->page_SignatureProcess);

	qslotCreateSignatureFromUrlTriggered();
}

void AFQSignaturePopup::qslotSignatureCompletedTriggered()
{
	ui->pushButton_CloseButton->setEnabled(true);
	if (m_pLoadingMovie)
		_StopLoadingMovie();
	_SetDownloadedImages();
	ui->stackedWidget_Signature->setCurrentWidget(ui->page_SignatureComplete);

	m_isSignature = false;
}

void AFQSignaturePopup::qslotSignatureCamRecapture()
{
	obs_display_add_draw_callback(ui->widget_SignatureCam->GetDisplay(),
		AFQSignaturePopup::_DrawPreview,
		this);

	qslotChangeCamImage(0);
	_ChangeCamUploadedButtonStyle(false);
	ui->stackedWidget_SignatureCam->setCurrentWidget(ui->page_SignatureCamDisplay);
}

void AFQSignaturePopup::qslotSignatureRestartTriggered()
{
	ui->stackedWidget_Signature->setCurrentWidget(ui->page_SignatureImageUpload);
}

void AFQSignaturePopup::qslotSignatureNavigateUploadUrl()
{
	MAINFRAME->NavigateDefaultBrowser(QString::fromStdString(SIGNATURE_UPLOAD_PAGE));
}

void AFQSignaturePopup::qslotVideoDeviceListChanged(int changedindex)
{
	UNUSED_PARAMETER(changedindex);
	if (!m_obsInputSource) {
		return;
	}

	QVariant data;

    const char* propId;
#ifdef _WIN32
    propId = "video_device_id";
#elif defined(__APPLE__)
    propId = "device";
#endif
    
	std::unique_ptr<obs_properties_t, properties_delete_t> props =
		properties_t(obs_source_properties(m_obsInputSource), obs_properties_destroy);
	obs_property_t* prop = obs_properties_get(props.get(), propId);

	int index = ui->comboBox_SignatureCamList->currentIndex();
	if (index != -1)
		data = ui->comboBox_SignatureCamList->itemData(index);

	OBSDataAutoRelease settings = obs_source_get_settings(m_obsInputSource);

	obs_data_set_string(settings, propId, data.toByteArray().constData());

	obs_source_update(m_obsInputSource, settings);
	obs_property_modified(prop, settings);

	m_curData = data;
}

void AFQSignaturePopup::qslotScreenShot()
{
	if (CONFIG_CONTEXT.CheckSavvyTempFolder())
	{
		MAIN_SCENESOURCE->ScreenshotSource(m_obsInputSource, true);
		connect(MAIN_SCENESOURCE, &CMainSceneSource::qsignalScreenShotFinished, this, &AFQSignaturePopup::qslotSetScreenShotImage);
	}
}

void AFQSignaturePopup::qslotSetScreenShotImage()
{
	obs_display_remove_draw_callback(ui->widget_SignatureCam->GetDisplay(),
		AFQSignaturePopup::_DrawPreview,
		this);

	disconnect(MAIN_SCENESOURCE, &CMainSceneSource::qsignalScreenShotFinished, this, &AFQSignaturePopup::qslotSetScreenShotImage);

	_ChangeCamUploadedButtonStyle(false);

	CONFIG_CONTEXT.CheckSavvyTempFolder();
	QString savvyFile = m_savvyFolderPath + "/savvy.png";

	QPixmap camScreenshot(savvyFile);
	ui->label_SignatureCamScreenShot->setPixmap(camScreenshot.scaled(ui->label_SignatureCamScreenShot->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
	ui->stackedWidget_SignatureCam->setCurrentWidget(ui->page_SignatureCamScreenShot);
	_ChangeCamUploadedButtonStyle(true);
	m_imagePath = savvyFile;
}

void AFQSignaturePopup::qslotAddDrawCallback()
{
	obs_display_add_draw_callback(ui->widget_SignatureCam->GetDisplay(),
		AFQSignaturePopup::_DrawPreview,
		this);
}

void AFQSignaturePopup::_DrawPreview(void* data, uint32_t cx, uint32_t cy)
{
	AFQSignaturePopup* window = static_cast<AFQSignaturePopup*>(data);

	if (!window->m_obsInputSource)
		return;

	uint32_t sourceCX = std::max(obs_source_get_width(window->m_obsInputSource), 1u);
	uint32_t sourceCY = std::max(obs_source_get_height(window->m_obsInputSource), 1u);

	if (sourceCX == 1 && sourceCY == 1) {

		QMetaObject::invokeMethod(window, "qslotCheckCurrentDeviceActive", Qt::QueuedConnection, Q_ARG(bool, true));

		if (2 != window->ui->stackedWidget_SignatureCam->currentIndex())
			QMetaObject::invokeMethod(window, "qslotChangeSignatureCamPage", Qt::QueuedConnection, Q_ARG(int, 2));
		return;
	} else {

		QMetaObject::invokeMethod(window, "qslotCheckCurrentDeviceActive", Qt::QueuedConnection, Q_ARG(bool, false));

		if (0 != window->ui->stackedWidget_SignatureCam->currentIndex())
			QMetaObject::invokeMethod(window, "qslotChangeSignatureCamPage", Qt::QueuedConnection, Q_ARG(int, 0));
	}

	int x, y;
	int newCX, newCY;
	float scale;

	GetScaleAndCenterPos(sourceCX, sourceCY, cx, cy, x, y, scale);
	newCX = int(scale * float(sourceCX));
	newCY = int(scale * float(sourceCY));
	gs_viewport_push();
	gs_projection_push();
	const bool previous = gs_set_linear_srgb(true);

	gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY), -100.0f, 100.0f);
	gs_set_viewport(x, y, newCX, newCY);
	obs_source_video_render(window->m_obsInputSource);

	gs_set_linear_srgb(previous);
	gs_projection_pop();
	gs_viewport_pop();

	gs_viewport_push();
	gs_projection_push();

	int imageX = 262 * cx/320;
	int imageY = 122 * cy/180;
	int imageWidth = 48 * cx / 320;
	int imageHeight = 48 * cy / 180;
	sourceCX = 48;
	sourceCY = 48;

	GetScaleAndCenterPos(sourceCX, sourceCY, 48, 48, x, y, scale);

	newCX = int(scale * float(sourceCX));
	newCY = int(scale * float(sourceCY));

	//262, 122
	gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY), -100.0f, 100.0f);
	gs_set_viewport(imageX, imageY, imageWidth, imageHeight);
	obs_source_video_render(window->m_obsImageSource);

	gs_set_linear_srgb(previous);
	gs_projection_pop();
	gs_viewport_pop();
}


void AFQSignaturePopup::qslotChangeCamImage(int mouseStatus)
{
	std::string absPath;

	bool foundIcon = false;

	if (mouseStatus == 1)
		foundIcon = GetDataFilePath("assets/savvy/camera_hover.png", absPath);
	else if (mouseStatus == 2)
		foundIcon = GetDataFilePath("assets/savvy/camera_pressed.png", absPath);
	else
		foundIcon = GetDataFilePath("assets/savvy/camera.png", absPath);

	OBSDataAutoRelease settings = obs_source_get_settings(m_obsImageSource);
	const char* file = obs_data_get_string(settings, "file");
	if (strcmp(file, absPath.c_str()) != 0)
	{
		obs_data_set_string(settings, "file", absPath.c_str());

		obs_source_update(m_obsImageSource, settings);
	}
}

void AFQSignaturePopup::qslotChangeSignatureCamPage(int index)
{
	if (index == ui->stackedWidget_SignatureCam->currentIndex())
		return;
	ui->stackedWidget_SignatureCam->setCurrentIndex(index);
}


void AFQSignaturePopup::qslotCheckCurrentDeviceActive(bool start)
{
	if (start) {
		if(!m_timerCurrentDeivceActive.isActive())
			m_timerCurrentDeivceActive.start(1000);
	}
	else {
		if (m_timerCurrentDeivceActive.isActive())
			m_timerCurrentDeivceActive.stop();
	}
}

void AFQSignaturePopup::qslotTimerCheckCurrentDeviceActive()
{
	const char* propId;
#ifdef _WIN32
	propId = "video_device_id";
#elif defined(__APPLE__)
	propId = "device";
#endif

	OBSDataAutoRelease settings = obs_source_get_settings(m_obsInputSource);

	obs_data_set_string(settings, propId, m_curData.toByteArray().constData());

	obs_source_update(m_obsInputSource, settings);
}

void AFQSignaturePopup::closeEvent(QCloseEvent* event)
{
	if (m_isSignature)
	{
		int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
			this,
			"",
			QTStr("Savvy.Signature.Create.CancelInfo"),
			true, true, QTStr("Savvy.Signature.Create.Cancel"));

		if (QDialog::Accepted == result)
		{
			if (!m_reactionAble)
				emit qsignalCloseSignature(false);

			obs_display_remove_draw_callback(ui->widget_SignatureCam->GetDisplay(),
				AFQSignaturePopup::_DrawPreview,
				this);

			obs_source_dec_showing(m_obsInputSource);
			obs_source_remove(m_obsInputSource);
			obs_source_release(m_obsInputSource);
			m_obsInputSource = nullptr;

			qDebug() << "input source destroyed";

			obs_source_dec_showing(m_obsImageSource);
			obs_source_remove(m_obsImageSource);
			obs_source_release(m_obsImageSource);
			m_obsImageSource = nullptr;

			qDebug() << "image source destroyed";

			QDir dir(m_savvyFolderPath);
			dir.removeRecursively();

			event->accept();
		}
		else
		{
			event->ignore();
		}
	}
}

void AFQSignaturePopup::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape) //Need For CloseEvent
		close();
}

void AFQSignaturePopup::qslotSignatureDownloadTriggered()
{
	QString FolderPath = "";
	if (_SelectedDownloadFolderPath(FolderPath))
	{
		QString path = _SelectedDownloadPath(true, FolderPath);
		_SignatureFromUrl(path, true);


		path = _SelectedDownloadPath(false, FolderPath);
		_SignatureFromUrl(path, false);
	}
}

void AFQSignaturePopup::_InitSignatureWidget()
{
	if (CONFIG_CONTEXT.CheckSavvyTempFolder())
	{
		char savvypath[512] = { 0, };
		if (GetAppConfigPath(savvypath, sizeof(savvypath), "SOOPStudio/savvy") > 0)
			m_savvyFolderPath = QString::fromUtf8(savvypath);
	}

	_CreateLoadingMovie();

	ui->pushButton_CloseButton->setProperty("buttonType", "closeButton");

	ui->pushButton_SignatureStart->setProperty("pushButtonTheme", "type2");
	PolishStyleSheet(ui->pushButton_SignatureStart);
	ui->pushButton_SignatureBalloonCreate->setProperty("pushButtonTheme", "type2");
	PolishStyleSheet(ui->pushButton_SignatureBalloonCreate);
	ui->pushButton_SignatureDownload->setProperty("pushButtonTheme", "type2");
	PolishStyleSheet(ui->pushButton_SignatureDownload);

	connect(ui->pushButton_CloseButton, &QPushButton::clicked, this, &AFQSignaturePopup::close);
	_SetSignatureStartingPage();
	AddSource();

	_SetSignatureBackground();
	_SetSignatureStyle();

	connect(this, &AFQSignaturePopup::qsignalSignatureJobComplete, this, &AFQSignaturePopup::qslotStartSignatureStatusCheck);
	connect(this, &AFQSignaturePopup::qsignalSignatureUpload, this, &AFQSignaturePopup::qslotUploadImageLoaded);
	connect(ui->page_SignatureImageStart, &AFQDragDropImageWidget::qsignalFileUploaded,
		this, &AFQSignaturePopup::qslotShowUploadedImage);
	connect(ui->page_SignatureImageFinish, &AFQDragDropImageWidget::qsignalFileUploaded,
		this, &AFQSignaturePopup::qslotShowUploadedImage);
	connect(ui->page_SignatureImageFailed, &AFQDragDropImageWidget::qsignalFileUploaded,
		this, &AFQSignaturePopup::qslotShowUploadedImage);


	connect(&m_timerCurrentDeivceActive, &QTimer::timeout, 
		this, &AFQSignaturePopup::qslotTimerCheckCurrentDeviceActive);

	ui->lineEdit_SignatureBalloonCount->setMaxLength(5);
	ui->lineEdit_SignatureBalloonCount->setText("1");

	ui->lineEdit_SignatureMessage->setMaxLength(5);

	connect(ui->lineEdit_SignatureBalloonCount, &QLineEdit::textChanged,
		this, &AFQSignaturePopup::qslotSignatureBalloonEditing);

	connect(ui->comboBox_SignatureCamList, &QComboBox::currentIndexChanged,
		this, &AFQSignaturePopup::qslotVideoDeviceListChanged);
	connect(ui->widget_SignatureCam, &AFQSavvyCamWidget::qsignalMouseOnButton,
		this, &AFQSignaturePopup::qslotChangeCamImage);

	connect(ui->pushButton_SignatureStart, &QPushButton::clicked, this, &AFQSignaturePopup::qslotStartSignatureTriggered);

	connect(ui->pushButton_SignatureCamCreate, &QPushButton::clicked, this, &AFQSignaturePopup::qslotUploadCreateTriggered);
	connect(ui->pushButton_SignatureImageCreate, &QPushButton::clicked, this, &AFQSignaturePopup::qslotUploadCreateTriggered);
	connect(ui->pushButton_SignatureUpload, &QPushButton::clicked, this, &AFQSignaturePopup::qslotUploadImageTriggered);
	connect(ui->pushButton_SignatureChangeImage, &QPushButton::clicked, this, &AFQSignaturePopup::qslotUploadImageTriggered);
	connect(ui->pushButton_SignatureReupload, &QPushButton::clicked, this, &AFQSignaturePopup::qslotUploadImageTriggered);

	connect(ui->pushButton_SignatureBalloonCreate, &QPushButton::clicked, this, &AFQSignaturePopup::qslotSignatureCreateTriggered);

	connect(ui->pushButton_SignatureDownload, &QPushButton::clicked, this, &AFQSignaturePopup::qslotSignatureDownloadTriggered);

	connect(ui->pushButton_SignatureNew, &QPushButton::clicked, this, &AFQSignaturePopup::qslotStartSignatureTriggered);

	connect(ui->pushButton_SignatureRecapture, &QPushButton::clicked, this, &AFQSignaturePopup::qslotSignatureCamRecapture);

	connect(ui->pushButton_SignatureRegister, &QPushButton::clicked, this, &AFQSignaturePopup::qslotSignatureNavigateUploadUrl);
}

void AFQSignaturePopup::_SetSignatureStartingPage()
{
	ui->pushButton_CloseButton->setEnabled(true);

	bool skipStart = config_get_bool(CONFIG_CONTEXT.GetAppConfig(), "General", "SkipSavvyStartPage");

	ui->checkBox_SkipStart->setChecked(skipStart);
	if (skipStart)
	{
		qslotStartSignatureTriggered();
	}
	else
	{
		ui->stackedWidget_Signature->setCurrentWidget(ui->page_SignatureStart);
	}
}

void AFQSignaturePopup::_CreateLoadingMovie()
{
	if (m_pLoadingMovie == nullptr)
	{
		std::string absPath;
		GetDataFilePath("assets", absPath);
		QString gifPath = QString("%1/savvy/loading.gif").
			arg(absPath.data());
		m_pLoadingMovie = new QMovie(gifPath, QByteArray(), this);
	}
}

void AFQSignaturePopup::_StartLoadingMovie(bool create)
{
	if (create)
	{
		connect(m_pLoadingMovie, &QMovie::frameChanged, [=] {
			ui->label_SignatureLoadingImage->setPixmap(m_pLoadingMovie->currentPixmap());
			});
	}
	else
	{
		connect(m_pLoadingMovie, &QMovie::frameChanged, [=] {
			ui->label_SignatureLoadingUpload->setPixmap(m_pLoadingMovie->currentPixmap());
			});
	}
	m_pLoadingMovie->start();
}

void AFQSignaturePopup::_StopLoadingMovie()
{
	m_pLoadingMovie->stop();
	disconnect(m_pLoadingMovie, &QMovie::frameChanged, nullptr, nullptr);
}

bool AFQSignaturePopup::isValidImageFile(const QString& filePath)
{
	QFileInfo fileInfo(filePath);

	QString extension = fileInfo.suffix().toLower();
	if (!(extension == "jpg" || extension == "jpeg" || extension == "png"))
	{
		AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
			"", QTStr("Savvy.Signature.ImageSuffix.Failed"), false, true, "", 0, 0, "type1");
		return false;
	}

	if (fileInfo.size() > MAX_FILE_SIZE)
	{
		AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
			"", QTStr("Savvy.Signature.ImageSize.Failed"), false, true, "", 0, 0, "type1");
		return false;
	}

	return true;
}

void AFQSignaturePopup::AddSource()
{
	if (!AFSourceUtil::AddSavvySource("image_source", m_obsImageSource))
		return;

	qslotChangeCamImage(false);

#if defined(_WIN32)
	const char* sourceId = "dshow_input";
#elif defined(__APPLE__)
	const char* sourceId = "av_capture_input";
#endif

	if (!AFSourceUtil::AddSavvySource(sourceId, m_obsInputSource))
		return;

	UpdateSourceComboProperties();
	qslotVideoDeviceListChanged(0);

	obs_source_inc_showing(m_obsInputSource);
	const char* name = obs_source_get_name(m_obsInputSource);
	enum obs_source_type type = obs_source_get_type(m_obsInputSource);

	uint32_t caps = obs_source_get_output_flags(m_obsInputSource);
	bool drawable_type = (type == OBS_SOURCE_TYPE_INPUT ||
		type == OBS_SOURCE_TYPE_SCENE);
	bool drawable_preview = (caps & OBS_SOURCE_NOT_DRAW_PREVIEW) == 0 &&
		(caps & OBS_SOURCE_VIDEO) != 0;

	if (drawable_preview && drawable_type) {
		ui->widget_SignatureCam->show();
		connect(ui->widget_SignatureCam, &AFQTDisplay::qsignalDisplayCreated,
			this, &AFQSignaturePopup::qslotAddDrawCallback);
	}

	connect(ui->widget_SignatureCam, &AFQTDisplay::qsignalLeftMousePressed, this, &AFQSignaturePopup::qslotScreenShot);
}

void AFQSignaturePopup::UpdateSourceComboProperties()
{
	if (!m_obsInputSource)
		return;

	std::unique_ptr<obs_properties_t, properties_delete_t> props =
		properties_t(obs_source_properties(m_obsInputSource), obs_properties_destroy);

    const char* propId;
#ifdef _WIN32
    propId = "video_device_id";
#elif defined(__APPLE__)
    propId = "device";
#endif
    
    obs_property_t* prop = obs_properties_get(props.get(), propId);
    
	OBSDataAutoRelease settings = obs_source_get_settings(m_obsInputSource);
	QString device_id = obs_data_get_string(settings, propId);
	device_id = device_id.split(":").at(0);

	size_t count = obs_property_list_item_count(prop);
	size_t current_idx = 0;

	ui->comboBox_SignatureCamList->blockSignals(true);
	for (size_t idx = 0; idx < count; idx++) {

		const char* name = obs_property_list_item_name(prop, idx);
		QVariant var = propertyListToQVariant(prop, idx);

		ui->comboBox_SignatureCamList->addItem(QT_UTF8(name), var);

		if (0 == device_id.compare(name)) {
			current_idx = idx;
		}
	}
	ui->comboBox_SignatureCamList->setCurrentIndex(current_idx);
	ui->comboBox_SignatureCamList->blockSignals(false);
}

void AFQSignaturePopup::_SignatureFromUrl(QString path, bool isPC)
{
	QFile file(path);

	if (!file.open(QIODevice::WriteOnly)) {
		qDebug() << "Failed to open file for writing:" << path;
		return;
	}

	CURL* curl;
	CURLcode res;

	std::string url = SOOP_SIGNATURE_PC_URL;
	if (!isPC)
		url = SOOP_SIGNATURE_MOBILE_URL;
	url += m_jobID;

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		//curl_easy_setopt(curl, CURLOPT_PROXY, "");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			file.remove();
			curl_easy_cleanup(curl);
			return;
		}

		curl_easy_cleanup(curl);
	}

	file.close();
}

QString AFQSignaturePopup::_DefaultDownloadPath(bool isPC)
{
	QString downloadImage = "";

	if (CONFIG_CONTEXT.CheckSavvyTempFolder())
	{
		downloadImage = m_savvyFolderPath + "/signature_balloon_pc.png";
		if (!isPC)
			downloadImage = m_savvyFolderPath + "/signature_balloon_mobile.png";
	}

	//MAC 경로 확인 필요
	downloadImage.replace("\\", "/");

	return downloadImage;
}

bool AFQSignaturePopup::_SelectedDownloadFolderPath(QString& path)
{
	bool retVal = true;
	path = "";
	path = QFileDialog::getExistingDirectory(nullptr, "폴더 선택", QDir::homePath());

	if (path == "")
		retVal = false;

	return retVal;
}

QString AFQSignaturePopup::_SelectedDownloadPath(bool isPC, QString path)
{
	QString PCVersion = isPC ? "pc" : "mobile";
	QString currentDate = QDate::currentDate().toString("yyyy-MM-dd");
	QString downloadImage = QString("%1/%2signature_balloon_%3.png").arg(path).arg(currentDate).arg(PCVersion);

	bool exist = true;
	int fileCount = 1;

	while (QFileInfo::exists(downloadImage)) 
	{ 
		downloadImage = QString("%1/%2signature_balloon_%3 (%4).png")
			.arg(path)
			.arg(currentDate)
			.arg(PCVersion)
			.arg(fileCount++);
	}

	//MAC 경로 확인 필요
	downloadImage.replace("\\", "/");

	return downloadImage;
}

void AFQSignaturePopup::_SetDownloadedImages()
{
	QString pcPath = _DefaultDownloadPath(true);
	QString mobilePath = _DefaultDownloadPath(false);

	QPixmap pcPixmap(pcPath);
	QPixmap mobilePixmap(mobilePath);
	QPixmap croppedPixmap = mobilePixmap.copy(0, 85, mobilePixmap.width(), mobilePixmap.height() - 85);

	ui->label_SignaturePcImage->setPixmap(pcPixmap.scaled(ui->label_SignaturePcImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
	ui->label_SignatureMobileImage->setPixmap(croppedPixmap.scaled(ui->label_SignatureMobileImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void AFQSignaturePopup::_ChangeImageUploadedButtonStyle(bool uploaded)
{
	if (uploaded)
	{
		ui->pushButton_SignatureImageCreate->setProperty("pushButtonTheme", "type2");
		PolishStyleSheet(ui->pushButton_SignatureImageCreate);
		ui->pushButton_SignatureImageCreate->setEnabled(true);
		m_isSignature = true;
	}
	else
	{
		ui->pushButton_SignatureImageCreate->setProperty("pushButtonTheme", "type1");
		PolishStyleSheet(ui->pushButton_SignatureImageCreate);
		ui->pushButton_SignatureImageCreate->setEnabled(false);
	}
}

void AFQSignaturePopup::_ChangeCamUploadedButtonStyle(bool uploaded)
{
	if (uploaded)
	{
		ui->pushButton_SignatureCamCreate->setProperty("pushButtonTheme", "type2");
		PolishStyleSheet(ui->pushButton_SignatureCamCreate);
		ui->pushButton_SignatureCamCreate->setEnabled(true);
		m_isSignature = true;
	}
	else
	{
		ui->pushButton_SignatureCamCreate->setProperty("pushButtonTheme", "type1");
		PolishStyleSheet(ui->pushButton_SignatureCamCreate);
		ui->pushButton_SignatureCamCreate->setEnabled(false);
	}
}

void AFQSignaturePopup::_SetSignatureBackground()
{
	QGridLayout* layout = new QGridLayout();
	layout->setSpacing(6);
	layout->setContentsMargins(10, 10, 10, 10);

	std::string absPath;
	GetDataFilePath("assets/savvy/style/random.png", absPath);
	AFQSavvyStyleButton* button1 = new AFQSavvyStyleButton(
		"0",
		QString::fromStdString(absPath),
		QTStr("Savvy.Style.Random"));
	button1->CheckStyle(true);
	m_selectedBackground = button1->StyleNum();

	GetDataFilePath("assets/savvy/style/heart.png", absPath);
	AFQSavvyStyleButton* button2 = new AFQSavvyStyleButton(
		"8",
		QString::fromStdString(absPath),
		QTStr("Savvy.Style.Heart"));

	GetDataFilePath("assets/savvy/style/forest.png", absPath);
	AFQSavvyStyleButton* button3 = new AFQSavvyStyleButton(
		"6",
		QString::fromStdString(absPath),
		QTStr("Savvy.Style.Forest"));

	GetDataFilePath("assets/savvy/style/blackboard.png", absPath);
	AFQSavvyStyleButton* button4 = new AFQSavvyStyleButton(
		"7",
		QString::fromStdString(absPath),
		QTStr("Savvy.Style.BlackBoard"));

	GetDataFilePath("assets/savvy/style/winter.png", absPath);
	AFQSavvyStyleButton* button5 = new AFQSavvyStyleButton(
		"1",
		QString::fromStdString(absPath),
		QTStr("Savvy.Style.Winter"));

	GetDataFilePath("assets/savvy/style/yellowflower.png", absPath);
	AFQSavvyStyleButton* button6 = new AFQSavvyStyleButton(
		"4",
		QString::fromStdString(absPath),
		QTStr("Savvy.Style.YelloFlower"));

	GetDataFilePath("assets/savvy/style/neon.png", absPath);
	AFQSavvyStyleButton* button7 = new AFQSavvyStyleButton(
		"3",
		QString::fromStdString(absPath),
		QTStr("Savvy.Style.Neon"));

	GetDataFilePath("assets/savvy/style/cloud.png", absPath);
	AFQSavvyStyleButton* button8 = new AFQSavvyStyleButton(
		"2",
		QString::fromStdString(absPath),
		QTStr("Savvy.Style.Cloud"));

	GetDataFilePath("assets/savvy/style/blackgold.png", absPath);
	AFQSavvyStyleButton* button9 = new AFQSavvyStyleButton(
		"5",
		QString::fromStdString(absPath),
		QTStr("Savvy.Style.BlackGold"));
	button9->setProperty("clicked", false);

	m_backGroundButtons.append(button1);
	m_backGroundButtons.append(button2);
	m_backGroundButtons.append(button3);
	m_backGroundButtons.append(button4);
	m_backGroundButtons.append(button5);
	m_backGroundButtons.append(button6);
	m_backGroundButtons.append(button7);
	m_backGroundButtons.append(button8);
	m_backGroundButtons.append(button9);

	int row = 3;
	int column = 3;
	int rowCount = 0;
	int columnCount = 0;

	for (int i = 0; i < m_backGroundButtons.count(); i++)
	{
		layout->addWidget(m_backGroundButtons[i], rowCount, columnCount);
		columnCount++;

		if (columnCount > column - 1)
		{
			columnCount = 0;
			rowCount++;
		}
		connect(m_backGroundButtons[i], &AFQHoverWidget::qsignalMouseClick,
			this, &AFQSignaturePopup::qslotSignatureBackgroundSelectTriggered);
	}

	ui->widget_SignatureBackground->setLayout(layout);
}

void AFQSignaturePopup::_SetSignatureStyle()
{
	QGridLayout* layout = new QGridLayout();
	layout->setSpacing(6);
	layout->setContentsMargins(10, 10, 10, 10);

	std::string absPath;
	GetDataFilePath("assets/savvy/style/2d.png", absPath);
	AFQSavvyStyleButton* button1 = new AFQSavvyStyleButton(
		"2",
		QString::fromStdString(absPath),
		QTStr("Savvy.Style.2D"));
	button1->CheckStyle(true);
	m_selectedStyle = button1->StyleNum();

	GetDataFilePath("assets/savvy/style/halfreal.png", absPath);
	AFQSavvyStyleButton* button2 = new AFQSavvyStyleButton(
		"1",
		QString::fromStdString(absPath),
		QTStr("Savvy.Style.HalfReal"));

	GetDataFilePath("assets/savvy/style/real.png", absPath);
	AFQSavvyStyleButton* button3 = new AFQSavvyStyleButton(
		"0",
		QString::fromStdString(absPath),
		QTStr("Savvy.Style.Real"));

	m_styleButtons.append(button1);
	m_styleButtons.append(button2);
	m_styleButtons.append(button3);

	int row = 3;
	int column = 1;
	int rowCount = 0;
	int columnCount = 0;

	for (int i = 0; i < m_styleButtons.count(); i++)
	{
		layout->addWidget(m_styleButtons[i], rowCount, columnCount);
		columnCount++;

		if (columnCount > column - 1)
		{
			columnCount = 0;
			rowCount++;
		}
		connect(m_styleButtons[i], &AFQHoverWidget::qsignalMouseClick,
			this, &AFQSignaturePopup::qslotSignatureStyleSelectTriggered);
	}

	ui->widget_SignatureStyle->setLayout(layout);
}

