#include "CMainUpdate.h"

#include <QDir>

CMainUpdate::CMainUpdate(QWidget* parent, QNetworkAccessManager* networkManager) :
    QObject(parent),
    m_pNetworkManager(networkManager)
{
}

CMainUpdate::~CMainUpdate()
{
    m_pNetworkManager = nullptr;
}

#ifdef _WIN32
void CMainUpdate::qslotHandleUpdateHashResponse()
{
    auto reply = qobject_cast<QNetworkReply*>(sender());
    if (reply->error() == QNetworkReply::NoError) {
        m_serverUpdatehash = reply->readAll().trimmed();

        if (QString::compare(m_localUpdateHashStr, m_serverUpdatehash, Qt::CaseInsensitive) == 0)
        {
            qDebug() << "Update not downloaded!";
        }
        else
        {
            QString workingDir = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
            QString bitDir = QFileInfo(workingDir).absolutePath();
            QString binDir = QFileInfo(bitDir).absolutePath();
            QString updaterDir = QFileInfo(binDir).absolutePath() + "\\update";
            QString savePath = QDir::toNativeSeparators(updaterDir + "\\" UPDATE_LAUNCHER_TAR_NAME);

            QUrl downloadUrl(QString::fromStdString(UPDATE_SERVER_URL) + "/" + UPDATE_BASE() + "/update/" +UPDATE_LAUNCHER_TAR_NAME);
            _DownloadUpdater(downloadUrl, savePath);
        }
    }
    else {
        qDebug() << "Failed to fetch server update Hash:" << reply->errorString();
    }
    reply->deleteLater();

}

void CMainUpdate::qslotHandleUpdateHashDownResponse()
{
    auto reply = qobject_cast<QNetworkReply*>(sender());
    if (reply->error() == QNetworkReply::NoError) {
        m_serverUpdatehash = reply->readAll().trimmed();

        QFile file(m_updateTimeFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << m_serverUpdatehash;
            file.close();
            qDebug() << "UpdateTime.txt has been updated successfully at" << m_serverUpdatehash;
        }
        else {
            qDebug() << "Failed to open UpdateTime.txt for writing at" << m_serverUpdatehash;
        }
    }
    else {
        qDebug() << "Failed to fetch server update Hash:" << reply->errorString();
    }
    reply->deleteLater();

    QString savePath = QCoreApplication::applicationDirPath() + "/" UPDATE_LAUNCHER_TAR_NAME;
    QUrl downloadUrl(QString::fromStdString(UPDATE_SERVER_URL) + "/" + UPDATE_BASE() + "/" STUDIO_BIN "/" STUDIO_64BIT "/" UPDATE_LAUNCHER_TAR_NAME);
    _DownloadUpdater(downloadUrl, savePath);
}
#endif

#ifdef __APPLE__
void CMainUpdate::qslotDownloadDMGFinished(QNetworkReply* reply)
{
    if (reply->error()) {
        reply->deleteLater();
        return;
    }

    QString filePath = QDir::tempPath() + "/SOOPStudio.dmg";
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        reply->deleteLater();
        return;
    }

    while (!reply->atEnd()) {
        QByteArray data = reply->read(4096);
        file.write(data);
    }

    file.close();

    _InstallApplication(filePath);
    reply->deleteLater();
}
#endif

#ifdef _WIN32
void CMainUpdate::SetUpdatePath(const QString& path)
{
    if (!path.isEmpty()) {
        m_updateBase = path;
       
        blog(LOG_INFO, "CMainUpdate: Path [%s]", m_updateBase.toUtf8().constData());
        emit updatePathChanged(m_updateBase);
    }
}

void CMainUpdate::FetchServerUpdateTime()
{
    blog(LOG_INFO, "FetchServerUpdateTime starting...");
    m_updateType = 0;

    if (!m_pNetworkManager) {
        blog(LOG_ERROR, "Network Manager is null.");
        return;
    }

    QString urlStr = QString::fromStdString(UPDATE_SERVER_URL) + "/" + UPDATE_BASE() + "/" UPDATE_TIME_FILE;
    QUrl url(urlStr);

    auto performRequest = [&](QUrl targetUrl) -> QByteArray {
        QNetworkRequest request(targetUrl);
        request.setRawHeader("Cache-Control", "no-cache");
        request.setRawHeader("Pragma", "no-cache");

        QNetworkReply* reply = m_pNetworkManager->get(request);
        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);

        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

        timeoutTimer.start(5000);
        loop.exec();

        QByteArray result;
        if (timeoutTimer.isActive()) {
            timeoutTimer.stop();
            if (reply->error() == QNetworkReply::NoError) {
                result = reply->readAll().trimmed();
            }
            else {
                blog(LOG_INFO, "Request Error [%s]: %s",
                    targetUrl.scheme().toStdString().c_str(),
                    reply->errorString().toStdString().c_str());
            }
        }
        else {
            blog(LOG_INFO, "Request Timeout [%s]", targetUrl.scheme().toStdString().c_str());
            if (reply->isRunning()) reply->abort();
        }
        reply->deleteLater();
        return result;
        };

    m_serverUpdateTime = performRequest(url);

    if (m_serverUpdateTime.isEmpty()) {
        blog(LOG_INFO, "HTTPS failed. Falling back to HTTP");
        url.setScheme("http");
        m_serverUpdateTime = performRequest(url);

        if (m_serverUpdateTime.isEmpty() && m_updateBase != "STUDIO") {
            blog(LOG_INFO, "[Path Fallback] HTTPS failed on custom path. Switching to DEFAULT_PATH");

            SetUpdatePath("STUDIO");

            FetchServerUpdateTime();
            return;
        }

        if (!m_serverUpdateTime.isEmpty()) {
            blog(LOG_INFO, "HTTP Fallback Success. Forcing update to fix SSL libraries.");           
            m_updateType = 2;
            _UpdateFlow();
            return;
        }
    }

    if (!m_serverUpdateTime.isEmpty()) {
        _CompareUpdateTimeWithServer(m_serverUpdateTime);
    }
    else {
        App()->SetNoUpdate(true);
    }
}
void CMainUpdate::UpdaterCheck()
{
    if (!m_pNetworkManager)
        return;

    _EnsureUpdaterNotRunning();

    m_updateTimeFilePath = GetLocalAppDataPath() + "/" UPDATE_FOLDER_BASE "/update/updateVer.txt";

    QFile file(m_updateTimeFilePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        m_localUpdateHashStr = in.readAll().trimmed();
        file.close();
    }

    QUrl url(QString::fromStdString(UPDATE_SERVER_URL) + "/" + UPDATE_BASE() + "/update/updateVer.txt");
    QNetworkRequest request(url);

    request.setRawHeader("Cache-Control", "no-cache");
    request.setRawHeader("Pragma", "no-cache");

    auto reply = m_pNetworkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &CMainUpdate::qslotHandleUpdateHashResponse);
}
#endif

#ifdef __APPLE__
bool CMainUpdate::CheckForUpdates()
{
    QUrl infoUrl("");
    QNetworkRequest request(infoUrl);

    QNetworkAccessManager networkManager;
    QNetworkReply* reply = networkManager.get(request);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();

        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        if (!jsonDoc.isObject()) {
            reply->deleteLater();
            return false;
        }

        QJsonObject jsonObj = jsonDoc.object();
        QString latestVersion = jsonObj["latestVersion"].toString();
        QString downloadUrl = jsonObj["downloadUrl"].toString();

        if (latestVersion != m_currentVersion) {
            g_bIsDownloadingUpdate = true;
            _DownloadUpdateDMG(downloadUrl);
            return true;
        }
        else {
            return false;
        }
    }
    else {
        qDebug() << "Update check failed:" << reply->errorString();
    }
    reply->deleteLater();

    return false;
}
#endif

void CMainUpdate::terminateProcess(const QString& processName) {
    QProcess process;
    process.start("tasklist", QStringList() << "/fi" << QString("IMAGENAME eq %1").arg(processName));
    process.waitForFinished();
    QString output = process.readAllStandardOutput();

    if (output.contains(processName, Qt::CaseInsensitive)) {
        QProcess::execute("taskkill", QStringList() << "/f" << "/im" << processName);
        qDebug() << processName << "was running and terminated.";
    }
    else {
        qDebug() << processName << "is not running.";
    }
}

void CMainUpdate::_EnsureUpdaterNotRunning()
{
    terminateProcess("updater.exe");
    terminateProcess("SOOPUpdaterLauncher.exe");
}

#ifdef _WIN32
void CMainUpdate::_DownloadUpdater(const QUrl& downloadUrl, const QString& savePath)
{
    if (!m_pNetworkManager)
        return;

    qDebug() << "Download URL: " << downloadUrl.toString();

    QNetworkRequest request(downloadUrl);
    QNetworkReply* reply = m_pNetworkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, savePath]() {
        if (reply->error() == QNetworkReply::NoError) {
            // Ensure the directory exists
            QDir().mkpath(QFileInfo(savePath).absolutePath());

            QFile file(savePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();


                QFileInfo fileInfo(savePath);
                QString fileExtension = fileInfo.completeSuffix();

                if (fileExtension == "tar.gz") {

                    QString gzFilePath = fileInfo.absoluteFilePath();
                    QString tarFilePath = fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".tar";

                    if (_DecompressGZFile(gzFilePath, tarFilePath)) {
                        if (_DecompressTarFile(tarFilePath, fileInfo.absolutePath())) {
                            QFile::remove(gzFilePath);
                        }

                        if (QFile::remove(tarFilePath)) {
                            qDebug() << "File removed successfully:" << tarFilePath;
                        }
                        else {
                            qDebug() << "Failed to remove file:" << tarFilePath;
                            QFile::remove(tarFilePath);
                        }

                    }
                }
                else if (fileExtension == "gz") {
                    QString decompressedFilePath = fileInfo.absolutePath() + "/" + fileInfo.baseName();

                    if (_DecompressGZFile(savePath, decompressedFilePath)) {
                        QFile::remove(savePath);
                        qDebug() << "Decompressed .gz file successfully to:" << decompressedFilePath;
                    }
                    else {
                        qDebug() << "Failed to decompress .gz file.";
                    }
                }

                QFile hashFile(m_updateTimeFilePath);
                if (hashFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream out(&hashFile);
                    out << m_serverUpdatehash;
                    hashFile.close();
                    qDebug() << "UpdateTime.txt has been updated successfully at" << m_serverUpdatehash;
                }
                else {
                    qDebug() << "Failed to open UpdateTime.txt for writing at" << m_serverUpdatehash;
                }

                qDebug() << "Updater downloaded successfully to:" << savePath;
            }
            else {
                qDebug() << "Failed to save the downloaded updater.";
            }
        }
        else {
            qDebug() << "Failed to download updater:" << reply->errorString();
        }
        reply->deleteLater();
        });
}

void CMainUpdate::_UpdateFlow()
{
    QString FoldermPath = GetLocalAppDataPath() + "/SOOPStudio/update";
    QString programPath = GetLocalAppDataPath() + "/SOOPStudio/update/SOOPUpdaterLauncher.exe";

    switch (m_updateType)
    {
    case 1:
    {
        QStringList arguments;        

        QProcess* process = new QProcess();
        process->setWorkingDirectory(FoldermPath);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [](int exitCode, QProcess::ExitStatus exitStatus) {
                Q_UNUSED(exitCode);
                Q_UNUSED(exitStatus);
               
            });

        process->start(programPath, arguments);
        process->waitForStarted();


        std::string folderPath = FoldermPath.toStdString();
        std::string exePath = programPath.toStdString();

        blog(LOG_INFO, "FoldermPath %s", folderPath.c_str());
        blog(LOG_INFO, "programPath %s", exePath.c_str());


        if (process->waitForStarted()) {
            QEventLoop loop;
            QTimer timer;
            timer.setSingleShot(true);

            connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
            timer.start(20000);
            loop.exec();

            qDebug() << "Waited for 20 seconds after SOOPUpdaterLauncher.exe started.";
        }
        else {
            qDebug() << "Failed to start SOOPUpdaterLauncher.exe.";
        }

        QCoreApplication::quit();
        process->deleteLater();
    }
    break;
    case 2:
    {
        QStringList arguments;

        arguments << "--no-ui";

        QProcess* process = new QProcess();
        process->setWorkingDirectory(FoldermPath);
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [](int exitCode, QProcess::ExitStatus exitStatus) {
                Q_UNUSED(exitCode);
                Q_UNUSED(exitStatus);
            });

        process->start(programPath, arguments);
        process->waitForStarted();


        std::string folderPath = FoldermPath.toStdString();
        std::string exePath = programPath.toStdString();

        blog(LOG_INFO, "FoldermPath [--no-ui] %s", folderPath.c_str());
        blog(LOG_INFO, "programPath [--no-ui] %s", exePath.c_str());

        if (process->waitForStarted()) {
            QEventLoop loop;
            QTimer timer;
            timer.setSingleShot(true);

            connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
            timer.start(20000);
            loop.exec();

            qDebug() << "Waited for 20 seconds after SOOPUpdaterLauncher.exe started.";
        }
        else {
            qDebug() << "Failed to start SOOPUpdaterLauncher.exe.";
        }

        QCoreApplication::quit();
        process->deleteLater();
    }
    break;
    case 3:
    {

    }
    break;

    case 0:
    default:
    {
        App()->SetNoUpdate(true);
    }
    break;
    }
}

bool CMainUpdate::_DecompressGZFile(const QString& sourcePath, const QString& destPath)
{
    gzFile inFileZ = gzopen(sourcePath.toLocal8Bit().constData(), "rb");
    if (!inFileZ) {
        qDebug() << "Cannot open gzip file:" << sourcePath;
        return false;
    }

    QFile outFile(destPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        gzclose(inFileZ);
        qDebug() << "Cannot open output file:" << destPath;
        return false;
    }

    const int bufferSize = 4096;
    char buffer[bufferSize];
    int numRead = 0;
    while ((numRead = gzread(inFileZ, buffer, bufferSize)) > 0) {
        outFile.write(buffer, numRead);
    }

    gzclose(inFileZ);
    outFile.close();

    qDebug() << "Decompressed file successfully from:" << sourcePath << "to:" << destPath;
    return true;
}

bool CMainUpdate::_DecompressTarFile(const QString& sourcePath, const QString& destPath)
{
    QProcess process;
    process.setWorkingDirectory(destPath);
    process.start("tar", QStringList() << "-xf" << sourcePath);
    //QFile::remove(sourcePath); // Clean up tar file
    if (!process.waitForFinished(-1)) {
        qDebug() << "tar process failed:" << process.errorString();
    }
    else if (process.exitCode() != 0) {
        qDebug() << "tar exited with code" << process.exitCode();
        qDebug() << "Standard Error:" << process.readAllStandardError();
    }
    return process.exitCode() == 0;
}

void CMainUpdate::_CompareUpdateTimeWithServer(const QString& serverUpdateTime)
{
    blog(LOG_INFO, "compareUpdateTimeWithServer serverUpdateTime [%s]", serverUpdateTime.toUtf8().constData());
        
    m_updateTimeFilePath = GetLocalAppDataPath() + "/SOOPStudio/UpdateTime_soop.txt";

    QFile file(m_updateTimeFilePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_updateType = 1;
        _UpdateFlow();
        return;
    }

    QTextStream in(&file);
    QString localUpdateTimeStr = in.readLine().trimmed();
    file.close();

    blog(LOG_INFO, "compareUpdateTimeWithServer localUpdateTime [%s]", localUpdateTimeStr.toUtf8().constData());

    if (localUpdateTimeStr == m_serverUpdateTime) {
        m_updateType = 0;
        _UpdateFlow();
        return;
    }

    QRegularExpression regex("^(\\d{4}/\\d{1,2}/\\d{1,2} \\d{1,2}:\\d{1,2})(?:\\s(\\d+))?$");
    QRegularExpressionMatch localMatch = regex.match(localUpdateTimeStr);
    QRegularExpressionMatch serverMatch = regex.match(m_serverUpdateTime);

    QDateTime localUpdateTime = QDateTime::fromString(localMatch.captured(1), "yyyy/M/d H:m");
    QDateTime serverUpdateTimeParsed = QDateTime::fromString(serverMatch.captured(1), "yyyy/M/d H:m");

    if (localUpdateTime != serverUpdateTimeParsed) {
        m_updateType = 1;
        _UpdateFlow();
        return;
    }

    if (serverMatch.captured(2) == "99") {
        m_updateType = 3;
    }
    else if (!serverMatch.captured(2).isEmpty()) {
        m_updateType = 2;
    }
    else {
        m_updateType = 0;
    }
    _UpdateFlow();
}

QString CMainUpdate::_CalculateFileHash(const QString& filePath)
{
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (hash.addData(&file)) {
            return hash.result().toHex();
        }
    }
    return QString();
}
#endif

#ifdef __APPLE__
void CMainUpdate::_DownloadUpdateDMG(const QString& url) {

    QDialog* progressDialog = new QDialog(nullptr);
    progressDialog->setWindowTitle(tr("SOOPStudio"));
    progressDialog->setModal(true);
    progressDialog->resize(400, 150);

    QVBoxLayout* layout = new QVBoxLayout(progressDialog);

    QLabel* label = new QLabel(tr("Downloading SOOPStudio"), progressDialog);
    layout->addWidget(label);

    QProgressBar* progressBar = new QProgressBar(progressDialog);
    progressBar->setRange(0, 100);
    layout->addWidget(progressBar);

    QLabel* percentageLabel = new QLabel("0%", progressDialog);
    percentageLabel->setAlignment(Qt::AlignRight);
    layout->addWidget(percentageLabel);

    progressDialog->show();

    QNetworkRequest request(url);
    QNetworkReply* reply = m_pNetworkManager->get(request);

    connect(reply, &QNetworkReply::downloadProgress, [progressBar, percentageLabel](qint64 bytesReceived, qint64 bytesTotal) {
        if (bytesTotal > 0) {
            int progress = static_cast<int>((100.0 * bytesReceived) / bytesTotal);
            progressBar->setValue(progress);
            percentageLabel->setText(QString::number(progress) + "%");
        }
     });

    connect(reply, &QNetworkReply::finished, this, [this, reply, progressDialog]() {
        progressDialog->close();
        progressDialog->deleteLater();
        onDownloadFinished(reply);
    });

    connect(reply, &QNetworkReply::errorOccurred, [progressDialog, reply](QNetworkReply::NetworkError code) {
        Q_UNUSED(code);
        QMessageBox::critical(progressDialog, tr("Download Error"), reply->errorString());
        progressDialog->close();
    });
}


void CMainUpdate::onDownloadFinished(QNetworkReply* reply) {
    if (reply->error()) {
        reply->deleteLater();
        return;
    }

    QString filePath = QDir::tempPath() + "/SOOPStudio.dmg";
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        reply->deleteLater();
        return;
    }

    while (!reply->atEnd()) {
        QByteArray data = reply->read(4096);
        file.write(data);
    }

    file.close();

    _InstallApplication(filePath);
    reply->deleteLater();
}

void CMainUpdate::_InstallApplication(const QString& filePath) {

    QString command = "open \"" + filePath + "\"";
    system(command.toStdString().c_str());

    qApp->quit();
}
#endif
