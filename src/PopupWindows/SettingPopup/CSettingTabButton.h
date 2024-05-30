#ifndef CSETTINGTABBUTTON_H
#define CSETTINGTABBUTTON_H

#include <QPushButton>

namespace Ui {
class AFQSettingTabButton;
}

class AFQSettingTabButton : public QPushButton
{
    Q_OBJECT

public:
    explicit AFQSettingTabButton(QWidget *parent = nullptr);
    ~AFQSettingTabButton();
    void SetButton(const char* type, QString name);
    const char* GetButtonType() { return m_pTypeName; };

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

public slots:
    void qslotButtonClicked();
    void qslotButtonUnchecked();

signals:
    void qsignalButtonClicked();

private:
    Ui::AFQSettingTabButton *ui;
    const char* m_pTypeName;
};

#endif // CSETTINGTABBUTTON_H
