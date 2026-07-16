#ifndef CINSTANTVODSAVERWIDGET_H
#define CINSTANTVODSAVERWIDGET_H

#include <QWidget>
#include <QAbstractButton>

#include "UIComponent/CTopBaseWindow.h"

namespace Ui
{
    class AFQInstantVodSaverWidget;
}

class AFQInstantVodSaverWidget : public AFTTopBaseDialog 
{
#pragma region QT Field, CTOR/DTOR

    Q_OBJECT

public:
    explicit AFQInstantVodSaverWidget(QWidget* parent = nullptr, QString prevTitle = "");
    ~AFQInstantVodSaverWidget();

private slots:
    void _qslotLineEditChanged(bool focus);
    void _qslotClickedSaveButton();
    void _qslotClickedCancelButton();

signals:
    void qsignalCloseTriggered(int type);
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    bool VodSaveSuccess() { return m_vodSaveSuccess; };
#pragma endregion public func

#pragma region protected func
protected:
    void closeEvent(QCloseEvent* event) override;
#pragma endregion protected func

#pragma region private func
private:
    void _Init(QString prevTitle);
#pragma endregion private func

#pragma region private member var
private:
    Ui::AFQInstantVodSaverWidget* ui;

    QString m_previousTitle;
    bool    m_vodSaveSuccess = false;
#pragma endregion private member var
};
#endif // CINSTANTVODSAVERWIDGET_H
