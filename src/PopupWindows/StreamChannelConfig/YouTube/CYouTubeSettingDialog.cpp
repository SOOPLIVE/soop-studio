#include "CYouTubeSettingDialog.hpp"

#include <QToolTip>
#include <QDateTime>
#include <QDesktopServices>
#include <QFileInfo>
#include <QStandardPaths>
#include <QImageReader>

#include "qt-wrappers.hpp"
#include "Application/CApplication.h"

#include "clickable-label.hpp"

#include "UIComponent/CMessageBox.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "ViewModel/Auth/YouTube/youtube-api-wrappers.hpp"

const QString SchedulDateAndTimeFormat = "yyyy-MM-dd'T'hh:mm:ss'Z'";
const QString RepresentSchedulDateAndTimeFormat = "dddd, MMMM d, yyyy h:m";
const QString IndexOfGamingCategory = "20";

AFQYouTubeSettingDialog::AFQYouTubeSettingDialog(QWidget *parent, AFAuth *auth,
				     bool broadcastReady)
	: AFTTopBaseDialog(parent),
	  ui(new Ui::AFQYouTubeSettingDialog),
	  m_pApiYouTube(dynamic_cast<YoutubeApiWrappers *>(auth)),
	  m_pWorkerThread(new WorkerThread(m_pApiYouTube)),
	  m_broadcastReady(broadcastReady)
{
	ui->setupUi(this);

	ui->privacyBox->addItem(QTStr("YouTube.Actions.Privacy.Public"),
				"public");
	ui->privacyBox->addItem(QTStr("YouTube.Actions.Privacy.Unlisted"),
				"unlisted");
	ui->privacyBox->addItem(QTStr("YouTube.Actions.Privacy.Private"),
				"private");

	ui->latencyBox->addItem(QTStr("YouTube.Actions.Latency.Normal"),
				"normal");
	ui->latencyBox->addItem(QTStr("YouTube.Actions.Latency.Low"), "low");
	ui->latencyBox->addItem(QTStr("YouTube.Actions.Latency.UltraLow"),
				"ultraLow");

	UpdateOkButtonStatus();

	connect(ui->title, &QLineEdit::textChanged, this,
		[&](const QString &) { this->UpdateOkButtonStatus(); });
	connect(ui->privacyBox, &QComboBox::currentTextChanged, this,
		[&](const QString &) { this->UpdateOkButtonStatus(); });
	connect(ui->yesMakeForKids, &QRadioButton::toggled, this,
		[&](bool) { this->UpdateOkButtonStatus(); });
	connect(ui->notMakeForKids, &QRadioButton::toggled, this,
		[&](bool) { this->UpdateOkButtonStatus(); });
	connect(ui->tabWidget, &QTabWidget::currentChanged, this,
		[&](int) { this->UpdateOkButtonStatus(); });
	connect(ui->pushButton, &QPushButton::clicked, this,
		&AFQYouTubeSettingDialog::OpenYouTubeDashboard);

	ui->pushButton->setProperty("pushButtonTheme", "type4");

	connect(ui->helpAutoStartStop, &QLabel::linkActivated, this,
		[](const QString &) {
			QToolTip::showText(
				QCursor::pos(),
				QTStr("YouTube.Actions.AutoStartStop.TT"));
		});
	connect(ui->help360Video, &QLabel::linkActivated, this,
		[](const QString &link) { QDesktopServices::openUrl(link); });
	connect(ui->helpMadeForKids, &QLabel::linkActivated, this,
		[](const QString &link) { QDesktopServices::openUrl(link); });

	ui->scheduledTime->setVisible(false);
	connect(ui->checkScheduledLater, &QCheckBox::stateChanged, this,
		[&](int state) {
			ui->scheduledTime->setVisible(state);
			if (state) {
				ui->checkAutoStart->setVisible(true);
				ui->checkAutoStop->setVisible(true);
				ui->helpAutoStartStop->setVisible(true);

				ui->checkAutoStart->setChecked(false);
				ui->checkAutoStop->setChecked(false);
			} else {
				ui->checkAutoStart->setVisible(false);
				ui->checkAutoStop->setVisible(false);
				ui->helpAutoStartStop->setVisible(false);

				ui->checkAutoStart->setChecked(true);
				ui->checkAutoStop->setChecked(true);
			}
			UpdateOkButtonStatus();
		});

	ui->checkAutoStart->setVisible(false);
	ui->checkAutoStop->setVisible(false);
	ui->helpAutoStartStop->setVisible(false);

	ui->scheduledTime->setDateTime(QDateTime::currentDateTime());

	auto thumbSelectionHandler = [&]() {
		if (m_thumbnailFile.isEmpty()) {
			QString filePath = OpenFile(
				this,
				QTStr("YouTube.Actions.Thumbnail.SelectFile"),
				QStandardPaths::writableLocation(
					QStandardPaths::PicturesLocation),
				QString("Images (*.png *.jpg *.jpeg *.gif)"));

			if (!filePath.isEmpty()) {
				QFileInfo tFile(filePath);
				if (!tFile.exists()) {
					return ShowErrorDialog(
						this,
						QTStr("YouTube.Actions.Error.FileMissing"));
				} else if (tFile.size() > 2 * 1024 * 1024) {
					return ShowErrorDialog(
						this,
						QTStr("YouTube.Actions.Error.FileTooLarge"));
				}

				m_thumbnailFile = filePath;
				ui->selectedFileName->setText(m_thumbnailFile);
				ui->selectFileButton->setText(QTStr(
					"YouTube.Actions.Thumbnail.ClearFile"));

				QImageReader imgReader(filePath);
				imgReader.setAutoTransform(true);
				const QImage newImage = imgReader.read();
				ui->thumbnailPreview->setPixmap(
					QPixmap::fromImage(newImage).scaled(
						160, 90, Qt::KeepAspectRatio,
						Qt::SmoothTransformation));
			}
		} else {
			m_thumbnailFile.clear();
			ui->selectedFileName->setText(QTStr(
				"YouTube.Actions.Thumbnail.NoFileSelected"));
			ui->selectFileButton->setText(
				QTStr("YouTube.Actions.Thumbnail.SelectFile"));
			ui->thumbnailPreview->setPixmap(
				GetPlaceholder().pixmap(QSize(16, 16)));
		}
	};

	connect(ui->selectFileButton, &QPushButton::clicked, this, thumbSelectionHandler);
	connect(ui->thumbnailPreview, &ClickableLabel::clicked, this, thumbSelectionHandler);

	if (!m_pApiYouTube) {
		blog(LOG_DEBUG, "YouTube API auth NOT found.");
		Cancel();
		return;
	}

    std::string channelName;
    
    AFOAuth* rawAuth = dynamic_cast<AFOAuth*>(m_pApiYouTube);
    AFBasicAuth* pDataAuthed = nullptr;
    if (rawAuth != nullptr)
        rawAuth->GetConnectedAFBasicAuth(pDataAuthed);
    if (pDataAuthed != nullptr)
        channelName = pDataAuthed->channelID;
    
	QString caption = QTStr("YouTube.Actions.WindowTitle").arg(channelName.c_str());
#ifdef _WIN32
	ui->titleLabel->setText(caption);
#elif (__APPLE__)
	setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint);
	setWindowTitle(caption);
	ui->titleFrame->hide();
#endif

	connect(ui->cancelButton, &QPushButton::clicked, this, &AFQYouTubeSettingDialog::qslotCancel);
	connect(ui->closeButton, &QPushButton::clicked, this, &AFQYouTubeSettingDialog::qslotCancel);

	QVector<CategoryDescription> category_list;
	if (!m_pApiYouTube->GetVideoCategoriesList(category_list)) {
		ShowErrorDialog(
			parent,
			m_pApiYouTube->GetLastError().isEmpty()
				? QTStr("YouTube.Actions.Error.General")
				: QTStr("YouTube.Actions.Error.Text")
					  .arg(m_pApiYouTube->GetLastError()));
		Cancel();
		return;
	}
	for (auto &category : category_list) {
		ui->categoryBox->addItem(category.title, category.id);
		if (category.id == IndexOfGamingCategory) {
			ui->categoryBox->setCurrentText(category.title);
		}
	}

	connect(ui->okButton, &QPushButton::clicked, this,
		&AFQYouTubeSettingDialog::InitBroadcast);
	/*connect(ui->saveButton, &QPushButton::clicked, this,
		&AFQYouTubeSettingDialog::ReadyBroadcast);*/

	qDeleteAll(ui->scrollAreaWidgetContents->findChildren<QWidget *>(
		QString(), Qt::FindDirectChildrenOnly));

	// Add label indicating loading state
	QLabel *loadingLabel = new QLabel();
	loadingLabel->setTextFormat(Qt::RichText);
	loadingLabel->setAlignment(Qt::AlignHCenter);
	loadingLabel->setText(
		QString("<big>%1</big>")
			.arg(QTStr("YouTube.Actions.EventsLoading")));
	ui->scrollAreaWidgetContents->layout()->addWidget(loadingLabel);

	// Delete "loading..." label on completion
	connect(m_pWorkerThread, &WorkerThread::finished, this, [&] {
		QLayoutItem *item =
			ui->scrollAreaWidgetContents->layout()->takeAt(0);
		item->widget()->deleteLater();
	});

	connect(m_pWorkerThread, &WorkerThread::failed, this, [&]() {
		auto last_error = m_pApiYouTube->GetLastError();
		if (last_error.isEmpty())
			last_error = QTStr("YouTube.Actions.Error.YouTubeApi");

		if (!m_pApiYouTube->GetTranslatedError(last_error))
			last_error = QTStr("YouTube.Actions.Error.Text")
					     .arg(last_error);

		ShowErrorDialog(this, last_error);
		QDialog::reject();
	});

	connect(m_pWorkerThread, &WorkerThread::new_item, this,
		[&](const QString &title, const QString &dateTimeString,
		    const QString &broadcast, const QString &status,
		    bool astart, bool astop) {
			ClickableLabel *label = new ClickableLabel();
			label->setTextFormat(Qt::RichText);

			if (status == "live" || status == "testing") {
				// Resumable stream
				label->setText(
					QString("<big>%1</big><br/>%2")
						.arg(title,
						     QTStr("YouTube.Actions.Stream.Resume")));

			} else if (dateTimeString.isEmpty()) {
				// The broadcast created by YouTube Studio has no start time.
				// Yes this does violate the restrictions set in YouTube's API
				// But why would YouTube care about consistency?
				label->setText(
					QString("<big>%1</big><br/>%2")
						.arg(title,
						     QTStr("YouTube.Actions.Stream.YTStudio")));
			} else {
				label->setText(
					QString("<big>%1</big><br/>%2")
						.arg(title,
						     QTStr("YouTube.Actions.Stream.ScheduledFor")
							     .arg(dateTimeString)));
			}

			label->setAlignment(Qt::AlignHCenter);
			label->setMargin(4);

			connect(label, &ClickableLabel::clicked, this,
				[&, label, broadcast, astart, astop]() {
					for (QWidget *i :
					     ui->scrollAreaWidgetContents->findChildren<
						     QWidget *>(
						     QString(),
						     Qt::FindDirectChildrenOnly)) {

						i->setProperty(
							"isSelectedEvent",
							"false");

						PolishStyleSheet(i);
					}
					label->setProperty("isSelectedEvent",
							   "true");
					PolishStyleSheet(label);

					this->m_selectedBroadcast = broadcast;
					this->m_autostart = astart;
					this->m_autostop = astop;
					UpdateOkButtonStatus();
				});
			ui->scrollAreaWidgetContents->layout()->addWidget(
				label);

			if (m_selectedBroadcast == broadcast)
				label->clicked();
		});
	m_pWorkerThread->start();


	bool rememberSettings = config_get_bool(ACTIVECONFIG, "YouTube", "RememberSettings");
	if (rememberSettings)
		LoadSettings();

	// Switch to events page and select readied broadcast once loaded
	if (m_broadcastReady) {
		ui->tabWidget->setCurrentIndex(1);
		m_selectedBroadcast = m_pApiYouTube->GetBroadcastId();
	}

#ifdef __APPLE__
	// MacOS theming issues
	this->resize(this->width() + 200, this->height() + 120);
#endif
	m_valid = true;
}


void AFQYouTubeSettingDialog::qslotCancel()
{
	blog(LOG_DEBUG, "YouTube live broadcast creation cancelled.");
	Cancel();
}

void AFQYouTubeSettingDialog::showEvent(QShowEvent *event)
{
	if (MAINFRAME->IsSmallResolution())
		resize(800, 550);

	QDialog::showEvent(event);
	if (m_thumbnailFile.isEmpty())
		ui->thumbnailPreview->setPixmap(
			GetPlaceholder().pixmap(QSize(16, 16)));
}

AFQYouTubeSettingDialog::~AFQYouTubeSettingDialog()
{
	m_pWorkerThread->stop();
	m_pWorkerThread->wait();

	delete m_pWorkerThread;
}

void WorkerThread::run()
{
	if (!m_pending)
		return;

	emit ready();
}

void AFQYouTubeSettingDialog::UpdateOkButtonStatus()
{
	bool enable = false;
   
	if (ui->tabWidget->currentIndex() == 0) {
		enable = !ui->title->text().isEmpty() &&
			 !ui->privacyBox->currentText().isEmpty() &&
			 (ui->yesMakeForKids->isChecked() ||
			  ui->notMakeForKids->isChecked());
            ui->okButton->setEnabled(enable);
		//ui->saveButton->setEnabled(enable);

		if (ui->checkScheduledLater->checkState() == Qt::Checked) {
			ui->okButton->setText(
				QTStr("YouTube.Actions.Create_Schedule"));
			/*ui->saveButton->setText(
				QTStr("YouTube.Actions.Create_Schedule_Ready"));*/
		} else {
			ui->okButton->setText(
				QTStr("YouTube.Actions.Create_GoLive"));
			/*ui->saveButton->setText(
				QTStr("YouTube.Actions.Create_Ready"));*/
		}
		//ui->pushButton->setVisible(false);
	} else {
		enable = !m_selectedBroadcast.isEmpty();
		ui->okButton->setEnabled(enable);
		//ui->saveButton->setEnabled(enable);
		ui->okButton->setText(QTStr("YouTube.Actions.Choose_GoLive"));
		//ui->saveButton->setText(QTStr("YouTube.Actions.Choose_Ready"));

		//ui->pushButton->setVisible(true);
	}
}
bool AFQYouTubeSettingDialog::CreateEventAction(YoutubeApiWrappers *api,
					  BroadcastDescription &broadcast,
					  StreamDescription &stream,
					  bool stream_later,
					  bool ready_broadcast)
{
	YoutubeApiWrappers *apiYouTube = api;
	UiToBroadcast(broadcast);

	if (stream_later) {
		// DateTime parser means that input datetime is a local, so we need to move it
		auto dateTime = ui->scheduledTime->dateTime();
		auto utcDTime = dateTime.addSecs(-dateTime.offsetFromUtc());
		broadcast.schedul_date_time =
			utcDTime.toString(SchedulDateAndTimeFormat);
	} else {
		// stream now is always autostart/autostop
		broadcast.auto_start = true;
		broadcast.auto_stop = true;
		broadcast.schedul_date_time =
			QDateTime::currentDateTimeUtc().toString(
				SchedulDateAndTimeFormat);
	}

	m_autostart = broadcast.auto_start;
	m_autostop = broadcast.auto_stop;

	blog(LOG_DEBUG, "Scheduled date and time: %s",
	     broadcast.schedul_date_time.toStdString().c_str());
	if (!apiYouTube->InsertBroadcast(broadcast)) {
		blog(LOG_DEBUG, "No broadcast created.");
		return false;
	}
	if (!apiYouTube->SetVideoCategory(broadcast.id, broadcast.title,
					  broadcast.description,
					  broadcast.category.id)) {
		blog(LOG_DEBUG, "No category set.");
		return false;
	}
	if (!m_thumbnailFile.isEmpty()) {
		blog(LOG_INFO, "Uploading thumbnail file \"%s\"...",
		     m_thumbnailFile.toStdString().c_str());
		if (!apiYouTube->SetVideoThumbnail(broadcast.id,
						   m_thumbnailFile)) {
			blog(LOG_DEBUG, "No thumbnail set.");
			return false;
		}
	}

	if (!stream_later || ready_broadcast) {
		stream = {"", "", "SOOP Studio Video Stream"};
		if (!apiYouTube->InsertStream(stream)) {
			blog(LOG_DEBUG, "No stream created.");
			return false;
		}
	}

#ifdef YOUTUBE_ENABLED
	if (OBSBasic::Get()->GetYouTubeAppDock())
		OBSBasic::Get()->GetYouTubeAppDock()->BroadcastCreated(
			broadcast.id.toStdString().c_str());
#endif

	return true;
}

bool AFQYouTubeSettingDialog::ChooseAnEventAction(YoutubeApiWrappers *api,
					    StreamDescription &stream)
{
	YoutubeApiWrappers *apiYouTube = api;

    _SaveStreamKeyToAFAuthManager(stream.name.toStdString());

#ifdef YOUTUBE_ENABLED
	if (OBSBasic::Get()->GetYouTubeAppDock())
		OBSBasic::Get()->GetYouTubeAppDock()->BroadcastSelected(
			m_selectedBroadcast.toStdString().c_str());
#endif

	return true;
}

void AFQYouTubeSettingDialog::ShowErrorDialog(QWidget *parent, QString text)
{
	AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
							   QT_UTF8(""), text);

	//QMessageBox dlg(parent);
	//dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowCloseButtonHint);
	//dlg.setWindowTitle(QTStr("YouTube.Actions.Error.Title"));
	//dlg.setText(text);
	//dlg.setTextFormat(Qt::RichText);
	//dlg.setIcon(QMessageBox::Warning);
	//dlg.setStandardButtons(QMessageBox::StandardButton::Ok);
	//dlg.exec();
}

void AFQYouTubeSettingDialog::InitBroadcast()
{
	BroadcastDescription broadcast;
	StreamDescription stream;

	AFQMessageBox mb(QDialogButtonBox::StandardButtons(), this, QTStr("YouTube.Actions.Notify.Title"), 
		QTStr("YouTube.Actions.Notify.CreatingBroadcast"), true);

	//QMessageBox msgBox(this);
	//msgBox.setWindowFlags(msgBox.windowFlags() &
	//		      ~Qt::WindowCloseButtonHint);
	//msgBox.setWindowTitle(QTStr("YouTube.Actions.Notify.Title"));
	//msgBox.setText(QTStr("YouTube.Actions.Notify.CreatingBroadcast"));
	//msgBox.setStandardButtons(QMessageBox::StandardButtons());

	bool success = false;
	auto action = [&]() {
		if (ui->tabWidget->currentIndex() == 0) {
			success = this->CreateEventAction(
				m_pApiYouTube, broadcast, stream,
				ui->checkScheduledLater->isChecked());
		} else {
			success = this->ChooseAnEventAction(m_pApiYouTube, stream);
			if (success)
				broadcast.id = this->m_selectedBroadcast;
		};
		QMetaObject::invokeMethod(&mb, "accept",
					  Qt::QueuedConnection);
	};
	QScopedPointer<QThread> thread(CreateQThread(action));
	thread->start();
	mb.exec();
	thread->wait();

	if (success) {
		if (ui->tabWidget->currentIndex() == 0) {
			// Stream later usecase.
			if (ui->checkScheduledLater->isChecked()) {
				AFQMessageBox msg(QDialogButtonBox::Ok, this, QTStr("YouTube.Actions.EventCreated.Title"),
					QTStr("YouTube.Actions.EventCreated.Text"), true);
				//QMessageBox msg(this);
				//msg.setWindowTitle(QTStr(
				//	"YouTube.Actions.EventCreated.Title"));
				//msg.setText(QTStr(
				//	"YouTube.Actions.EventCreated.Text"));
				//msg.setStandardButtons(QMessageBox::Ok);
				msg.exec();
				// Close dialog without start streaming.
				Cancel();
			} else {
				// Stream now usecase.
				blog(LOG_DEBUG, "New valid stream: %s",
				     QT_TO_UTF8(stream.name));
				emit ok(QT_TO_UTF8(broadcast.id),
					QT_TO_UTF8(stream.id),
					QT_TO_UTF8(stream.name), true, true,
					true);
                _SaveStreamKeyToAFAuthManager(stream.name.toStdString());
				Accept();
			}
		} else {
			// Stream to precreated broadcast usecase.
			emit ok(QT_TO_UTF8(broadcast.id), QT_TO_UTF8(stream.id),
				QT_TO_UTF8(stream.name), m_autostart, m_autostop,
				true);
            _SaveStreamKeyToAFAuthManager(stream.name.toStdString());
			Accept();
		}
	} else {
		// Fail.
		auto last_error = m_pApiYouTube->GetLastError();
		if (last_error.isEmpty())
			last_error = QTStr("YouTube.Actions.Error.YouTubeApi");
		if (!m_pApiYouTube->GetTranslatedError(last_error))
			last_error =
				QTStr("YouTube.Actions.Error.NoBroadcastCreated")
					.arg(last_error);

		ShowErrorDialog(this, last_error);
	}
}


// TODO: remove function (deprecated)
void AFQYouTubeSettingDialog::ReadyBroadcast()
{
	BroadcastDescription broadcast;
	StreamDescription stream;
	QMessageBox msgBox(this);
	msgBox.setWindowFlags(msgBox.windowFlags() &
			      ~Qt::WindowCloseButtonHint);
	msgBox.setWindowTitle(QTStr("YouTube.Actions.Notify.Title"));
	msgBox.setText(QTStr("YouTube.Actions.Notify.CreatingBroadcast"));
	msgBox.setStandardButtons(QMessageBox::StandardButtons());

	bool success = false;
	auto action = [&]() {
		if (ui->tabWidget->currentIndex() == 0) {
			success = this->CreateEventAction(
				m_pApiYouTube, broadcast, stream,
				ui->checkScheduledLater->isChecked(), true);
		} else {
			success = this->ChooseAnEventAction(m_pApiYouTube, stream);
			if (success)
				broadcast.id = this->m_selectedBroadcast;
		};
		QMetaObject::invokeMethod(&msgBox, "accept",
					  Qt::QueuedConnection);
	};
	QScopedPointer<QThread> thread(CreateQThread(action));
	thread->start();
	msgBox.exec();
	thread->wait();

	if (success) {
        _SaveStreamKeyToAFAuthManager(stream.name.toStdString());

		emit ok(QT_TO_UTF8(broadcast.id), QT_TO_UTF8(stream.id),
			QT_TO_UTF8(stream.name), m_autostart, m_autostop, false);
		Accept();
	} else {
		// Fail.
		auto last_error = m_pApiYouTube->GetLastError();
		if (last_error.isEmpty())
			last_error = QTStr("YouTube.Actions.Error.YouTubeApi");
		if (!m_pApiYouTube->GetTranslatedError(last_error))
			last_error =
				QTStr("YouTube.Actions.Error.NoBroadcastCreated")
					.arg(last_error);

		ShowErrorDialog(this, last_error);
	}
}

void AFQYouTubeSettingDialog::UiToBroadcast(BroadcastDescription &broadcast)
{
	broadcast.title = ui->title->text();
	// ToDo: UI warning rather than silent truncation
	broadcast.description = ui->description->toPlainText().left(5000);
	broadcast.privacy = ui->privacyBox->currentData().toString();
	broadcast.category.title = ui->categoryBox->currentText();
	broadcast.category.id = ui->categoryBox->currentData().toString();
	broadcast.made_for_kids = ui->yesMakeForKids->isChecked();
	broadcast.latency = ui->latencyBox->currentData().toString();
	broadcast.auto_start = ui->checkAutoStart->isChecked();
	broadcast.auto_stop = ui->checkAutoStop->isChecked();
	broadcast.dvr = ui->checkDVR->isChecked();
	broadcast.schedul_for_later = ui->checkScheduledLater->isChecked();
	broadcast.projection = ui->check360Video->isChecked() ? "360"
							      : "rectangular";

	if (ui->checkRememberSettings->isChecked())
		SaveSettings(broadcast);
}

void AFQYouTubeSettingDialog::SaveSettings(BroadcastDescription &broadcast)
{
	config_t* config = ACTIVECONFIG;
	//
	config_set_string(config, "YouTube", "Title", QT_TO_UTF8(broadcast.title));
	config_set_string(config, "YouTube", "Description", QT_TO_UTF8(broadcast.description));
	config_set_string(config, "YouTube", "Privacy", QT_TO_UTF8(broadcast.privacy));
	config_set_string(config, "YouTube", "CategoryID", QT_TO_UTF8(broadcast.category.id));
	config_set_string(config, "YouTube", "Latency", QT_TO_UTF8(broadcast.latency));
	config_set_bool(config, "YouTube", "MadeForKids", broadcast.made_for_kids);
	config_set_bool(config, "YouTube", "AutoStart", broadcast.auto_start);
	config_set_bool(config, "YouTube", "AutoStop", broadcast.auto_start);
	config_set_bool(config, "YouTube", "DVR", broadcast.dvr);
	config_set_bool(config, "YouTube", "ScheduleForLater", broadcast.schedul_for_later);
	config_set_string(config, "YouTube", "Projection", QT_TO_UTF8(broadcast.projection));
	config_set_string(config, "YouTube", "ThumbnailFile", QT_TO_UTF8(m_thumbnailFile));
	config_set_bool(config, "YouTube", "RememberSettings", true);
}

void AFQYouTubeSettingDialog::LoadSettings()
{
	config_t* config = ACTIVECONFIG;
	//
	const char *title = config_get_string(config, "YouTube", "Title");
	ui->title->setText(QT_UTF8(title));

	const char *desc = config_get_string(config, "YouTube", "Description");
	ui->description->setPlainText(QT_UTF8(desc));

	const char *priv = config_get_string(config, "YouTube", "Privacy");
	int index = ui->privacyBox->findData(priv);
	ui->privacyBox->setCurrentIndex(index);

	const char *catID = config_get_string(config, "YouTube", "CategoryID");
	index = ui->categoryBox->findData(catID);
	ui->categoryBox->setCurrentIndex(index);

	const char *latency = config_get_string(config, "YouTube", "Latency");
	index = ui->latencyBox->findData(latency);
	ui->latencyBox->setCurrentIndex(index);

	bool dvr = config_get_bool(config, "YouTube", "DVR");
	ui->checkDVR->setChecked(dvr);

	bool forKids =
		config_get_bool(config, "YouTube", "MadeForKids");
	if (forKids)
		ui->yesMakeForKids->setChecked(true);
	else
		ui->notMakeForKids->setChecked(true);

	bool schedLater = config_get_bool(config, "YouTube", "ScheduleForLater");
	ui->checkScheduledLater->setChecked(schedLater);

	bool autoStart =
		config_get_bool(config, "YouTube", "AutoStart");
	ui->checkAutoStart->setChecked(autoStart);

	bool autoStop =
		config_get_bool(config, "YouTube", "AutoStop");
	ui->checkAutoStop->setChecked(autoStop);

	const char *projection =
		config_get_string(config, "YouTube", "Projection");
	if (projection && *projection) {
		if (strcmp(projection, "360") == 0)
			ui->check360Video->setChecked(true);
		else
			ui->check360Video->setChecked(false);
	}

	const char *thumbFile = config_get_string(config, "YouTube", "ThumbnailFile");
	if (thumbFile && *thumbFile) {
		QFileInfo tFile(thumbFile);
		// Re-check validity before setting path again
		if (tFile.exists() && tFile.size() <= 2 * 1024 * 1024) {
			m_thumbnailFile = tFile.absoluteFilePath();
			ui->selectedFileName->setText(m_thumbnailFile);
			ui->selectFileButton->setText(
				QTStr("YouTube.Actions.Thumbnail.ClearFile"));

			QImageReader imgReader(m_thumbnailFile);
			imgReader.setAutoTransform(true);
			const QImage newImage = imgReader.read();
			ui->thumbnailPreview->setPixmap(
				QPixmap::fromImage(newImage).scaled(
					160, 90, Qt::KeepAspectRatio,
					Qt::SmoothTransformation));
		}
	}
}

void AFQYouTubeSettingDialog::_SaveStreamKeyToAFAuthManager(std::string streamKey)
{
    AFOAuth* rawAuth = dynamic_cast<AFOAuth*>(m_pApiYouTube);
    AFBasicAuth* pDataAuthed = nullptr;
    if (rawAuth != nullptr)
        rawAuth->GetConnectedAFBasicAuth(pDataAuthed);
    if (pDataAuthed != nullptr)
    {
        pDataAuthed->checkedRTMPKey = true;
        pDataAuthed->keyRTMP = streamKey;
        // fixed url !! need check
        pDataAuthed->urlRTMP = YOUTUBE_RTMP_URL;
    }
}

void AFQYouTubeSettingDialog::OpenYouTubeDashboard()
{
	ChannelDescription channel;
	if (!m_pApiYouTube->GetChannelDescription(channel)) {
		blog(LOG_DEBUG, "Could not get channel description.");
		ShowErrorDialog(
			this,
			m_pApiYouTube->GetLastError().isEmpty()
				? QTStr("YouTube.Actions.Error.General")
				: QTStr("YouTube.Actions.Error.Text")
					  .arg(m_pApiYouTube->GetLastError()));
		return;
	}

	//https://studio.youtube.com/channel/UCA9bSfH3KL186kyiUsvi3IA/videos/live?filter=%5B%5D&sort=%7B%22columnType%22%3A%22date%22%2C%22sortOrder%22%3A%22DESCENDING%22%7D
	QString uri =
		QString("https://studio.youtube.com/channel/%1/videos/live?filter=[]&sort={\"columnType\"%3A\"date\"%2C\"sortOrder\"%3A\"DESCENDING\"}")
			.arg(channel.id);
	QDesktopServices::openUrl(uri);
}

void AFQYouTubeSettingDialog::Cancel()
{
	m_pWorkerThread->stop();
	reject();
}
void AFQYouTubeSettingDialog::Accept()
{
	m_pWorkerThread->stop();
	accept();
}
