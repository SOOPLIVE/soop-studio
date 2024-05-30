#ifndef CAUTOCONFIGFINISHPAGE_H
#define CAUTOCONFIGFINISHPAGE_H

#include <QWidget>

namespace Ui {
class AFQAutoConfigFinishPage;
}

class AFQAutoConfigFinishPage : public QWidget
{
    Q_OBJECT

public:
    explicit AFQAutoConfigFinishPage(QWidget *parent = nullptr);
    ~AFQAutoConfigFinishPage();

private:
    Ui::AFQAutoConfigFinishPage *ui;
};

#endif // CAUTOCONFIGFINISHPAGE_H
