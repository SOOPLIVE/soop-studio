#pragma once

#include <QEvent>
#include <Qlabel>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

class AFQElidedSlideLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(int offset READ GetOffset WRITE SetOffset)

public:
    AFQElidedSlideLabel(QWidget* parent = nullptr);

    int  GetOffset();
    void SetOffset(int offset);

    void SetMultiLine(bool multiLine);

    void updateText(const QString& text);   // used BoroadInfo label Title

    void SetElidedAnimation(bool set);

public slots:
    void qslotHoverButton(QString id);
    void qslotHoverLabel();
    void qslotHoverWidth(int maxWidth);
    void qslotLeaveButton();
    void qslotFinishedAnimation();
    void qslotTextChanged();

signals:
    void qsignalMouseClick();
    void qsignalHoverEnter();
    void qsignalHoverLeave();
    void qsignalTextChanged();
    void qsignalTextMultiLineChanged(bool);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* e) override;

private:
    void startAnimation();
    void startAnimationWithWidth(int maxWidth);
    void finishAnimation();
    void repeatAnimation();

    void refreshMultiLineText();

private:
    QPropertyAnimation* m_pAnimation = nullptr;

    bool m_isHoverd = false;
    int m_fullTextWidth = 0;
    int m_offset = 0;
    int m_speed = 70;


    // only use broadInfo Dock
    bool m_multiLine = false;
    bool m_twoLine = false;

    QString m_multiLine1;
    QString m_multiLine2;
    QString m_prevText;
    QString m_originText;
};