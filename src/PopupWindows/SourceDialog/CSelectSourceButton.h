#pragma once

#include <QEvent>
#include <QPointer>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QMovie>

#include "UIComponent/CElidedSlideLabel.h"

class AFQAddSourceMenuButton : public QPushButton
{
    Q_OBJECT
public:
    explicit AFQAddSourceMenuButton(QString id_, QWidget* parent = nullptr) : 
        QPushButton(parent),
        id(id_)
    {}
    ~AFQAddSourceMenuButton() {}

public:
    QString GetSourceId() { return id; }

private:
    QString id;
};

class AFQSelectSourceButton : public QPushButton
{
#pragma region class initializer, destructor
public:
    explicit AFQSelectSourceButton(QString id, QString name, QWidget* parent = nullptr);
    ~AFQSelectSourceButton();

#pragma endregion class initializer, destructor

#pragma region QT Field
    Q_OBJECT
signals:
    void qSignalHoverButton(QString id);
    void qSignalLeaveButton();
#pragma endregion QT Field

protected:
    bool event(QEvent* e) override;

public:
    QString GetSourceId();

#pragma region private member var
private:
    QString m_srcId;

    QPointer<QLabel> m_pIconLabel  = nullptr;
    QPointer<AFQElidedSlideLabel> m_pTextLabel = nullptr;

    QString m_strText;

    QString m_strIconPath;
    QString m_strIconHoverPath;
    QString m_strGuidePath;

    bool m_bHover = false;

#pragma endregion private member var

};