#pragma once

#include <QWidget>


#include "ViewModel/Display/DisplayMisc.h"

// Forward
struct obs_display;
class QScreen;
class AFExpressOBSDisplay;


class AFQTDisplay : public QWidget
{
#pragma region QT Field, CTOR/DTOR
 Q_OBJECT

public:
    AFQTDisplay(QWidget* parent = nullptr,
                Qt::WindowFlags flags = Qt::WindowFlags());
    ~AFQTDisplay();

public slots:

private slots:
    void qslotWindowVisible(bool visible);
    void qslotScreenChagned(QScreen*);

signals:
    void qsignalDisplayCreated(AFQTDisplay* window);
    void qsignalDisplayResized();

#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void moveEvent(QMoveEvent* event) override;
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual bool nativeEvent(const QByteArray& eventType, void* message,
                             qintptr* result) override;

    virtual QPaintEngine* paintEngine() const override;


    QColor GetDisplayBackgroundColor() const;
    void SetDisplayBackgroundColor(const QColor& color);

    obs_display* GetDisplay() const;
    //

    void OnMove();
    void OnDisplayChange();
    //
#pragma endregion public func

#pragma region private func
#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
private:
    AFExpressOBSDisplay*        m_pViewModel = nullptr;            // 1:1 -> View:ViewModel

    uint32_t                    m_bgColor = GREY_COLOR_BACKGROUND;
#pragma endregion private member var
};