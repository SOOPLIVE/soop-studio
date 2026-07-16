#ifndef CHOTKEYSETTINGAREAWIDGET_H
#define CHOTKEYSETTINGAREAWIDGET_H

#include <QWidget>
#include <memory>
#include <string>
#include <QFormLayout>
#include <util/util.hpp>
#include <QPushButton>

#include <obs.hpp>

#include "CSettingUtils.h"

class AFHotkeyWidget;

typedef  QPair<QString, int>    HOTKEY_SECTION_INFO;
typedef  std::pair<bool, QPointer<AFHotkeyWidget>>  HOTKEY_WIDGET_PAIR;

namespace Ui {
class AFQHotkeySettingAreaWidget;
}

class AFQHotkeySettingAreaWidget : public QWidget
{
    Q_OBJECT
#pragma region class initializer, destructor
public:
    explicit AFQHotkeySettingAreaWidget(QWidget* parent = nullptr);
    ~AFQHotkeySettingAreaWidget();
#pragma endregion class initializer, destructor

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

#pragma region QT Field, CTOR/DTOR
public slots:
    void qslotHotkeyChanged();
    void qslotHotkeyFocusChanged();
    void qslotHotkeyAreaScrollChanged();

private slots:
    bool qslotScanDuplicateHotkeys(QFormLayout* layout);
    void qslotHotkeySearchByName(const QString text);
    void qslotHotkeySearchByKey(obs_key_combination_t combo);
    void qslotHotkeyNameResetClicked();
    void qslotHotkeyComboResetClicked();
    void qslotMoveToPosition();
    void qslotCheckScrollValue(int value);

signals:
    void qsignalHotkeyChanged();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void LoadHotkeysSettings(obs_hotkey_id ignoreKey = OBS_INVALID_HOTKEY_ID);
    void ReloadHotkeys(obs_hotkey_id ignoreKey = OBS_INVALID_HOTKEY_ID);
    void SaveHotkeysSettings();
    void ClearHotkeyValues();
    void UpdateFocusBehaviorComboBox();
    void SetVerticalScrollBarPosition();

    inline const QIcon& GetHotkeyConflictIcon() const
    {
        return m_hotkeyConflictIcon;
    }

    void SetHotkeysDataChangedVal(bool changed) { m_hotkeysDataChanged = changed; };
    bool HotkeysDataChanged() { return m_hotkeysDataChanged; };

#pragma endregion public func

#pragma region private func
private:
    void SearchHotkeys(const QString& text,
                       obs_key_combination_t filterCombo);


    void _RefreshHotkeyArea();
    void _AdjustHotkeySectionScroll();
    void _MakeHotkeySectionButtons();

#pragma endregion private func


#pragma region public member var
public:
#pragma endregion public member var

#pragma region private member var
private:
    Ui::AFQHotkeySettingAreaWidget* ui;

    OBSSignal m_hotkeyRegisteredOBSSignal;
    OBSSignal m_hotkeyUnregisteredOBSSignal;

    bool m_loading = false;
    bool m_hotkeysDataChanged = false;
    bool m_hotkeysLoaded = false;
    bool m_sectionButtonClicked = false;
    int  m_areaTrace = 0;

    std::vector<HOTKEY_WIDGET_PAIR> m_hotkeyWigets;
    QList<HOTKEY_SECTION_INFO>      m_listSectionPosY;
    QMap<int, QPushButton*>         m_mapSectionButton;

    QIcon m_hotkeyConflictIcon;
#pragma endregion private member var

};

#endif // CHOTKEYSETTINGAREAWIDGET_H
