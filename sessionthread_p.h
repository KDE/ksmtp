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
  License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef KSMTP_SESSIONTHREAD_H
#define KSMTP_SESSIONTHREAD_H

#include <QThread>
#include <QMutex>
#include <QQueue>

#include <ktcpsocket.h>

namespace KSmtp {

  class ServerResponse;
  class Session;

  class SessionThread : public QThread
  {
    Q_OBJECT

  public:
    explicit SessionThread(const QString &hostName, quint16 port, Session *session);
    ~SessionThread();

    QString hostName() const;
    quint16 port() const;

  public slots:
    void reconnect();
    void closeSocket();
    void startTls();
    void tlsConnected();
    void sendData(const QByteArray &payload);

  signals:
    void responseReceived(const ServerResponse &response);
    void sslError(const KSslErrorUiData&);

  protected:
    void run();

  private slots:
    void writeDataQueue();
    void readResponse();
    void doCloseSocket();

  private:
    ServerResponse parseResponse(const QByteArray &response);

    KTcpSocket *m_socket;
    QMutex m_mutex;
    QQueue<QByteArray> m_dataQueue;

    Session *m_parentSession;
    QString m_hostName;
    quint16 m_port;
  };

}

#endif // KSMTP_SESSIONTHREAD_H
