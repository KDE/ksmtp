/*
  SPDX-FileCopyrightText: 2010 BetterInbox <contact@betterinbox.com>
  SPDX-FileContributor: Gregory Schlomoff <gregory.schlomoff@gmail.com>

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include "session.h"

#include <QObject>
#include <QQueue>
#include <QStringList>
#include <QTimer>

#include <QSslSocket>

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
    void startSsl(QSsl::SslProtocol version);

    QSsl::SslProtocol negotiatedEncryption() const;

public Q_SLOTS:
    void handleSslError(const KSslErrorUiData &data);

    void socketDisconnected();
    void encryptionNegotiationResult(bool encrypted, QSsl::SslProtocol version);
    void responseReceived(const KSmtp::ServerResponse &response);
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
    QSsl::SslProtocol m_sslVersion;

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

