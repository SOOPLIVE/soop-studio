#pragma once
#include <QWidget>
#include <QPointer>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QLayout>

#include <QInputMethodEvent>

class AFQBasicLineEdit : public QLineEdit {

#pragma region QT Field
    Q_OBJECT
public:
    AFQBasicLineEdit(QWidget* parent = nullptr);

signals:
    void qsignalAsciiFailed();

private slots:
    void _qslotFilterText(const QString& text);
#pragma endregion QT Field

#pragma region public func
public:
    void AllowOnlyAscii();
    void RestorePreviousText(bool enable);
#pragma endregion public func

#pragma region protected func
protected:
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    virtual void inputMethodEvent(QInputMethodEvent* event);
#pragma endregion protected func

#pragma region private member var
private:
    bool m_restorePreviousText = true;

    QString previousText; 
    QString m_lastPreedit;
    int m_preeditStart = -1;
    int m_preeditLength = 0;
#pragma endregion private member var
};

class AFQFocusAwareLineEdit : public AFQBasicLineEdit
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

public:
    AFQFocusAwareLineEdit(QWidget* parent = nullptr);
    ~AFQFocusAwareLineEdit();

signals:
    void qsignalFocusChanged(bool hasFocus);
#pragma endregion QT Field, CTOR/DTOR

#pragma region protected func
protected:
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    //void inputMethodEvent(QInputMethodEvent* event) override;
#pragma endregion protected func
};



class AFQLengthAwareLineEdit : public QWidget
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

public:
    explicit AFQLengthAwareLineEdit(QWidget* parent = nullptr);
    ~AFQLengthAwareLineEdit();

signals:
    void qsignalTextChanged(const QString&);
    void qsignalEditFinished();

private slots:
    void _qslotUpdateCurrentTextLength();
    void _qslotLineEditFocusChanged(bool hasFocus);
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void SetMaxLength(int maxLength);
    void SetText(const QString& text);
    void SetPlaceholderText(const QString& text);
    void SetMaxLengthVisible(bool visible);
    void SelectAll();
    void SetFocus();
    void AllowOnlyAscii();

    QString GetText();

    void SetLineEditProperty(const char* propertyName, const QVariant& propertyValue);
    void PolishLineEditStyle();

    AFQFocusAwareLineEdit* GetLineEdit() { return m_lineEdit; };

    void SetUseBroadInfoDock(bool useBroadInfoDock);
#pragma endregion public func

#pragma region private func
private:
    void _Init();
#pragma endregion private func

#pragma region protected member var
protected:
    QPointer<QHBoxLayout> m_hLayout;
    QPointer<AFQFocusAwareLineEdit> m_lineEdit;
    QPointer<QLabel> m_currentLengthLabel; // Show Current Length
    QPointer<QLabel> m_maxLengthLabel; // Show Max Length

    int m_maxLength = 10;
    bool m_useBroadInfoDock = false;
#pragma endregion protected member var
};


class AFQPrefixLengthAwareLineEdit : public AFQLengthAwareLineEdit
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

public:
    explicit AFQPrefixLengthAwareLineEdit(QWidget* parent = nullptr);
    ~AFQPrefixLengthAwareLineEdit();

public slots:
    void qslotSetPrefixVisible(bool visible);
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void SetPrefixText(const QString& text);
#pragma endregion public func

#pragma region private func
private:
    void _InitPrefix();
#pragma endregion private func

#pragma region private member var
private:
    QPointer<QLabel> m_prefixLabel;
    bool m_useBroadInfoDock = false;
#pragma endregion private member var
};


class AFQSecureLengthAwareLineEdit : public AFQLengthAwareLineEdit 
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

public:
    explicit AFQSecureLengthAwareLineEdit(QWidget* parent = nullptr);
    ~AFQSecureLengthAwareLineEdit();

public slots:
    void qslotHideTextToggled();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void SetHideTextButtonVisible(bool visible);
#pragma endregion public func

#pragma region private func
private:
    void _InitHideText();
#pragma endregion private func

#pragma region private member var
private:
    QPointer<QPushButton> m_hideTextButton;
#pragma endregion private member var
};