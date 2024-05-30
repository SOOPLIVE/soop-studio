#ifndef CAUTOCONFIGSTACKEDWIDGET_H
#define CAUTOCONFIGSTACKEDWIDGET_H

#include <QWidget>

namespace Ui {
class AFQAutoConfigStackedWidget;
}

class AFQAutoConfigStackedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AFQAutoConfigStackedWidget(QWidget *parent = nullptr);
    ~AFQAutoConfigStackedWidget();

private:
    Ui::AFQAutoConfigStackedWidget *ui;
};

#endif // CAUTOCONFIGSTACKEDWIDGET_H
