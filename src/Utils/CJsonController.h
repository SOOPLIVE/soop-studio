
#ifndef AFCJSONCONTROLLER_H
#define AFCJSONCONTROLLER_H

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>


class AFJsonController
{
public:
    AFJsonController();

    QString ReadDpiSample();
    bool WriteDpiSample(QString dpiValue);
    
    QString ReadDockPositionSample();
    bool WriteDockPositionSample(QByteArray dockPos);
};

#endif // AFCJSONCONTROLLER_H
