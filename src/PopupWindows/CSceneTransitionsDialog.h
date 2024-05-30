#ifndef CSCENETRANSITIONSDIALOG_H
#define CSCENETRANSITIONSDIALOG_H

#include <QDialog>

#include <obs.hpp>

#include "UIComponent/CRoundedDialogBase.h"

namespace Ui {
class AFQSceneTransitionsDialog;
}

class AFQSceneTransitionsDialog : public AFQRoundedDialogBase
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
    void qslotOkButtonClicked();
    void qslotCloseButtonClicked();

    void qslotRenameTransition();
    void qslotShowProperties();

public:
    void        CreatePropertiesWindow(obs_source_t* source);

private:
    void        _SetCurTransitionUI(OBSSource curTransition, int curDuration);
    void        _AddTransition(const char* id);
    void        _RenameTransition(OBSSource transition);

private:
    Ui::AFQSceneTransitionsDialog*ui;
    OBSSource m_prevTransition;
};

#endif // CSCENETRANSEFFECTDIALOG_H
