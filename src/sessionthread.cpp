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

#include "sessionthread_p.h"
#include "serverresponse_p.h"
#include "session.h"
#include "session_p.h"
#include "ksmtp_debug.h"

#include <QTimer>
#include <QUrl>
#include <QFile>
#include <QCoreApplication>
#include <QNetworkProxy>

using namespace KSmtp;

SessionThread::SessionThread(const QString &hostName, quint16 port, Session *session)
    : QThread()
    , m_socket(nullptr)
    , m_logFile(nullptr)
    , m_parentSession(session)
    , m_hostName(hostName)
    , m_port(port)
    , m_useProxy(false)
{
    moveToThread(this);

    const auto logfile = qgetenv("KSMTP_SESSION_LOG");
    if (!logfile.isEmpty()) {
        static uint sSessionCount = 0;
        const QString filename = QStringLiteral("%1.%2.%3").arg(QString::fromUtf8(logfile))
                                 .arg(qApp->applicationPid())
                                 .arg(++sSessionCount);
        m_logFile = new QFile(filename);
        if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qCWarning(KSMTP_LOG) << "Failed to open log file" << filename << ":" << m_logFile->errorString();
            delete m_logFile;
            m_logFile = nullptr;
        }
    }
}

SessionThread::~SessionThread()
{
    delete m_logFile;
}

QString SessionThread::hostName() const
{
    return m_hostName;
}

quint16 SessionThread::port() const
{
    return m_port;
}

void SessionThread::sendData(const QByteArray &payload)
{
    QMutexLocker locker(&m_mutex);
    //qCDebug(KSMTP_LOG) << "C:: " << payload;
    if (m_logFile) {
        m_logFile->write("C: " + payload + '\n');
        m_logFile->flush();
    }

    m_dataQueue.enqueue(payload + "\r\n");
    QTimer::singleShot(0, this, &SessionThread::writeDataQueue);
}

void SessionThread::writeDataQueue()
{
    QMutexLocker locker(&m_mutex);

    while (!m_dataQueue.isEmpty()) {
        m_socket->write(m_dataQueue.dequeue());
    }
}

void SessionThread::readResponse()
{
    QMutexLocker locker(&m_mutex);

    if (!m_socket->bytesAvailable()) {
        return;
    }

    const QByteArray data = m_socket->readLine();
    //qCDebug(KSMTP_LOG) << "S:" << data;
    if (m_logFile) {
        m_logFile->write("S: " + data);
        m_logFile->flush();
    }

    const ServerResponse response = parseResponse(data);
    Q_EMIT responseReceived(response);

    if (m_socket->bytesAvailable()) {
        QTimer::singleShot(0, this, &SessionThread::readResponse);
    }
}

void SessionThread::closeSocket()
{
    QTimer::singleShot(0, this, &SessionThread::doCloseSocket);
}

void SessionThread::doCloseSocket()
{
    m_socket->close();
}

void SessionThread::reconnect()
{
    QMutexLocker locker(&m_mutex);

    if (m_socket->state() != KTcpSocket::ConnectedState
        && m_socket->state() != KTcpSocket::ConnectingState) {
        if (!m_useProxy) {
            qCDebug(KSMTP_LOG) << "using no proxy";

            QNetworkProxy proxy;
            proxy.setType(QNetworkProxy::NoProxy);
            m_socket->setProxy(proxy);
        } else {
            qCDebug(KSMTP_LOG) << "using default system proxy";
        }

        m_socket->connectToHost(hostName(), port());
    }
}

void SessionThread::run()
{
    m_socket = new KTcpSocket;

    connect(m_socket, &KTcpSocket::readyRead,
            this, &SessionThread::readResponse, Qt::QueuedConnection);

    connect(m_socket, &KTcpSocket::disconnected,
            m_parentSession->d, &SessionPrivate::socketDisconnected);
    connect(m_socket, &KTcpSocket::connected,
            m_parentSession->d, &SessionPrivate::socketConnected);
    connect(m_socket, QOverload<KTcpSocket::Error>::of(&KTcpSocket::error),
            this, [this](KTcpSocket::Error err) {
        qCWarning(KSMTP_LOG) << "Socket error:" << err << m_socket->errorString();
        Q_EMIT m_parentSession->connectionError(m_socket->errorString());
    });
    connect(this, &SessionThread::encryptionNegotiationResult,
            m_parentSession->d, &SessionPrivate::encryptionNegotiationResult);

    connect(this, &SessionThread::responseReceived,
            m_parentSession->d, &SessionPrivate::responseReceived);

    exec();

    delete m_socket;
}

void SessionThread::setUseNetworkProxy(bool useProxy)
{
    m_useProxy = useProxy;
}

ServerResponse SessionThread::parseResponse(const QByteArray &resp)
{
    QByteArray response(resp);

    // Remove useless CRLF
    int indexOfCR = response.indexOf("\r");
    int indexOfLF = response.indexOf("\n");

    if (indexOfCR > 0) {
        response.truncate(indexOfCR);
    }
    if (indexOfLF > 0) {
        response.truncate(indexOfLF);
    }

    // Server response code
    QByteArray code = response.left(3);
    bool ok = false;
    const int returnCode = code.toInt(&ok);
    if (!ok) {
        return ServerResponse();
    }

    // RFC821, Appendix E
    const bool multiline = (response.at(3) == '-');

    if (returnCode) {
        response = response.mid(4); // Keep the text part
    }

    return ServerResponse(returnCode, response, multiline);
}

void SessionThread::startSsl(KTcpSocket::SslVersion version)
{
    QMutexLocker locker(&m_mutex);

    m_socket->setAdvertisedSslVersion(version);
    m_socket->ignoreSslErrors();
    connect(m_socket, &KTcpSocket::encrypted, this, &SessionThread::sslConnected);
    m_socket->startClientEncryption();
}

void SessionThread::sslConnected()
{
    QMutexLocker locker(&m_mutex);
    KSslCipher cipher = m_socket->sessionCipher();

    if (!m_socket->sslErrors().isEmpty() || m_socket->encryptionMode() != KTcpSocket::SslClientMode
        || cipher.isNull() || cipher.usedBits() == 0) {
        qCDebug(KSMTP_LOG) << "Initial SSL handshake failed. cipher.isNull() is" << cipher.isNull()
                           << ", cipher.usedBits() is" << cipher.usedBits()
                           << ", the socket says:" <<  m_socket->errorString()
                           << "and the list of SSL errors contains"
                           << m_socket->sslErrors().count() << "items.";
        KSslErrorUiData errorData(m_socket);
        Q_EMIT sslError(errorData);
    } else {
        qCDebug(KSMTP_LOG) << "TLS negotiation done.";

        Q_EMIT encryptionNegotiationResult(true, m_socket->negotiatedSslVersion());
    }
}

void SessionThread::handleSslErrorResponse(bool ignoreError)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    QMetaObject::invokeMethod(this, [this, ignoreError] {
        doHandleSslErrorResponse(ignoreError);
    }, Qt::QueuedConnection);
#else
    QMetaObject::invokeMethod(this, "doHandleSslErrorResponse", Qt::QueuedConnection,
                              Q_ARG(bool, ignoreError));
#endif
}

void SessionThread::doHandleSslErrorResponse(bool ignoreError)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (!m_socket) {
        return;
    }
    if (ignoreError) {
        Q_EMIT encryptionNegotiationResult(true, m_socket->negotiatedSslVersion());
    } else {
        //reconnect in unencrypted mode, so new commands can be issued
        m_socket->disconnectFromHost();
        m_socket->waitForDisconnected();
        m_socket->connectToHost(m_hostName, m_port);
        Q_EMIT encryptionNegotiationResult(false, KTcpSocket::UnknownSslVersion);
    }
}
