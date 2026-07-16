#ifndef CBREAKTIME_H
#define CBREAKTIME_H

#include <QDialog>

#include "UIComponent/CQtDisplay.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "obs.hpp"
#include <QStringList>
#include <QPointer>
#include <vector> 
#include <memory>
#include <QListWidgetItem>
#include <QVBoxLayout>

#include <UIComponent/CTopBaseWindow.h>

enum {
    RoleRestricted = Qt::UserRole + 7001,
    RoleRestrictedId = Qt::UserRole + 7002
};

struct SceneInfo {
    QString name;
    bool restricted = false;
    QString restrictedId;
};

namespace Ui {
class AFBreaktime;
}

struct BreaktimeScenePreviewCtx {
    OBSWeakSourceAutoRelease weak;
};

struct ScenePreviewSlot {
    QPointer<AFQTDisplay> display;
    OBSSource source = nullptr;
    std::unique_ptr<BreaktimeScenePreviewCtx> ctx;
    ScenePreviewSlot() = default;
    ScenePreviewSlot(ScenePreviewSlot&&) noexcept = default;
    ScenePreviewSlot& operator=(ScenePreviewSlot&&) noexcept = default;
    ScenePreviewSlot(const ScenePreviewSlot&) = delete;
    ScenePreviewSlot& operator=(const ScenePreviewSlot&) = delete;
};

class AFBreaktime : public AFTTopBaseDialog
{
    Q_OBJECT

public:
    explicit AFBreaktime(QWidget *parent = nullptr, int dlgPage = 1);
    ~AFBreaktime();

    int GetSelectedMinutes() const { return m_selectedMinutes; }  //1: 1분, 2: 2분 ~ 10: 10분
    int GetSelectedBgm() const { return m_selectedBgm; }

    QString GetCurrentSceneName() const;
    bool    IsDefaultSceneSelected() const;
    bool    IsMessageChecked() const;
    bool    IsSoundMuted() const;
    bool    IsSoundMuted2() const;
    int     GetSliderVolume() const;
    int     GetSliderVolume2() const;


    void addBreaktimeScene(const QString& sceneName);
    void setBreaktimeScenes(const QStringList& sceneNames);
    void selectSceneByName(const QString& name);
  
    QString GetMessageText() const;
    QString GetEditableMessageText() const;

    void SetInitialState(int minutes, int bgm, bool msgChecked, bool soundMuted, int volume, const QString& sceneName, const QString& messageText);
    void SetInitialState_Playing(int nRemainingtime, int bgm, bool msgChecked, bool soundMuted, int volume, const QString& sceneName, const QString& messageText);

    void UpdateRemainingTimeLabel(int remainingSec);

    bool SaveBreaktimeConfig();
    bool SaveBreaktimeConfig2();
private slots:
    void qslotCloseButtonTriggered();
    void qslotCancelButtonTriggered();
    void qslotApplyButtonTriggered();
    void qslotStopButtonTriggered();
    void qslotBreaktimeChanged(int index); 
    void qslotBgmChanged(int index);
    void qslotCheckBoxBreaktimeToggled(bool checked);
    void qslotSoundButtonToggled(bool checked);
    void qslotVolumeChanged(int value);
    void qslotPreviewButtonToggled(bool checked);
    void qslotBreaktimeTicked(int remainingSec, int totalSec);
    void qslotBreaktimeFinished();
    void qslotSceneItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
private:
    Ui::AFBreaktime *ui;
    
    int m_selectedMinutes = 3;
          
    QStringList m_sceneNames;
    void rebuildScenePages(); 
    OBSSource _getSourceForSceneName(const QString& name) const;
    static void _ScenePageRender(void* data, uint32_t cx, uint32_t cy);

    std::vector<ScenePreviewSlot> m_scenePreviewSlots;
    std::vector<ScenePreviewSlot> m_scenePreviewSlots2;

    void _buildPlayingScene(const QString& sceneName);

    // BGM (comboBox_breaktime_bgm)
    // 0 = Not Used
    // 1 = Upbeat tempo
    // 2 = Medium tempo
    // 3 = Synthesizer
    // 4 = Calm tempo
    int m_selectedBgm = 0;     // Default BGM (0= Not Used)

    QString m_defaultMessage;
    QString m_editableMessage;
    QString m_disabledMessage;

    void applyMessageState(bool checked, bool focusOnEdit = true);
    void applyDisabledState();

    bool m_isPreviewPlaying = false;

    int m_lastValidSceneRow = 0;
    QHash<QString, QString> m_restrictedScenes;

    bool _isSceneRestricted(OBSScene scene, QString* hitId);
    void _markSceneItemRestricted(QListWidgetItem* item, const QString& hitId);

    void _showRestrictedAlert(const QString& id);
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

signals:
    void signalForcePreviewToggle(bool checked);
};

#endif // CBREAKTIME_H
