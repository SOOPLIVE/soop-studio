#pragma once

#include "obs.hpp"

#include <map>
#include <string>

#include "CoreModel/Browser/CCefManager.h"
#include "UIComponent/CTopBaseWindow.h"
#include "UIComponent/CBorderPopupBaseWidget.h"

namespace Ui {
    class AFQBorderPopupBaseWidget;
};

class AFQCefPopupDialog : public AFTTopBaseDialog
{
    Q_OBJECT

    enum CefPopupType {
        CefPopupType_None = -1,
        CefPopupType_KBOGraphic_All,
        CefPopupType_KBOGraphic_Score,
        CefPopupType_KBOGraphic_Stadium,
        CefPopupType_KBOGraphic_Player,
        CefPopupType_KBOGraphic_Livetext,
        CefPopupType_CommerceGoal,
        CefPopupType_CommerceRank,
        CefPopupType_Football_Graphic_All,
        CefPopupType_Football_Graphic_Player,
        CefPopupType_Football_Graphic_Change,
        CefPopupType_Football_Graphic_Score,
        CefPopupType_Football_Graphic_Livetext,
        //
        CefPopupType_End
    };
#pragma region class initializer, destructor
public:
    explicit AFQCefPopupDialog(QWidget* parent,
                               obs_source_t* source,
                               Qt::WindowFlags flag = Qt::WindowFlags(),
                               bool widthResizable = true,
                               bool heightResizable = true);
    ~AFQCefPopupDialog();

#pragma endregion class initializer, destructor

#pragma region QT Field

public slots:
    void qslotDataReceivedFromBrowser(const QCefQuery& query);
    void qslotCloseCustom();

#pragma endregion QT Field

#pragma region public member func
public:
    CefPopupType GetIndexCefPopup(const char* id);

    void SetUrl(const std::string& url);
    void ReloadCefWidget();
    void ExecuteScript(const std::string& script);
    QWidget* GetWidgetByName(const QString& name);

#pragma endregion public member func

#pragma region protected, private func
protected:
    virtual void closeEvent(QCloseEvent* event) override;

private:
    void _RegisterSource(obs_source_t* source);
    OBSSource _GetSource();

    bool _Initialize();
    void _ApplyMoveInAllArea(QObject* applyWidget);

    void _ParseKBOGraphic(const QCefQuery& query);
    void _ParseFootballGraphic(const QCefQuery& query);

    std::string GetCefPopupURL(CefPopupType type);
#pragma endregion protected, private func

#pragma region public member var

#pragma endregion public member var

#pragma region private member var
private:
    Ui::AFQBorderPopupBaseWidget* ui;

    std::string m_sourceId;
    CefPopupType m_sourceType = CefPopupType::CefPopupType_None;
    OBSWeakSource m_weakSource = nullptr;
    QCefWidget* m_pCustomBrowserWidget = nullptr;

#pragma endregion private member var
};
