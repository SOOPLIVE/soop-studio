#ifndef CCUSTOMBROWSERCOLLECTION_H
#define CCUSTOMBROWSERCOLLECTION_H

#include <QWidget>
#include <QMap>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>

#include "MainFrame/CMainBaseWidget.h"
#include "UIComponent/CLengthAwareCustomLineEdit.h"

#define CUSTOMBROWSERINFO AFQCustomBrowserCollection::CustomBrowserInfo 
#define CUSTOM_MAX_COUNT 20

namespace Ui {
class AFQCustomBrowserCollection;
}


class AFQCustomList : public QWidget {

#pragma region QT Field
    Q_OBJECT
public:
    AFQCustomList(QWidget* parent = nullptr) : QWidget(parent) {}

public slots:
    void qslotTextChanged();

signals:
    void qsignalClear();
    void qsignalAllEditFilled();
#pragma endregion QT Field


#pragma region public func
public:
    void CustomListInit(bool newList);
    void InsertCustomList(QString name, QString path, QString uuid, bool isOpen);

    void SetDeleteButtonEnable(bool enable);
    void FirstCreated() { m_new = true; };

    QString GetName();
    QString GetUrl();
    bool    IsOpen();
    QString GetUuid() { return m_uuid; };

    QCheckBox* getCheckBox() { return m_showPopupCheckBox; };

#pragma endregion public func


#pragma region private member var
private:
    AFQBasicLineEdit*   m_pNameLineEdit = nullptr;
    AFQBasicLineEdit*   m_pUrlLineEdit = nullptr;
    QPushButton*           m_pClearButton = nullptr;
    QCheckBox*          m_showPopupCheckBox = nullptr;

    QString m_uuid;
    
    bool m_new = false;
#pragma endregion private member var
};


class AFQCustomBrowserCollection : public QWidget
{
#pragma region QT Field
    Q_OBJECT

public:
    explicit AFQCustomBrowserCollection(QWidget* parent = nullptr);
    ~AFQCustomBrowserCollection();

    struct CustomBrowserInfo
    {
        QString Name = "";
        QString Url = "";
        QString Uuid = "";
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        bool isDock = true;
        bool isOpen = true;
    };

public slots:
    void qslotShowCheckChanged(bool checked);

private slots:
    void qslotDeleteCustomBrowser();
    void qslotTextFilled();
    void qslotApplyTriggered();

signals:
    void qsignalCloseTriggered(int type);

#pragma endregion QT Field

#pragma region public func
public:
    void CustomBrowserCollectionInit(
        QVector<AFQCustomBrowserCollection::CustomBrowserInfo> vec);

#pragma endregion public func

#pragma region protected func
protected:
    void closeEvent(QCloseEvent* event) override;
#pragma endregion protected func

#pragma region private func
private:
    void _AddNewCustomBrowser();
    void _LoadCustomBrowser(QString name, QString path, QString uuid, bool isOpen);
    void _LoadCustomBrowserList(
        QVector<AFQCustomBrowserCollection::CustomBrowserInfo> vec);
    QList<QString> _SaveCustomBrowserList();
    void _ConnectSignal();
#pragma endregion private func

#pragma region private member var
private:
    Ui::AFQCustomBrowserCollection* ui;

    AFQCustomList* m_pNewLineWidget = nullptr;
    QVector<AFQCustomBrowserCollection::CustomBrowserInfo> m_infoVec;

    int m_loadedCustomBrowser = 0;

#pragma endregion private member var
};

#endif // CCUSTOMBROWSERCOLLECTION_H
