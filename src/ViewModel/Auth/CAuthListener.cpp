#include "CAuthListener.hpp"

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QString>
#include <QtNetwork/QTcpSocket>

#include "qt-wrappers.hpp"

#include "Common/StudioDefine.h"
#include "Application/CApplication.h"


static const QString serverResponseHeader =
	QStringLiteral("HTTP/1.0 200 OK\n"
		       "Connection: close\n"
		       "Content-Type: text/html; charset=UTF-8\n"
		       "Server: SOOP Studio\n"
		       "\n"
		       "<html><head><title>SOOP Studio"
		       "</title></head>");


AFAuthListener::AFAuthListener(QObject *parent) : QObject(parent)
{
	m_pServer = new QTcpServer(this);
	connect(m_pServer, &QTcpServer::newConnection, this,
		&AFAuthListener::NewConnection);
	if (!m_pServer->listen(QHostAddress::LocalHost, 0)) {
		blog(LOG_DEBUG, "Server could not start");
		emit fail();
	} else {
		blog(LOG_DEBUG, "Server started at port %d",
		     m_pServer->serverPort());
	}
}

quint16 AFAuthListener::GetPort()
{
	return m_pServer ? m_pServer->serverPort() : 0;
}

void AFAuthListener::SetState(QString state)
{
	this->m_state = state;
}

void AFAuthListener::SetGlobalSoop()
{
	m_globalSoop = true;
}

void AFAuthListener::NewConnection()
{
	QTcpSocket *socket = m_pServer->nextPendingConnection();
	if (socket) {
		connect(socket, &QTcpSocket::disconnected, socket,
			&QTcpSocket::deleteLater);
		connect(socket, &QTcpSocket::readyRead, socket, [&, socket]() {
			QByteArray buffer;
			while (socket->bytesAvailable() > 0) {
				buffer.append(socket->readAll());
			}
			socket->write(QT_TO_UTF8(serverResponseHeader));
			QString redirect = QString::fromLatin1(buffer);
			blog(LOG_DEBUG, "redirect: %s", QT_TO_UTF8(redirect));

			if (true == m_globalSoop) {
				QUrl urldata(redirect);
				emit ok(urldata.fromPercentEncoding(redirect.toUtf8()));
			}
			else {
				QRegularExpression re_state(
					"(&|\\?)state=(?<state>[^&]+)");
				QRegularExpression re_code(
					"(&|\\?)code=(?<code>[^&]+)");

				QRegularExpressionMatch match =
					re_state.match(redirect);

				QString code;

				if (match.hasMatch()) {
					if (m_state == match.captured("state")) {
						match = re_code.match(redirect);
						if (!match.hasMatch())
							blog(LOG_DEBUG, "no 'code' "
								"in server "
								"redirect");

						code = match.captured("code");
					}
					else {
						blog(LOG_WARNING, "state mismatch "
							"while handling "
							"redirect");
					}
				}
				else {
					blog(LOG_DEBUG, "no 'state' in "
						"server redirect");
				}

				if (code.isEmpty()) {
					emit fail();
				}
				else {
					emit ok(code);
				}
			}			
			socket->flush();
			socket->close();
		});
	} else {
		emit fail();
	}
}
