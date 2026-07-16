#ifndef CSAVVYSTYLEBUTTON_H
#define CSAVVYSTYLEBUTTON_H

#include <QWidget>
#include <UIComponent/CBasicHoverWidget.h>

namespace Ui {
class AFQSavvyStyleButton;
}


class AFQSavvyStyleButton : public AFQHoverWidget
{
    Q_OBJECT

public:
    explicit AFQSavvyStyleButton(std::string styleNum, QString styleName, QString styleText, QWidget* parent = nullptr);
    ~AFQSavvyStyleButton();

    void CheckStyle(bool checked);

    std::string StyleNum() { return m_styleNum; };
    void TriggerClick();
private:
    Ui::AFQSavvyStyleButton *ui;
    QString m_styleName;

    bool m_clicked = false;
    std::string m_styleNum;
};

#endif // CSAVVYSTYLEBUTTON_H
