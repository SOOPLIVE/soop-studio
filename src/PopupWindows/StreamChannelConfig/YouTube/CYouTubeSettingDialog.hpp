#pragma once

#include <QDialog>
#include <QString>
#include <QThread>

#include "ui_stream-channel-youtube-setting.h"
#include "ViewModel/Auth/YouTube/youtube-api-wrappers.hpp"

#include "UIComponent/CTopBaseWindow.h"

class WorkerThread : public QThread {
	Q_OBJECT

public:
	WorkerThread(YoutubeApiWrappers *api) : QThread(), m_pApiYouTube(api) {}

	void stop() { m_pending = false; }

protected:
	YoutubeApiWrappers* m_pApiYouTube;
	bool m_pending = true;

public slots:
	void run() override;

signals:
	void ready();
	void new_item(const QString &title, const QString &dateTimeString,
				  const QString &broadcast, const QString &status,
				  bool astart, bool astop);
	void failed();
};

class AFQYouTubeSettingDialog : public AFTTopBaseDialog {
	Q_OBJECT
	Q_PROPERTY(QIcon m_thumbPlaceholder READ GetPlaceholder WRITE SetPlaceholder DESIGNABLE true)

	std::unique_ptr<Ui::AFQYouTubeSettingDialog> ui;

signals:
	void ok(const QString &broadcast_id, const QString &stream_id,
			const QString &key, bool autostart, bool autostop,
			bool start_now);
	void qsignalMakeChat(QString chat_id, std::string api_id);

private slots:
	void qslotCancel();

protected:
	void showEvent(QShowEvent *event) override;
	void UpdateOkButtonStatus();

	bool CreateEventAction(YoutubeApiWrappers *api,
						   BroadcastDescription &broadcast,
						   StreamDescription &stream, bool stream_later,
						   bool ready_broadcast = false);
	bool ChooseAnEventAction(YoutubeApiWrappers *api, StreamDescription &stream);

	void ShowErrorDialog(QWidget *parent, QString text);

public:
	explicit AFQYouTubeSettingDialog(QWidget *parent, AFAuth *auth, bool m_broadcastReady);
	virtual ~AFQYouTubeSettingDialog() override;

	bool Valid() { return m_valid; };

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

	QIcon GetPlaceholder() { return m_thumbPlaceholder; }
	void SetPlaceholder(const QIcon &icon) { m_thumbPlaceholder = icon; }

	QString m_selectedBroadcast;
	bool m_autostart, m_autostop;
	bool m_valid = false;
	YoutubeApiWrappers* m_pApiYouTube;
	WorkerThread* m_pWorkerThread = nullptr;
	bool m_broadcastReady = false;
	QString m_thumbnailFile;
	QIcon m_thumbPlaceholder;
};
