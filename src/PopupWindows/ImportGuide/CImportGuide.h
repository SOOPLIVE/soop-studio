#ifndef CIMPORTGUIDE_H
#define CIMPORTGUIDE_H

#include <QDialog>
#include "CImportGuidePreset.h"

#include "UIComponent/CTopBaseWindow.h"

namespace Ui {
class AFQImportGuide;
}

class AFQImportGuide : public AFTTopBaseDialog
{
    Q_OBJECT

public:
    explicit AFQImportGuide(QWidget *parent = nullptr);
    ~AFQImportGuide();


signals:
    void qsignalLoadOBSDataTriggered();
    void qsignalLoadFreecShotDataTriggered();

public slots:
    void qslotFromFreecshotTriggered();
    void qslotFromFreecshotProfileTriggered();
    void qslotFromOBSProfileTriggered();
    void qslotFromOBSSceneTriggered();
    void qslotNewSettingTriggered();

    void qslotOBSProfileCopyTriggered();

    void LoadFromStudio2Json();

private slots:
    void _qslotApplyPresetTriggered();
    void _qslotSelfSettingTriggered();
    void _qslotSwitchPreset(bool left);
    void _qslotSetApplyOBSDataBtnActive();
    void _qslotSetApplyFreecShotDataBtnActive();
    void _qslotApplyOBSDataTriggered();
    void _qslotApplyFreecShotDataTriggered();

    // [ For Style Update ]
    void _qslotPressedFromFreecshotWidget();
    void _qslotReleasedFromFreecshotWidget();
    
    void _qslotPressedNewSettingWidget();
    void _qslotReleasedNewSettingWidget();

    void _qslotHoverEnterSelfSettingWidget();
    void _qslotHoverLeaveSelfSettingWidget();
    void _qslotPressedSelfSettingWidget();
    //

    void _qslotAddPresetTriggered();

public:
    void ImportGuideInit(bool addPreset = false);
    void ImportRecentGuideInit();
    std::string SelectedProfilePath() { return m_selectedProfilePath; };
    BroadPresetType GetPresetType() { return m_presetType; }
    const QVector<QPair<QString, QRectF>> GetSourcesGeometry(BroadPresetType presetType);
    int GetStartChoice() { return m_startChoice; }

private:
    bool GetProfileFilePath(const char* folderpath, const char* collection, QString& filePath);
    void _LoadOBSProfileData();
    void _LoadOBSSceneData();
    void _LoadFreecShotData();
    void _UpdateWidgetStyle(const QList<QWidget*>& widgets, const QMap<QString, QVariant>& properties);
    void _ApplyFreecShotSharedLogic(const QString& filePath, const std::string& collectionName);

private:
    Ui::AFQImportGuide *ui = nullptr;

    QString m_soopProfileDir = "";
    QString m_soopSceneDir = "";
    QString m_obsProfileDir = "";
    QString m_obsSceneDir = "";
    QString m_FreecShotProfileDir = "";

    std::string m_selectedProfilePath = "";
    int m_step = 0;
    BroadPresetType m_presetType = BroadPresetType::New;

    int m_startChoice = 0; // 1 : freecshot,  2 : new setting, 0 : default

    QTimer* m_pStepTimer = nullptr;
};

#endif // CIMPORTGUIDE_H
