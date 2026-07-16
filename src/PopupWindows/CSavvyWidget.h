#ifndef CSAVVYWIDGET_H
#define CSAVVYWIDGET_H

#include <QWidget>
#include <QMetaEnum>
#include <QTimer>
#include <QMovie>
#include <QDragEnterEvent>
#include <QMimeData>

#include "Application/CApplication.h"
#include "UIComponent/CTopBaseWindow.h"
#include "UIComponent/CSavvyStyleButton.h"

namespace Ui {
class AFQSavvyWidget;
}

class AFQSavvyWidget : public AFTTopBaseDialog
{
    Q_OBJECT


public:
    explicit AFQSavvyWidget(QWidget *parent = nullptr);
    ~AFQSavvyWidget();

public slots:
    void qslotStartSignature();
    void qslotStartReaction();

signals:
    void qsignalSavvyClosedTriggered(bool opened);
    void qsignalStartSignatureTriggered(bool reactionAble);
    void qsignalStartReactionTriggered();

protected:
    virtual void closeEvent(QCloseEvent* event) override;

private:
    void _InitSelectSavvy();

private:
    Ui::AFQSavvyWidget *ui;
};

#endif // CSAVVYWIDGET_H
