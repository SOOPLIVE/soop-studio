#ifndef CSCENETRANSITIONSDIALOG_H
#define CSCENETRANSITIONSDIALOG_H

#include <QDialog>

#include <obs.hpp>

#include "UIComponent/CTopBaseWindow.h"

namespace Ui {
class AFQSceneTransitionsDialog;
}

class AFQSceneTransitionsDialog : public AFTTopBaseDialog
{
    Q_OBJECT

public:
    explicit AFQSceneTransitionsDialog(QWidget *parent, 
                                       OBSSource curTransition, 
                                       int curDuration );
    ~AFQSceneTransitionsDialog();

private slots:
    void qslotAddTransition();
    void qslotRemoveTransition();
    void qslotMenuDotTransition();
    void qslotChangeTransition(int);
    void qslotChangeDuration(int);
    //void qslotOkButtonClicked();
    void qslotCloseButtonClicked();

    void qslotRenameTransition();
    void qslotShowProperties();

public:
    void        CreatePropertiesWindow(obs_source_t* source);
    void        SetWidgetsEnabled(bool enable);

#pragma region protected func
protected:
    void showEvent(QShowEvent* event) override;
#pragma endregion protected func

private:
    void        _SetCurTransitionUI(OBSSource curTransition, int curDuration);
    void        _AddTransition(const char* id);
    void        _RenameTransition(OBSSource transition);

private:
    Ui::AFQSceneTransitionsDialog*ui;
    OBSSource m_prevTransition;
};

#endif // CSCENETRANSEFFECTDIALOG_H
