#include "CSceneSelectDialog.h"

#include "qt-wrapper.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Source/CSource.h"

AFQSceneSelectDialog::AFQSceneSelectDialog(QWidget* parent) : 
	AFQRoundedDialogBase(parent),
	ui(new Ui::AFQSceneSelectDialog)
{
	ui->setupUi(this);

    NoFocusListTempDelegate* delegate = new NoFocusListTempDelegate(this);
    ui->sceneListWidget->setItemDelegate(delegate);

	AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

	OBSScene curScene = sceneContext.GetCurrOBSScene();

	SceneItemVector& sceneItemVector = sceneContext.GetSceneItemVector();

    int  sceneCount = 0;
    auto iter = sceneItemVector.begin();
    for (; iter != sceneItemVector.end(); ++iter) {
        AFQSceneListItem* item = (*iter);
        if (!item)
            continue;

        if (item->GetScene() == curScene)
            continue;

        OBSSource sceneSource = obs_scene_get_source(item->GetScene());
        const char* name = obs_source_get_name(sceneSource);
        ui->sceneListWidget->addItem(QT_UTF8(name));
        sceneCount++;
    }

    ui->sceneListWidget->adjustSize();

    if(sceneCount > 0)
        ui->sceneListWidget->setCurrentRow(0);

    connect(ui->closeButton, &QPushButton::clicked, 
            this, &AFQSceneSelectDialog::qSlotCloseButtonClicked);

    connect(ui->buttonBox, &QDialogButtonBox::clicked, 
            this, &AFQSceneSelectDialog::qSlotButtonBoxClicked);

}

AFQSceneSelectDialog::~AFQSceneSelectDialog()
{
}

void AFQSceneSelectDialog::qSlotButtonBoxClicked(QAbstractButton* button)
{
    QDialogButtonBox::ButtonRole val = ui->buttonBox->buttonRole(button);

    if (val == QDialogButtonBox::AcceptRole) {
        QListWidgetItem* item = ui->sceneListWidget->currentItem();
        if (!item)
            return;

        m_sourceName = item->text();
    }
}

void AFQSceneSelectDialog::qSlotCloseButtonClicked()
{
    close();
}