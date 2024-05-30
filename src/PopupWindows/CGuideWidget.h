#ifndef CPROGRAMGUIDEWIDGET_H
#define CPROGRAMGUIDEWIDGET_H

//#include <QWidget>
#include "MainFrame/CMainBaseWidget.h"
#include "UIComponent/CBasicHoverWidget.h"

namespace Ui {
    class AFQProgramGuideItemWidget;
}

class AFQProgramGuideItemWidget : public AFQHoverWidget
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

public:
    enum class GuideItemState
    {
        None = 0,
        Hidden,
        Visible,
        Completed
    };

    explicit AFQProgramGuideItemWidget(QWidget* parent, int stepNum);
    ~AFQProgramGuideItemWidget();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void SetGuideItemState(const GuideItemState state);
    void SetContentText(const QString missionTxt, const QString completedTxt = "");
    void SetIconPath(const char* hiddenIcon, const char* visibleIcon, const char* completedIcon = "");
    void SetBGColor(const QString activeColor, const QString inactiveColor);
    int GetGuideItem() { return m_nStepNum; };
#pragma endregion public func

#pragma region private member var
private:
    Ui::AFQProgramGuideItemWidget* ui;

    int m_nStepNum = 0;
    QString m_strMissionText = "";
    QString m_strCompletedText = "";
    GuideItemState m_eState = GuideItemState::None;

    const char* m_strHiddenIconPath = "";
    const char* m_strVisibleIconPath = "";
    const char* m_strCompletedIconPath = "";

    QString m_strActiveBGColor = "";
    QString m_strInactiveBGColor = "";
#pragma endregion private member var
};

namespace Ui {
    class AFQProgramGuideWidget;
}

class AFQProgramGuideWidget : public AFCQMainBaseWidget
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

public:
    explicit AFQProgramGuideWidget(QWidget* parent = nullptr, Qt::WindowFlags flag = Qt::FramelessWindowHint);
    ~AFQProgramGuideWidget();

public slots:
    void qslotMouseClicked();

signals:
    void qsignalMissionTrigger(int missionNum);
    void qsignalCloseMission();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void NextMission(int index);
    void StartGuide();
#pragma endregion public func

#pragma region private func
private:
    void _InitGuide();
    void _ClearGuideItems();
    void _SetGuideAllCleared();
#pragma endregion private func

#pragma region protected func
protected:
#pragma endregion protected func

#pragma region private member var
private:
    Ui::AFQProgramGuideWidget* ui;
    std::vector<AFQProgramGuideItemWidget*> m_guideItems;

    int m_nCurrentStep = 0;
    QString m_strGuideTitleText = "";
#pragma endregion private member var
};

#endif // CPROGRAMGUIDEWIDGET_H