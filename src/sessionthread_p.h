/*
  SPDX-FileCopyrightText: 2010 BetterInbox <contact@betterinbox.com>
  SPDX-FileContributor: Christophe Laveault <christophe@betterinbox.com>
  SPDX-FileContributor: Gregory Schlomoff <gregory.schlomoff@gmail.com>

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QMutex>
#include <QQueue>
#include <QSslSocket>
#include <QThread>

#include <ksslerroruidata.h>

class QFile;
namespace KSmtp
{
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
    void responseReceived(const KSmtp::ServerResponse &response);
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
    bool m_useProxy = false;
};
}

