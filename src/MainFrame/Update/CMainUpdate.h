#pragma once

#include "MainFrame/CMainFrame.h"
#include "Common/StudioDefine.h"
#include <QDialog>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QLabel>

#define MACOS_SOOPSTUDIO_VERSION	"1.1.4"

#define UPDATE_FOLDER_BASE "SOOPStudio"
#define STUDIO_BIN "bin"
#define STUDIO_64BIT "64bit"
#define UPDATE_LAUNCHER_TAR_NAME "update.tar.gz"
#define UPDATE_LAUNCHER_NAME "SOOPUpdaterLauncher.exe"
#define UPDATE_TIME_FILE "UpdateTime_soop.txt"

class CMainUpdate : public QObject
{
#pragma region QT Field
	Q_OBJECT
public:
	CMainUpdate(QWidget* parent, QNetworkAccessManager* networkManager);
	~CMainUpdate();

private slots:

#ifdef _WIN32
	void qslotHandleUpdateHashResponse();
	void qslotHandleUpdateHashDownResponse();
#endif

#ifdef __APPLE__
	void qslotDownloadDMGFinished(QNetworkReply* reply);
#endif

signals:
	void updatePathChanged(const QString& newPath);

#pragma endregion QT Field


#pragma region public func
public:
#ifdef _WIN32
	void FetchServerUpdateTime();
	void UpdaterCheck();

	void SetUpdatePath(const QString& path);
#endif

#ifdef __APPLE__
	bool CheckForUpdates();
#endif
#pragma endregion public func


#pragma region private func
private:
	void _EnsureUpdaterNotRunning();
	void terminateProcess(const QString& processName);
#ifdef _WIN32
	void _DownloadUpdater(const QUrl& downloadUrl, const QString& savePath);
	void _UpdateFlow();
	bool _DecompressGZFile(const QString& sourcePath, const QString& destPath);
	bool _DecompressTarFile(const QString& sourcePath, const QString& destPath);
	void _CompareUpdateTimeWithServer(const QString& serverUpdateTime);
	QString _CalculateFileHash(const QString& filePath);

#endif

#ifdef __APPLE__
	void _InstallApplication(const QString& filePath);
	void _DownloadUpdateDMG(const QString& url);
    void onDownloadFinished(QNetworkReply* reply);
#endif
#pragma endregion private func


#pragma region private member var
private:
#ifdef __APPLE__
	QString m_currentVersion = MACOS_SOOPSTUDIO_VERSION;
	QString m_downloadUrl;
#endif

	QNetworkAccessManager* m_pNetworkManager = nullptr;

	int  m_updateType = 0;

	QString m_serverUpdateTime;
	QString m_updateTimeFilePath;
	QString m_localUpdateHashStr;
	QString m_serverUpdatehash;
	
	QString m_updateBase;
	QString UPDATE_BASE() const { return m_updateBase; }

#pragma endregion private member var
};
