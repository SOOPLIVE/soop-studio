#ifndef CCUSTOMBROWSERWIDGET_H
#define CCUSTOMBROWSERWIDGET_H

#include <QWidget>

namespace Ui {
class AFQCustomBrowserWidget;
}

class AFQCustomBrowserWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AFQCustomBrowserWidget(QWidget *parent = nullptr);
    ~AFQCustomBrowserWidget();

private:
    Ui::AFQCustomBrowserWidget *ui;
};

#endif // CCUSTOMBROWSERWIDGET_H
