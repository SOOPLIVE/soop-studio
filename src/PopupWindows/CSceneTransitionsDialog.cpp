#include "CSceneTransitionsDialog.h"
#include "ui_scene-transitions-dialog.h"

#include <string>

#include "qt-wrappers.hpp"
#include "Application/CApplication.h"

#include "CoreModel/Source/CSource.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"
#include "UIComponent/CCustomMenu.h"

#include "UIComponent/CNameDialog.h"
#include "UIComponent/CMessageBox.h"

AFQSceneTransitionsDialog::AFQSceneTransitionsDialog(QWidget* parent,
                                                     OBSSource curTransition,
                                                     int curDuration) :
    AFTTopBaseDialog((QDialog*)parent),
    ui(new Ui::AFQSceneTransitionsDialog)
{
    ui->setupUi(this);
    
#ifdef __APPLE__
    setWindowTitle(QTStr("Basic.SceneTransitions"));
    ui->titleFrame->hide();
#endif

    connect(ui->closeButton, &QPushButton::clicked, 
            this, &AFQSceneTransitionsDialog::qslotCloseButtonClicked);

    //connect(ui->buttonBox->button(QDialogButtonBox::Close),
    //        &QPushButton::clicked, this, 
    //        &AFQSceneTransitionsDialog::qslotCloseButtonClicked);

    //connect(ui->buttonBox->button(QDialogButtonBox::Ok),
    //    &QPushButton::clicked, this,
    //    &AFQSceneTransitionsDialog::qslotOkButtonClicked);

    _SetCurTransitionUI(curTransition, curDuration);

}

AFQSceneTransitionsDialog::~AFQSceneTransitionsDialog()
{
    delete ui;
}

void AFQSceneTransitionsDialog::qslotAddTransition()
{
    bool foundConfigurableTransitions = false;
    AFQCustomMenu menu(this);
    size_t idx = 0;
    const char* id;

    while (obs_enum_transition_types(idx++, &id)) {
        if (obs_is_source_configurable(id)) {
            const char* name = obs_source_get_display_name(id);
            QAction* action = new QAction(name, this);

            connect(action, &QAction::triggered,
                [this, id]() { _AddTransition(id); });

            menu.addAction(action);
            foundConfigurableTransitions = true;
        }
    }

    if (foundConfigurableTransitions)
        menu.exec(QCursor::pos());
}

void AFQSceneTransitionsDialog::qslotRemoveTransition()
{
    int index = ui->comboTransitions->currentIndex();
    const OBSSource tr = ui->comboTransitions->itemData(index).value<OBSSource>();

    if (!tr || !obs_source_configurable(tr) || !AFSourceUtil::QueryRemoveSource(tr))
        return;

    int idx = ui->comboTransitions->findData(QVariant::fromValue<OBSSource>(tr));
    if (idx == -1)
        return;

    ui->comboTransitions->removeItem(idx);

    std::vector<OBSSource>& transitions = SCENE_CONTEXT.GetRefTransitions();
    std::vector<OBSSource>::iterator iter = transitions.begin();
    for (; iter != transitions.end(); ++iter) {
        if ((*iter) == tr) {
            transitions.erase(iter);
            break;
        }
    }

    //MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED);

}

void AFQSceneTransitionsDialog::qslotChangeTransition(int)
{
    const OBSSource transition = ui->comboTransitions->currentData().value<OBSSource>();

    if (m_prevTransition == transition)
        return;

    bool fixed = transition ? obs_transition_fixed(transition) : false;
    ui->labelDuration->setVisible(!fixed);
    ui->spinDuration->setVisible(!fixed);

    bool configurable = obs_source_configurable(transition);
    ui->removeButton->setEnabled(configurable);
    ui->dotButton->setEnabled(configurable);

    m_prevTransition = transition;

    //MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_TRANSITION_CHANGED);

    SCENE_CONTEXT.SetTransition(transition);
    SCENE_CONTEXT.SetCurTransition(transition);
}

void AFQSceneTransitionsDialog::qslotChangeDuration(int)
{
    const int duration = ui->spinDuration->value();
    SCENE_CONTEXT.SetCurDuration(duration);
}

void AFQSceneTransitionsDialog::qslotMenuDotTransition()
{
    AFQCustomMenu popup(this);

    QAction* actionRename = popup.addAction(Str("Rename"), this, 
                                    &AFQSceneTransitionsDialog::qslotRenameTransition);

    QAction* actionProps  = popup.addAction(Str("Properties"), this, 
                                    &AFQSceneTransitionsDialog::qslotShowProperties);

    popup.exec(QCursor::pos());

}

//void AFQSceneTransitionsDialog::qslotOkButtonClicked()
//{
//    int index = ui->comboTransitions->currentIndex();
//    const OBSSource transition = ui->comboTransitions->itemData(index).value<OBSSource>();
//    const int duration = ui->spinDuration->value();
//
//    SetTransition(transition);
//    SCENE_CONTEXT.SetCurTransition(transition);
//    SCENE_CONTEXT.SetCurDuration(duration);
//}

void AFQSceneTransitionsDialog::qslotCloseButtonClicked()
{
    close();
}

void AFQSceneTransitionsDialog::qslotRenameTransition()
{
    int index = ui->comboTransitions->currentIndex();
    const OBSSource transition = ui->comboTransitions->itemData(index).value<OBSSource>();

    _RenameTransition(transition);
}

void AFQSceneTransitionsDialog::qslotShowProperties()
{
    int index = ui->comboTransitions->currentIndex();
    const OBSSource transition = ui->comboTransitions->itemData(index).value<OBSSource>();

    CreatePropertiesWindow(transition);
}

void AFQSceneTransitionsDialog::CreatePropertiesWindow(obs_source_t* source)
{
    if (!obs_source_configurable(source))
        return;

    MAINFRAME->CreateSourceProperties(source);
}

void AFQSceneTransitionsDialog::SetWidgetsEnabled(bool enable)
{
    const OBSSource transition = ui->comboTransitions->currentData().value<OBSSource>();
    bool configurable = obs_source_configurable(transition);

    ui->comboTransitions->setEnabled(enable);
    //ui->spinDuration->setEnabled(enable);
    ui->addButton->setEnabled(enable);
    
    if (configurable) 
    {
        ui->removeButton->setEnabled(enable);
        ui->dotButton->setEnabled(enable);
    }
}

void AFQSceneTransitionsDialog::showEvent(QShowEvent* event)
{
    QRect midRect = MAIN_BLOCKMANAGER->GetMidGeometry(this->size());
    QRect adjustRect;
    MAIN_BLOCKMANAGER->AdjustPositionOutSideFullScreen(midRect, adjustRect);

    move(adjustRect.x(), adjustRect.y());
}

void AFQSceneTransitionsDialog::_SetCurTransitionUI(OBSSource curTransition, int curDuration)
{
    const char* curTransitionName = obs_source_get_name(curTransition);

    ui->spinDuration->setValue(curDuration);

    std::vector<OBSSource> transitions = SCENE_CONTEXT.GetTransitions();
    for (OBSSource& tr : transitions) {
        const char* name = "";
        if (!tr)
            continue;
        name = obs_source_get_name(tr);

        bool match = (name && strcmp(name, curTransitionName) == 0);
        if (!name || !*name)
            name = Str("None");

        ui->comboTransitions->addItem(QT_UTF8(obs_source_get_name(tr)), QVariant::fromValue(OBSSource(tr)));
    }

    connect(ui->dotButton, &QPushButton::clicked,
            this, &AFQSceneTransitionsDialog::qslotMenuDotTransition);

    connect(ui->addButton, &QPushButton::clicked,
            this, &AFQSceneTransitionsDialog::qslotAddTransition);

    connect(ui->removeButton, &QPushButton::clicked,
            this, &AFQSceneTransitionsDialog::qslotRemoveTransition);

    connect(ui->comboTransitions, &QComboBox::currentIndexChanged,
            this, &AFQSceneTransitionsDialog::qslotChangeTransition);

    connect(ui->spinDuration, &QSpinBox::valueChanged,
            this, &AFQSceneTransitionsDialog::qslotChangeDuration);
    
    // set ui
    int idx = ui->comboTransitions->findData(QVariant::fromValue<OBSSource>(curTransition));
    if (idx != -1) {
        ui->comboTransitions->blockSignals(true);
        ui->comboTransitions->setCurrentIndex(idx);
        ui->comboTransitions->blockSignals(false);
    }

    bool fixed = curTransition ? obs_transition_fixed(curTransition) : false;
    ui->labelDuration->setVisible(!fixed);
    ui->spinDuration->setVisible(!fixed);

    bool configurable = obs_source_configurable(curTransition);
    ui->removeButton->setEnabled(configurable);
    ui->dotButton->setEnabled(configurable);

}

void AFQSceneTransitionsDialog::_AddTransition(const char* id)
{
    std::string name;
    QString placeHolderText = QT_UTF8(obs_source_get_display_name(id));
    QString format = placeHolderText + " (%1)";
    obs_source_t* source = nullptr;
    int i = 1;

    while ((SCENE_CONTEXT.FindTransition(QT_TO_UTF8(placeHolderText)))) {
        placeHolderText = format.arg(++i);
    }

    bool accepted = AFQNameDialog::AskForName(this,
                                              Str("TransitionNameDlg.Title"),
                                              Str("TransitionNameDlg.Text"),
                                              name, placeHolderText);

    if (accepted) {
        if (name.empty()) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                            QT_UTF8(""), Str("NoNameEntered.Text"));
            _AddTransition(id);
            return;
        }
        source = SCENE_CONTEXT.FindTransition(name.c_str());
        if (source) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                            QT_UTF8(""), Str("NameExists.Text"));
            _AddTransition(id);
            return;
        }

        source = obs_source_create_private(id, name.c_str(), NULL);
        SCENE_CONTEXT.InitTransition(source);
        ui->comboTransitions->addItem(QT_UTF8(name.c_str()),
                                      QVariant::fromValue(OBSSource(source)));
        ui->comboTransitions->setCurrentIndex(ui->comboTransitions->count() - 1);
        CreatePropertiesWindow(source);
        obs_source_release(source);

        //MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED);

        SCENE_CONTEXT.AddTransition(source);
    }
}

void AFQSceneTransitionsDialog::_RenameTransition(OBSSource transition)
{
    std::string name;
    QString placeHolderText = QT_UTF8(obs_source_get_name(transition));
    obs_source_t* source = nullptr;

    bool accepted = AFQNameDialog::AskForName(this,
                            Str("TransitionNameDlg.Title"),
                            Str("TransitionNameDlg.Text"),
                            name, placeHolderText);

    if (!accepted)
        return;
    if (name.empty()) {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this, 
                            QT_UTF8(""), Str("NoNameEntered.Text"));
        _RenameTransition(transition);
        return;
    }

    source = SCENE_CONTEXT.FindTransition(name.c_str());
    if (source) {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                        QT_UTF8(""), Str("NameExists.Text"));

        _RenameTransition(transition);
        return;
    }

    obs_source_set_name(transition, name.c_str());
    int idx = ui->comboTransitions->findData(QVariant::fromValue(transition));
    if (idx != -1) {
        ui->comboTransitions->setItemText(idx, QT_UTF8(name.c_str()));
    }
}
