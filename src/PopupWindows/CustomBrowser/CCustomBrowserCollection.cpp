#include "CCustomBrowserCollection.h"
#include "ui_custom-browser-collection.h"

#include <Application/CApplication.h>
#include <json11.hpp>
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "UIComponent/CBorderPopupBaseWidget.h"

void AFQCustomList::qslotTextChanged()
{
    if (!m_pNameLineEdit->text().isEmpty() && !m_pUrlLineEdit->text().isEmpty())
    {
        disconnect(m_pNameLineEdit, &QLineEdit::textEdited,
            this, &AFQCustomList::qslotTextChanged);

        disconnect(m_pUrlLineEdit, &QLineEdit::textEdited,
            this, &AFQCustomList::qslotTextChanged);
        emit qsignalAllEditFilled();
    }
}


void AFQCustomList::CustomListInit(bool newList)
{
    QWidget* checkBoxWidget = new QWidget(this);
    checkBoxWidget->setFixedWidth(52);
    QHBoxLayout* cLayout = new QHBoxLayout();
    cLayout->setContentsMargins(17, 0, 0, 0);

    m_showPopupCheckBox = new QCheckBox(checkBoxWidget);
    m_showPopupCheckBox->setText("");
    m_showPopupCheckBox->setChecked(false);
    cLayout->addWidget(m_showPopupCheckBox);
    checkBoxWidget->setLayout(cLayout);

    m_pNameLineEdit = new AFQBasicLineEdit(this);
    m_pNameLineEdit->setFixedSize(QSize(148, 40));

    m_pUrlLineEdit = new AFQBasicLineEdit(this);
    m_pUrlLineEdit->setFixedHeight(40);

    if (newList)
    {
        m_uuid = "";
        connect(m_pNameLineEdit, &QLineEdit::textEdited,
            this, &AFQCustomList::qslotTextChanged);

        connect(m_pUrlLineEdit, &QLineEdit::textEdited,
            this, &AFQCustomList::qslotTextChanged);
    }

    m_pClearButton = new QPushButton(this);
    m_pClearButton->setFixedSize(66, 40);
    m_pClearButton->setEnabled(false);
    m_pClearButton->setObjectName("pushButton_ClearCustomBrowser");

    m_pClearButton->setText(QTStr("Clear"));
    connect(m_pClearButton, &QPushButton::clicked,
        this, &AFQCustomList::qsignalClear);

    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(10);

    hLayout->addWidget(checkBoxWidget);
    hLayout->addWidget(m_pNameLineEdit);
    hLayout->addWidget(m_pUrlLineEdit);
    hLayout->addWidget(m_pClearButton);

    setLayout(hLayout);
}

void AFQCustomList::InsertCustomList(QString name, QString path, QString uuid, bool isOpen)
{
    if(m_pNameLineEdit)
        m_pNameLineEdit->setText(name);
    if (m_pUrlLineEdit)
        m_pUrlLineEdit->setText(path);
    if (m_showPopupCheckBox)
        m_showPopupCheckBox->setChecked(isOpen);
    m_uuid = uuid;
}

void AFQCustomList::SetDeleteButtonEnable(bool enable)
{
    if (m_pClearButton)
        m_pClearButton->setEnabled(enable);
}

QString AFQCustomList::GetName()
{
    return m_pNameLineEdit->text();
}

QString AFQCustomList::GetUrl()
{
    return m_pUrlLineEdit->text();
}

bool AFQCustomList::IsOpen()
{
    if (m_showPopupCheckBox)
        return m_showPopupCheckBox->isChecked();
    else
        return false;
}

AFQCustomBrowserCollection::AFQCustomBrowserCollection(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AFQCustomBrowserCollection)
{
    ui->setupUi(this);
}

AFQCustomBrowserCollection::~AFQCustomBrowserCollection()
{
    delete ui;
}


void AFQCustomBrowserCollection::qslotShowCheckChanged(bool checked)
{
    QCheckBox* senderWidget = qobject_cast<QCheckBox*>(sender());

    if (checked)
    {
        QList<AFQCustomList*> customList = findChildren<AFQCustomList*>();
        int count = 0;
        for (AFQCustomList* custom : customList)
        {
            if (custom && custom->IsOpen())
                count++;
        }

        if (count > 10)
        {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                "", QTStr("CustomBrowser.Open.Denied"));

            senderWidget->setChecked(false);
        }

    }
}

void AFQCustomBrowserCollection::qslotDeleteCustomBrowser()
{
    QWidget* senderwidget = reinterpret_cast<QWidget*>(sender());
    senderwidget->close();
    delete senderwidget;
    senderwidget = nullptr;
    if (m_loadedCustomBrowser == CUSTOM_MAX_COUNT)
        _AddNewCustomBrowser();
    m_loadedCustomBrowser--;
}

void AFQCustomBrowserCollection::qslotTextFilled()
{
    m_loadedCustomBrowser++;
    m_pNewLineWidget->SetDeleteButtonEnable(true);
    if (m_loadedCustomBrowser < CUSTOM_MAX_COUNT)
        _AddNewCustomBrowser();
    else
        m_pNewLineWidget = nullptr;
}

void AFQCustomBrowserCollection::qslotApplyTriggered()
{
    QList<QString> deleted = _SaveCustomBrowserList();
    MAIN_BLOCKMANAGER->ReloadCustomBrowserList(m_infoVec, deleted);
    MAINFRAME->ReloadCustomBrowserMenu();
}

void AFQCustomBrowserCollection::CustomBrowserCollectionInit(
    QVector<AFQCustomBrowserCollection::CustomBrowserInfo> vec)
{
    setWindowTitle(QTStr("Basic.MainMenu.Addon.CustomBrowserDocks"));

    connect(ui->pushButton_Cancel, &QPushButton::clicked,
        this, &AFQCustomBrowserCollection::close);
    connect(ui->pushButton_Apply, &QPushButton::clicked,
        this, &AFQCustomBrowserCollection::qslotApplyTriggered);

    _LoadCustomBrowserList(vec);
}

void AFQCustomBrowserCollection::closeEvent(QCloseEvent* event)
{
    emit qsignalCloseTriggered(ENUM_WINDOW_TYPE::CustomBrowserCollection);
}

void AFQCustomBrowserCollection::_AddNewCustomBrowser()
{
    AFQCustomList* newList = new AFQCustomList(this);

    newList->CustomListInit(true);
    connect(newList->getCheckBox(), &QCheckBox::toggled, this, &AFQCustomBrowserCollection::qslotShowCheckChanged);

    connect(newList, &AFQCustomList::qsignalAllEditFilled, 
        this, &AFQCustomBrowserCollection::qslotTextFilled);
    connect(newList, &AFQCustomList::qsignalClear,
        this, &AFQCustomBrowserCollection::qslotDeleteCustomBrowser);
    ui->widget_Contents->layout()->addWidget(newList);
    m_pNewLineWidget = newList;
}

void AFQCustomBrowserCollection::_LoadCustomBrowser(QString name, QString path, QString uuid, bool isOpen)
{
    AFQCustomList* newList = new AFQCustomList(this);
    newList->CustomListInit(false);

    newList->SetDeleteButtonEnable(true);
    connect(newList, &AFQCustomList::qsignalAllEditFilled,
        this, &AFQCustomBrowserCollection::qslotTextFilled);
    connect(newList, &AFQCustomList::qsignalClear,
        this, &AFQCustomBrowserCollection::qslotDeleteCustomBrowser);
    ui->widget_Contents->layout()->addWidget(newList);

    newList->InsertCustomList(name, path, uuid, isOpen);
}

void AFQCustomBrowserCollection::_LoadCustomBrowserList(
    QVector<AFQCustomBrowserCollection::CustomBrowserInfo> vec)
{
    m_loadedCustomBrowser = 0;
    m_infoVec = vec;

    for (AFQCustomBrowserCollection::CustomBrowserInfo info : m_infoVec)
    {
        _LoadCustomBrowser(info.Name, info.Url, info.Uuid, info.isOpen);
        m_loadedCustomBrowser++;
    }

    if(m_loadedCustomBrowser < 20)
        _AddNewCustomBrowser();


    _ConnectSignal();
}

QList<QString> AFQCustomBrowserCollection::_SaveCustomBrowserList()
{
    QList<QString> retVal;

    QList<AFQCustomList*> customList = findChildren<AFQCustomList*>();

    QVector<AFQCustomBrowserCollection::CustomBrowserInfo> newVec;

    for (AFQCustomList* custom : customList)
    {
        if(m_pNewLineWidget)
            if (custom == m_pNewLineWidget)
                continue;

        AFQCustomBrowserCollection::CustomBrowserInfo info;

        info.Name = custom->GetName();
        info.Url = custom->GetUrl();
        info.isOpen = custom->IsOpen();

        QString uuid = custom->GetUuid();
        if (uuid.isEmpty())
        {
            uuid = QUuid::createUuid().toString();
            uuid.replace(QRegularExpression("[{}-]"), "");
            info.Uuid = uuid;
            custom->InsertCustomList(info.Name, info.Url, info.Uuid, info.isOpen);
        }
        else
        {
            info.Uuid = custom->GetUuid();
        }

        newVec.push_back(info);
    }

    foreach (AFQCustomBrowserCollection::CustomBrowserInfo info, m_infoVec)
    {
        bool notfound = true;
        for (AFQCustomBrowserCollection::CustomBrowserInfo newInfo: newVec) {
            if (info.Uuid == newInfo.Uuid)
            {
                notfound = false;
                break;
            }

        }
        if (notfound)
        {
            retVal.append(info.Uuid);
        }
    }

    m_infoVec = newVec;

    return retVal;
}

void AFQCustomBrowserCollection::_ConnectSignal()
{
    QList<AFQCustomList*> customList = findChildren<AFQCustomList*>();

    for (AFQCustomList* custom : customList)
    {
        connect(custom->getCheckBox(), &QCheckBox::toggled, this, &AFQCustomBrowserCollection::qslotShowCheckChanged);
    }
}
