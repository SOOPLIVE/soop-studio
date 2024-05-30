#ifndef CCUSTOMBROWSERCOLLECTION_H
#define CCUSTOMBROWSERCOLLECTION_H

#include <QWidget>
#include <QMap>
#include <QLineEdit>
#include <QPushButton>

#include "MainFrame/CMainBaseWidget.h"

namespace Ui {
class AFQCustomBrowserCollection;
}

class AFQRememberLineEdit : public QLineEdit {

#pragma region QT Field
    Q_OBJECT
public:
    AFQRememberLineEdit(QWidget* parent = nullptr) : QLineEdit(parent) {}

#pragma endregion QT Field

#pragma region protected func
protected:
    void focusInEvent(QFocusEvent* event) override {
        previousText = this->text();
        QLineEdit::focusInEvent(event);
    }

    void focusOutEvent(QFocusEvent* event) override {
        if (this->text().isEmpty() && !previousText.isEmpty()) 
            this->setText(previousText);

        QLineEdit::focusOutEvent(event);
    }
#pragma endregion protected func

#pragma region private member var
private:
    QString previousText;
#pragma endregion private member var
};

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
    void InsertCustomList(QString name, QString path, QString uuid);

    void SetDeleteButtonEnable(bool enable);
    void FirstCreated() { m_bNew = true; };

    QString GetName();
    QString GetUrl();
    QString GetUuid() { return m_sUuid; };

#pragma endregion public func


#pragma region private member var
private:
    AFQRememberLineEdit*   m_qNameLineEdit = nullptr;
    AFQRememberLineEdit*   m_qUrlLineEdit = nullptr;
    QPushButton*           m_qClearButton = nullptr;

    QString m_sUuid;
    
    bool m_bNew = false;
#pragma endregion private member var
};


class AFQCustomBrowserCollection : public AFCQMainBaseWidget
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
        bool newCustom = false;
    };

public slots:

private slots:
    void qslotDeleteCustomBrowser();
    void qslotTextFilled();
    void qslotApplyTriggered();

signals:

#pragma endregion QT Field

#pragma region public func
public:
    void CustomBrowserCollectionInit(
        QVector<AFQCustomBrowserCollection::CustomBrowserInfo> vec);
    void ReloadCustomBrowserList(
        QVector<AFQCustomBrowserCollection::CustomBrowserInfo> vec);

#pragma endregion public func

#pragma region protected func
protected:
#pragma endregion protected func

#pragma region private func
private:
    void _AddNewCustomBrowser();
    void _LoadCustomBrowser(QString name, QString path, QString uuid);
    void _LoadCustomBrowserList(
        QVector<AFQCustomBrowserCollection::CustomBrowserInfo> vec);
    QList<QString> _SaveCustomBrowserList();
#pragma endregion private func

#pragma region private member var
private:
    Ui::AFQCustomBrowserCollection* ui;

    AFQCustomList* m_qNewLineWidget = nullptr;
    QVector<AFQCustomBrowserCollection::CustomBrowserInfo> m_qInfoVec;

    int m_LoadedCustomBrowser = 0;

#pragma endregion private member var
};

#endif // CCUSTOMBROWSERCOLLECTION_H
