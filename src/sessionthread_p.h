/*
  Copyright 2010 BetterInbox <contact@betterinbox.com>
      Author: Christophe Laveault <christophe@betterinbox.com>
              Gregory Schlomoff <gregory.schlomoff@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef KSMTP_SESSIONTHREAD_P_H
#define KSMTP_SESSIONTHREAD_P_H

#include <QThread>
#include <QMutex>
#include <QQueue>

#include <ktcpsocket.h>

class QFile;
namespace KSmtp {
class ServerResponse;
class Session;

class SessionThread : public QThread
{
    Q_OBJECT

public:
    explicit SessionThread(const QString &hostName, quint16 port, Session *session);
    ~SessionThread() override;

    Q_REQUIRED_RESULT QString hostName() const;
    Q_REQUIRED_RESULT quint16 port() const;

    void setUseNetworkProxy(bool useProxy);

    void handleSslErrorResponse(bool ignoreError);

public Q_SLOTS:
    void reconnect();
    void closeSocket();
    void startSsl(QSsl::SslProtocol version);
    void sendData(const QByteArray &payload);

Q_SIGNALS:
    void encryptionNegotiationResult(bool encrypted, QSsl::SslProtocol protocol);
    void responseReceived(const ServerResponse &response);
    void sslError(const KSslErrorUiData &);

protected:
    void run() override;

private Q_SLOTS:
    void sslConnected();
    void writeDataQueue();
    void readResponse();
    void doCloseSocket();
    void doHandleSslErrorResponse(bool ignoreError);

private:
    ServerResponse parseResponse(const QByteArray &response);

    std::unique_ptr<QSslSocket> m_socket;
    QMutex m_mutex;
    QQueue<QByteArray> m_dataQueue;
    std::unique_ptr<QFile> m_logFile;

    Session *m_parentSession = nullptr;
    QString m_hostName;
    quint16 m_port;
    bool m_useProxy;
};
}

#endif // KSMTP_SESSIONTHREAD_H
