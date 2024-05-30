#pragma once
#include "MainFrame/CMainBaseWidget.h"
#include "CoreModel/Browser/CCefManager.h"

namespace Ui {
    class AFQBorderPopupBaseWidget;
}

class AFQBorderPopupBaseWidget final : public AFCQMainBaseWidget
{
    Q_OBJECT
#pragma region class initializer, destructor
public:
    explicit AFQBorderPopupBaseWidget(QWidget* parent = nullptr, Qt::WindowFlags flag = Qt::FramelessWindowHint);
    ~AFQBorderPopupBaseWidget();

signals:
    void qsignalCloseWidget(QString uuid);
#pragma endregion class initializer, destructor

#pragma region public func
public:
    void AddWidget(QWidget* widget);
    void AddCustomWidget(QCefWidget* widget);
    void SetBlockType(const char* type, int typeNum);
    void SetToDockValue(bool toDock) { m_bToDock = toDock; };
    void SetUrl(const std::string& url);
    QWidget* GetWidgetByName(const QString& name);
    void SetIsCustom(bool custom) { m_bCustom = custom; };
#pragma endregion public func

#pragma region protected func
protected:
    void closeEvent(QCloseEvent* event) override;
#pragma endregion protected func

#pragma region private member var
private:
    Ui::AFQBorderPopupBaseWidget* ui;
    
    bool m_bToDock = false;
    bool m_bCustom = false;

    QCefWidget* m_qCustomBrowserWidget = nullptr;

    const char* m_cBlockType = "";
    int m_iBlockType = -1;
#pragma endregion private member var
};

