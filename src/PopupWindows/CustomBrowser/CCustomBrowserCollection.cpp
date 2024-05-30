#include "CCustomBrowserCollection.h"
#include "ui_custom-browser-collection.h"

#include <Application/CApplication.h>
#include <json11.hpp>
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "include/qt-wrapper.h"

void AFQCustomList::qslotTextChanged()
{
    if (!m_qNameLineEdit->text().isEmpty() && !m_qUrlLineEdit->text().isEmpty())
    {
        disconnect(m_qNameLineEdit, &QLineEdit::textEdited,
            this, &AFQCustomList::qslotTextChanged);

        disconnect(m_qUrlLineEdit, &QLineEdit::textEdited,
            this, &AFQCustomList::qslotTextChanged);
        emit qsignalAllEditFilled();
    }
}

void AFQCustomList::CustomListInit(bool newList)
{
    auto& localeTextManager = AFLocaleTextManager::GetSingletonInstance();

    m_qNameLineEdit = new AFQRememberLineEdit(this);
    m_qNameLineEdit->setFixedSize(QSize(200, 40));

    m_qUrlLineEdit = new AFQRememberLineEdit(this);
    m_qUrlLineEdit->setFixedHeight(40);

    if (newList)
    {
        m_sUuid = "";
        connect(m_qNameLineEdit, &QLineEdit::textEdited,
            this, &AFQCustomList::qslotTextChanged);

        connect(m_qUrlLineEdit, &QLineEdit::textEdited,
            this, &AFQCustomList::qslotTextChanged);
    }

    m_qClearButton = new QPushButton(this);
    m_qClearButton->setFixedSize(66, 40);
    m_qClearButton->setEnabled(false);
    m_qClearButton->setObjectName("pushButton_ClearCustomBrowser");

    m_qClearButton->setText(QTStr(localeTextManager.Str("Clear")));
    connect(m_qClearButton, &QPushButton::clicked,
        this, &AFQCustomList::qsignalClear);

    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(10);

    hLayout->addWidget(m_qNameLineEdit);
    hLayout->addWidget(m_qUrlLineEdit);
    hLayout->addWidget(m_qClearButton);

    setLayout(hLayout);
}

void AFQCustomList::InsertCustomList(QString name, QString path, QString uuid)
{
    m_qNameLineEdit->setText(name);
    m_qUrlLineEdit->setText(path);
    m_sUuid = uuid;
}

void AFQCustomList::SetDeleteButtonEnable(bool enable)
{
    if (m_qClearButton)
        m_qClearButton->setEnabled(enable);
}

QString AFQCustomList::GetName()
{
    return m_qNameLineEdit->text();
}

QString AFQCustomList::GetUrl()
{
    return m_qUrlLineEdit->text();
}

AFQCustomBrowserCollection::AFQCustomBrowserCollection(QWidget *parent) :
    AFCQMainBaseWidget(parent),
    ui(new Ui::AFQCustomBrowserCollection)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);    
    setAttribute(Qt::WA_TranslucentBackground);

    setAttribute(Qt::WA_DeleteOnClose);

}

AFQCustomBrowserCollection::~AFQCustomBrowserCollection()
{
    delete ui;
}

void AFQCustomBrowserCollection::qslotDeleteCustomBrowser()
{
    QWidget* senderwidget = reinterpret_cast<QWidget*>(sender());
    senderwidget->close();
    delete senderwidget;
    senderwidget = nullptr;
}

void AFQCustomBrowserCollection::qslotTextFilled()
{
    m_qNewLineWidget->SetDeleteButtonEnable(true);
    _AddNewCustomBrowser();
}

void AFQCustomBrowserCollection::qslotApplyTriggered()
{
    QList<QString> deleted = _SaveCustomBrowserList();
    App()->GetMainView()->GetMainWindow()->ReloadCustomBrowserList(m_qInfoVec,
                                                                   deleted);
    App()->GetMainView()->ReloadCustomBrowserMenu();
}

void AFQCustomBrowserCollection::ReloadCustomBrowserList(
    QVector<AFQCustomBrowserCollection::CustomBrowserInfo> vec)
{
    QList<AFQCustomList*> customList = findChildren<AFQCustomList*>();
    for (AFQCustomList* custom : customList)
    {
        
    }
}

void AFQCustomBrowserCollection::CustomBrowserCollectionInit(
    QVector<AFQCustomBrowserCollection::CustomBrowserInfo> vec)
{
    setWindowTitle(QTStr("Basic.MainMenu.Addon.CustomBrowserDocks"));

    connect(ui->pushButton_Close, &QPushButton::clicked,
        this, &AFQCustomBrowserCollection::close);
    connect(ui->pushButton_Cancel, &QPushButton::clicked,
        this, &AFQCustomBrowserCollection::close);
    connect(ui->pushButton_Apply, &QPushButton::clicked,
        this, &AFQCustomBrowserCollection::qslotApplyTriggered);

    _LoadCustomBrowserList(vec);
}

void AFQCustomBrowserCollection::_AddNewCustomBrowser()
{
    AFQCustomList* newList = new AFQCustomList(this);
    newList->CustomListInit(true);
    connect(newList, &AFQCustomList::qsignalAllEditFilled, 
        this, &AFQCustomBrowserCollection::qslotTextFilled);
    connect(newList, &AFQCustomList::qsignalClear,
        this, &AFQCustomBrowserCollection::qslotDeleteCustomBrowser);
    ui->widget_Contents->layout()->addWidget(newList);
    m_qNewLineWidget = newList;
}

void AFQCustomBrowserCollection::_LoadCustomBrowser(QString name, QString path, QString uuid)
{
    AFQCustomList* newList = new AFQCustomList(this);
    newList->CustomListInit(false);
    newList->SetDeleteButtonEnable(true);
    connect(newList, &AFQCustomList::qsignalAllEditFilled,
        this, &AFQCustomBrowserCollection::qslotTextFilled);
    connect(newList, &AFQCustomList::qsignalClear,
        this, &AFQCustomBrowserCollection::qslotDeleteCustomBrowser);
    ui->widget_Contents->layout()->addWidget(newList);

    newList->InsertCustomList(name, path, uuid);

}

void AFQCustomBrowserCollection::_LoadCustomBrowserList(
    QVector<AFQCustomBrowserCollection::CustomBrowserInfo> vec)
{
    m_LoadedCustomBrowser = 0;
    m_qInfoVec = vec;

    for (AFQCustomBrowserCollection::CustomBrowserInfo info : m_qInfoVec)
    {
        _LoadCustomBrowser(info.Name, info.Url, info.Uuid);
        m_LoadedCustomBrowser++;
    }

    _AddNewCustomBrowser();
}

QList<QString> AFQCustomBrowserCollection::_SaveCustomBrowserList()
{
    QList<QString> retVal;

    QList<AFQCustomList*> customList = findChildren<AFQCustomList*>();

    QVector<AFQCustomBrowserCollection::CustomBrowserInfo> newVec;

    for (AFQCustomList* custom : customList)
    {
        if (custom == m_qNewLineWidget)
            continue;

        AFQCustomBrowserCollection::CustomBrowserInfo info;

        info.Name = custom->GetName();
        info.Url = custom->GetUrl();

        QString uuid = custom->GetUuid();
        if (uuid == "")
        {
            QString uuid = QUuid::createUuid().toString();
            uuid.replace(QRegularExpression("[{}-]"), "");
            info.Uuid = uuid;
            info.newCustom = true;
            custom->InsertCustomList(info.Name, info.Url, info.Uuid);
        }
        else
        {
            info.Uuid = custom->GetUuid();
        }

        newVec.push_back(info);
    }

    for (AFQCustomBrowserCollection::CustomBrowserInfo info: m_qInfoVec)
    {
        bool notfound = true;
        for (AFQCustomBrowserCollection::CustomBrowserInfo newInfo: m_qInfoVec) {
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

    m_qInfoVec = newVec;

    return retVal;
}
