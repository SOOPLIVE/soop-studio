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

public:
    void AFBlockButtonWidgetInit(int type, QString key);
    int BlockButtonType() { return m_blockButtonType; }
    bool IsOnLabelVisible();
    void SetOnLabelVisible(bool visible);

protected:
    bool event(QEvent* e) override;

private:
    Ui::AFBlockButtonWidget* ui = nullptr;

    bool m_checkHover = false;
    int m_blockButtonType = -1;
};

#endif // CBLOCKBUTTONWIDGET_H