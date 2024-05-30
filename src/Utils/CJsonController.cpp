
#include "CJsonController.h"

AFJsonController::AFJsonController() {}

QString AFJsonController::ReadDpiSample()
{
    QString val;
    QFile file;
    file.setFileName("service.json");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    val = file.readAll();
    file.close();
    QJsonDocument d = QJsonDocument::fromJson(val.toUtf8());
    QJsonObject sett2 = d.object();
    QJsonValue value = sett2.value(QString("dpi_value"));
    qWarning() << value;
    QString item = value.toString();
    qWarning() << "QJsonObject of description: " << item;
    if (item == "")
    {
        item = "1.0";
    }
    return item;
}

QString AFJsonController::ReadDockPositionSample()
{
    QString val;
    QFile file;
    file.setFileName("service.json");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    val = file.readAll();
    file.close();
    QJsonDocument d = QJsonDocument::fromJson(val.toUtf8());
    QJsonObject sett2 = d.object();
    QJsonValue value = sett2.value(QString("dock_position"));
    return value.toString();
}

bool AFJsonController::WriteDockPositionSample(QByteArray dockPos)
{
    QFile file("service.json"); // json file
    if (!file.open(QIODevice::ReadOnly)) //read json content
    {
        return false;
    }

    QJsonDocument jsonOrg = QJsonDocument::fromJson(file.readAll());
    qDebug() << jsonOrg;
    file.close();

    QJsonObject rootObject = jsonOrg.object();
    qDebug() << rootObject;
    rootObject["dock_position"] = QString(dockPos);
    qDebug() << rootObject;

    QJsonDocument doc(rootObject);

    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }

    file.write(doc.toJson());
    file.close();
    return true;
}

bool AFJsonController::WriteDpiSample(QString dpiValue)
{
    QFile file("service.json"); // json file
    if (!file.open(QIODevice::ReadOnly)) //read json content
    {
        return false;
    }

    QJsonDocument jsonOrg = QJsonDocument::fromJson(file.readAll());
    qDebug() << jsonOrg;
    file.close();

    QJsonObject rootObject = jsonOrg.object();
    qDebug() << rootObject;
    rootObject["dpi_value"] = dpiValue;
    qDebug() << rootObject;

    QJsonDocument doc(rootObject);

    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }

    file.write(doc.toJson());
    file.close();

    return true;
}