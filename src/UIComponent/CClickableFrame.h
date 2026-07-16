#pragma once

#include <QFrame>
#include <QMouseEvent>
#include <QWidget>

class AFQClickableFrame : public QFrame
{
    Q_OBJECT
public:
    explicit AFQClickableFrame(QWidget* parent = nullptr);

signals:
    void qsignalFrameClicked();

protected:
    virtual void mousePressEvent(QMouseEvent* event) override;

};

