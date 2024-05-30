#pragma once

#include <QEvent>
#include <Qlabel>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

class AFQElidedSlideLabel : public QLabel {
    Q_OBJECT
        Q_PROPERTY(int offset READ GetOffset WRITE SetOffset)
public:
    AFQElidedSlideLabel(QWidget* parent = nullptr);

    int  GetOffset();
    void SetOffset(int offset);

public slots:
    void qSlotHoverButton(QString id);
    void qSlotLeaveButton();
    void qSlotFinishedAnimation();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void startAnimation();
    void finishAnimation();
    void repeatAnimation();

private:
    QPropertyAnimation* m_animation = nullptr;

    bool    m_isHoverd = false;
    int     m_fullTextWidth = 0;
    int     m_offset = 0;
    int     m_speed = 70;
};