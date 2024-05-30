#pragma once

#include <QDialog>
#include <QString>
#include <QThread>

#include "ui_stream-channel-youtube-setting.h"
#include "ViewModel/Auth/YouTube/youtube-api-wrappers.hpp"

#include "UIComponent/CRoundedDialogBase.h"

class WorkerThread : public QThread {
	Q_OBJECT
public:
	WorkerThread(YoutubeApiWrappers *api) : QThread(), apiYouTube(api) {}

	void stop() { pending = false; }

protected:
	YoutubeApiWrappers *apiYouTube;
	bool pending = true;

public slots:
	void run() override;
signals:
	void ready();
	void new_item(const QString &title, const QString &dateTimeString,
		      const QString &broadcast, const QString &status,
		      bool astart, bool astop);
	void failed();
};

class AFQYouTubeSettingDialog : public AFQRoundedDialogBase {
	Q_OBJECT
	Q_PROPERTY(QIcon thumbPlaceholder READ GetPlaceholder WRITE
			   SetPlaceholder DESIGNABLE true)

	std::unique_ptr<Ui::AFQYouTubeSettingDialog> ui;

signals:
	void ok(const QString &broadcast_id, const QString &stream_id,
		const QString &key, bool autostart, bool autostop,
		bool start_now);

private slots:
	void qslotCancel();

protected:
	void showEvent(QShowEvent *event) override;
	void UpdateOkButtonStatus();

	bool CreateEventAction(YoutubeApiWrappers *api,
			       BroadcastDescription &broadcast,
			       StreamDescription &stream, bool stream_later,
			       bool ready_broadcast = false);
	bool ChooseAnEventAction(YoutubeApiWrappers *api,
				 StreamDescription &stream);

	void ShowErrorDialog(QWidget *parent, QString text);

public:
	explicit AFQYouTubeSettingDialog(QWidget *parent, AFAuth *auth,
				   bool broadcastReady);
	virtual ~AFQYouTubeSettingDialog() override;

	bool Valid() { return valid; };

private:
	void InitBroadcast();
	void ReadyBroadcast();
	void UiToBroadcast(BroadcastDescription &broadcast);
	void OpenYouTubeDashboard();
	void Cancel();
	void Accept();
	void SaveSettings(BroadcastDescription &broadcast);
	void LoadSettings();
    void _SaveStreamKeyToAFAuthManager(std::string streamKey);

	QIcon GetPlaceholder() { return thumbPlaceholder; }
	void SetPlaceholder(const QIcon &icon) { thumbPlaceholder = icon; }

	QString selectedBroadcast;
	bool autostart, autostop;
	bool valid = false;
	YoutubeApiWrappers *apiYouTube;
	WorkerThread *workerThread = nullptr;
	bool broadcastReady = false;
	QString thumbnailFile;
	QIcon thumbPlaceholder;
};
