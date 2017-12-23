/*
  Copyright 2010 BetterInbox <contact@betterinbox.com>
      Author: Gregory Schlomoff <gregory.schlomoff@gmail.com>

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

#ifndef KSMTP_SESSION_P_H
#define KSMTP_SESSION_P_H

#include "session.h"

#include <QObject>
#include <QQueue>
#include <QTimer>
#include <QStringList>

#include <KTcpSocket>

class KJob;
class QEventLoop;

namespace KSmtp
{

class Job;
class SessionThread;
class ServerResponse;

class KSMTP_EXPORT SessionPrivate : public QObject
{
    Q_OBJECT

    friend class Session;

public:
    explicit SessionPrivate(Session *session);
    ~SessionPrivate() override;

    void addJob(Job *job);
    void sendData(const QByteArray &data);
    void setState(Session::State s);
    void startSsl(KTcpSocket::SslVersion version);

    KTcpSocket::SslVersion negotiatedEncryption() const;

public Q_SLOTS:
    void handleSslError(const KSslErrorUiData &data);

    void socketDisconnected();
    void encryptionNegotiationResult(bool encrypted, KTcpSocket::SslVersion version);
    void responseReceived(const ServerResponse &response);
    void socketConnected();
    void setAuthenticationMethods(const QList<QByteArray> &authMethods);

private Q_SLOTS:
    void doStartNext();
    void jobDone(KJob *job);
    void jobDestroyed(QObject *job);

    void onSocketTimeout();

private:

    void startHandshake();
    void startNext();
    void startSocketTimer();
    void stopSocketTimer();
    void restartSocketTimer();

    Session *const q;

    // Smtp session
    Session::State m_state;
    SessionThread *m_thread = nullptr;
    SessionUiProxy::Ptr m_uiProxy;
    int m_socketTimerInterval = 0;
    QTimer m_socketTimer;
    QEventLoop *m_startLoop = nullptr;
    KTcpSocket::SslVersion m_sslVersion;

    // Jobs
    bool m_jobRunning = false;
    Job *m_currentJob = nullptr;
    QQueue<Job *> m_queue;

    // Smtp info
    bool m_ehloRejected = false;
    int m_size = 0;
    bool m_allowsTls = false;
    QStringList m_authModes;
    QString m_customHostname;
};

}

#endif //KSMTP_SESSION_P_H
