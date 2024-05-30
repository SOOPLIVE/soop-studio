#ifndef CBROADINFODOCKWIDGET_H
#define CBROADINFODOCKWIDGET_H

#include "Blocks/CBaseDockWidget.h"

namespace Ui {
class AFBroadInfoDockWidget;
}

class AFBroadInfoDockWidget : public AFQBaseDockWidget
{
    Q_OBJECT

public:
    explicit AFBroadInfoDockWidget(QWidget *parent = nullptr);
    ~AFBroadInfoDockWidget();

private:
    Ui::AFBroadInfoDockWidget *ui;
};

#endif // CBROADINFODOCKWIDGET_H
