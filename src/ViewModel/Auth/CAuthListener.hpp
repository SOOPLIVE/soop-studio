#pragma once

#include <QObject>
#include <QtNetwork/QTcpServer>

class AFAuthListener : public QObject {
	Q_OBJECT

	QTcpServer *server;
	QString state;

signals:
	void ok(const QString &code);
	void fail();

protected:
	void NewConnection();
	bool m_globalSoop = false;
public:
	explicit AFAuthListener(QObject *parent = 0);
	quint16 GetPort();
	void SetState(QString state);
	void SetGlobalSoop();
};
