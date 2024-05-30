#ifndef CAUTOCONFIGSTREAMPAGE_H
#define CAUTOCONFIGSTREAMPAGE_H

#include <QWidget>

namespace Ui {
class AFQAutoConfigStreamPage;
}

class AFQAutoConfigStreamPage : public QWidget
{
    Q_OBJECT

public:
    explicit AFQAutoConfigStreamPage(QWidget *parent = nullptr);
    ~AFQAutoConfigStreamPage();

private:
    Ui::AFQAutoConfigStreamPage *ui;
};

#endif // CAUTOCONFIGSTREAMPAGE_H
