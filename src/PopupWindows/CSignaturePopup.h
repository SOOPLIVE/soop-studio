#ifndef CSIGNATUREPOPUP_H
#define CSIGNATUREPOPUP_H

#include <QWidget>
#include <QMetaEnum>
#include <QTimer>
#include <QMovie>
#include <QDragEnterEvent>
#include <QMimeData>

#include "Application/CApplication.h"
#include "UIComponent/CTopBaseWindow.h"
#include "UIComponent/CSavvyStyleButton.h"
#include "Common/StudioDefine.h"

#define SAVVY_MAX_BALLOON_COUNT 30000
#define SAVVY_CHECK_STATUS_TIME 3 * 1000
#define SAVVY_UPLOAD_TIME 3 * 1000
#define MAX_FILE_SIZE 5 * 1024 * 1024 //(5MB)

namespace Ui {
    class AFQSignaturePopup;
}

class AFQSavvyCamWidget : public AFQTDisplay
{
    Q_OBJECT

public:
    explicit AFQSavvyCamWidget(QWidget* parent = nullptr);
    ~AFQSavvyCamWidget() {};

signals:
    void qsignalMouseOnButton(int status);


protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;

private:
    bool m_mouseHoverOnButton = false;

};

class AFQDragDropImageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AFQDragDropImageWidget(QWidget* parent = nullptr);
    ~AFQDragDropImageWidget() {};

signals:
    void qsignalFileUploaded(QString filePath);

protected:
    virtual void dragEnterEvent(QDragEnterEvent* event) override;
    virtual void dropEvent(QDropEvent* event) override;
};


class AFQSignaturePopup : public AFTTopBaseDialog
{
    Q_OBJECT

public:
    explicit AFQSignaturePopup(bool reactionAble, QWidget *parent = nullptr);
    ~AFQSignaturePopup();

private slots:
    void qslotStartReaction();
    void qslotStartSignature();

    void qslotCreateSignatureFromUrlTriggered();
    void qslotReceiveSignatureJob(const QByteArray& responseData);
    void qslotStartSignatureStatusCheck(bool start);
    void qslotCheckSignatureStatus();
    void qslotReceiveSignatureStatus(const QByteArray& responseData);

    void qslotStartSignatureTriggered();

    void qslotUploadImageLoaded(bool success);
    void qslotUploadImageTriggered();
    void qslotUploadCreateTriggered();
    void qslotShowUploadedImage(const QString& filePath);

    void qslotSignatureStyleSelectTriggered();
    void qslotSignatureBackgroundSelectTriggered();
    void qslotSignatureBalloonEditing(const QString& text);
    void qslotSignatureCreateTriggered();

    void qslotSignatureCompletedTriggered();
    void qslotSignatureCamRecapture();

    void qslotSignatureDownloadTriggered();
    void qslotSignatureRestartTriggered();
    void qslotSignatureNavigateUploadUrl();

    void qslotVideoDeviceListChanged(int changedindex);
    void qslotScreenShot();
    void qslotSetScreenShotImage();
    void qslotAddDrawCallback();
    static void _DrawPreview(void* data, uint32_t cx, uint32_t cy);
    void qslotChangeCamImage(int mouseStatus); //default:normal, 1:hover, 2: clicked

    void qslotChangeSignatureCamPage(int index);
    void qslotCheckCurrentDeviceActive(bool start);
    void qslotTimerCheckCurrentDeviceActive();

signals:
    void qsignalSignatureJobComplete(bool success);
    void qsignalSignatureUpload(bool success);
    void qsignalCloseSignature(bool opened);

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    using properties_delete_t = decltype(&obs_properties_destroy);
    using properties_t =
        std::unique_ptr<obs_properties_t, decltype(&obs_properties_destroy)>;

    void _InitSignatureWidget();
    void _SetSignatureStartingPage();
    void _CreateLoadingMovie();
    void _StartLoadingMovie(bool create);//true: create signature, false: upload image
    void _StopLoadingMovie();
    void _SetSignatureBackground();
    void _SetSignatureStyle();

    bool isValidImageFile(const QString& filePath);

    void AddSource();
    void UpdateSourceComboProperties();

    void _SignatureFromUrl(QString path, bool isPC);
    QString _DefaultDownloadPath(bool isPC);
    bool _SelectedDownloadFolderPath(QString& path);
    QString _SelectedDownloadPath(bool isPC, QString path);

    void _SetDownloadedImages();

    void _ChangeImageUploadedButtonStyle(bool uploaded);
    void _ChangeCamUploadedButtonStyle(bool uploaded);

private:
    Ui::AFQSignaturePopup *ui;

    QString m_imagePath;
    QString m_savvyFolderPath;
    std::string m_userID;
    std::string m_selectedBackground;
    std::string m_selectedStyle;
    std::string m_balloonCount;
    std::string m_message;

    std::string m_jobID;

    QList<AFQSavvyStyleButton*> m_backGroundButtons;
    QList<AFQSavvyStyleButton*> m_styleButtons;

    OBSSource m_obsInputSource;
    OBSSource m_obsImageSource;

    QVariant m_curData;
    QTimer   m_timerCurrentDeivceActive;

    QMovie* m_pLoadingMovie = nullptr;

    bool m_isSignature = false;
    bool m_reactionAble = false;
};

#endif // CSIGNATUREPOPUP_H
