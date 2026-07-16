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
    const char* GetButtonType() { return typeName; };

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

public slots:
    void OnButtonClicked();
    void OnButtonUnchecked();

signals:
    void ButtonClicked();

private:
    Ui::AFQSettingTabButton *ui;
    const char* typeName;
};

#endif // CSETTINGTABBUTTON_H
