#ifndef CSYSTEMALERTWIDGET_H
#define CSYSTEMALERTWIDGET_H

#include <QWidget>
#include <QPointer>
#include <QTimer>

namespace Ui {
    class AFQSystemAlert;
}

class AFQSystemAlert : public QWidget
{
#pragma region QT Field
    Q_OBJECT
#pragma endregion QT Field

#pragma region class initializer, destructor
public:
    explicit AFQSystemAlert(QWidget* parent = nullptr,  
                            const QString& alertText = "",
                            const QString& channelID = "",
                            bool showInCorner = true,
                            int mainFrameWidth = 0);
    ~AFQSystemAlert();
#pragma endregion class initializer, destructor

#pragma region protected func
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
#pragma endregion protected func

#pragma region private member var
private:
    Ui::AFQSystemAlert* ui;
    QPointer<QTimer> m_qTimer;
#pragma endregion private member var
};

#endif // CSYSTEMALERTWIDGET_H