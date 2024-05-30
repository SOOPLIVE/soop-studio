#ifndef CAUTOCONFIGTESTPAGE_H
#define CAUTOCONFIGTESTPAGE_H

#include <QWidget>

namespace Ui {
class AFQAutoConfigTestPage;
}

class AFQAutoConfigTestPage : public QWidget
{
    Q_OBJECT

public:
    explicit AFQAutoConfigTestPage(QWidget *parent = nullptr);
    ~AFQAutoConfigTestPage();

private:
    Ui::AFQAutoConfigTestPage *ui;
};

#endif // CAUTOCONFIGTESTPAGE_H
