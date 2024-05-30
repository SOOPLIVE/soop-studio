#ifndef CBLOCKBUTTONWIDGET_H
#define CBLOCKBUTTONWIDGET_H

#include <QWidget>
#include <QPointer>
#include <QTimer>

namespace Ui {
class AFBlockButtonWidget;
}

class AFBlockButtonWidget : public QWidget
{
#pragma region QT Field
    Q_OBJECT
public:
    explicit AFBlockButtonWidget(QWidget* parent = nullptr);
    ~AFBlockButtonWidget();

public slots:
    void qslotButtonPressedTriggered();
    //void qslotButtonReleasedTriggered();
signals:
    void qsignalBlockButtonClicked(bool);
    void qsignalShowTooltip(int);
    void qsignalHideTooltip();
#pragma endregion QT Field

#pragma region public func
public:
    void AFBlockButtonWidgetInit(int type, const char* chartype);
    int BlockButtonType() { return m_BlockButtonType; }
    bool IsOnLabelVisible();
    void SetOnLabelVisible(bool visible);
#pragma endregion public func

#pragma region protected func
protected:
    bool event(QEvent* e) override;
#pragma endregion protected func

#pragma region private func
private:
#pragma endregion private func

#pragma region private member var
private:
    Ui::AFBlockButtonWidget* ui;

    bool m_bCheckHover = false;
    int m_BlockButtonType = -1;
    const char* m_cBlockType = "";
#pragma endregion private member var
};

#endif // CBLOCKBUTTONWIDGET_H
