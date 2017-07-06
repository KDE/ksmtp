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

#include "session.h"
#include "session_p.h"
#include "sessionthread_p.h"
#include "job.h"
#include "serverresponse_p.h"
#include "ksmtp_debug.h"

#include <QHostAddress>
#include <QUrl>
#include <QEventLoop>
#include <QPointer>

using namespace KSmtp;

Q_DECLARE_METATYPE(KTcpSocket::SslVersion)
Q_DECLARE_METATYPE(KSslErrorUiData)

SessionPrivate::SessionPrivate(Session *session)
    : QObject(session),
      q(session),
      m_state(Session::Disconnected),
      m_thread(nullptr),
      m_socketTimerInterval(10000),
      m_startLoop(nullptr),
      m_sslVersion(KTcpSocket::UnknownSslVersion),
      m_jobRunning(false),
      m_currentJob(nullptr),
      m_ehloRejected(false),
      m_size(0),
      m_allowsTls(false)
{
    qRegisterMetaType<KTcpSocket::SslVersion>();
    qRegisterMetaType<KSslErrorUiData>();
}

SessionPrivate::~SessionPrivate()
{
    m_thread->quit();
    m_thread->wait(10000);
    delete m_thread;
}

void SessionPrivate::handleSslError(const KSslErrorUiData &data)
{
    QPointer<SessionThread> _t = m_thread;
    const bool ignore = m_uiProxy && m_uiProxy->ignoreSslError(data);
    if (_t) {
        _t->handleSslErrorResponse(ignore);
    }
}


Session::Session(const QString &hostName, quint16 port, QObject *parent)
    : QObject(parent),
      d(new SessionPrivate(this))

{
    qRegisterMetaType<ServerResponse>("ServerResponse");

    QHostAddress ip;
    QString saneHostName = hostName;
    if (ip.setAddress(hostName)) {
        //saneHostName = QStringLiteral("[%1]").arg(hostName);
    }

    d->m_thread = new SessionThread(saneHostName, port, this);
    d->m_thread->start();

    connect(d->m_thread, &SessionThread::sslError,
            d, &SessionPrivate::handleSslError);
}

Session::~Session()
{
}

void Session::setUiProxy(const SessionUiProxy::Ptr &uiProxy)
{
    d->m_uiProxy = uiProxy;
}

SessionUiProxy::Ptr Session::uiProxy() const
{
    return d->m_uiProxy;
}

QString Session::hostName() const
{
    return d->m_thread->hostName();
}

quint16 Session::port() const
{
    return d->m_thread->port();
}

Session::State Session::state() const
{
    return d->m_state;
}

bool Session::allowsTls() const
{
    return d->m_allowsTls;
}

QStringList Session::availableAuthModes() const
{
    return d->m_authModes;
}

int Session::sizeLimit() const
{
    return d->m_size;
}

void Session::setSocketTimeout(int ms)
{
    bool timerActive = d->m_socketTimer.isActive();

    if (timerActive) {
        d->stopSocketTimer();
    }

    d->m_socketTimerInterval = ms;

    if (timerActive) {
        d->startSocketTimer();
    }
}

int Session::socketTimeout() const
{
    return d->m_socketTimerInterval;
}

void Session::open()
{
    QTimer::singleShot(0, d->m_thread, SLOT(reconnect()));
}

void Session::openAndWait()
{
    QEventLoop loop(nullptr);
    d->m_startLoop = &loop;
    open();
    d->m_startLoop->exec();
    d->m_startLoop = nullptr;
}

void Session::quit()
{
    if (d->m_state == Session::Disconnected) {
        return;
    }

    d->setState(Quitting);
    d->sendData("QUIT");
}

void Session::quitAndWait()
{
    if (d->m_state == Session::Disconnected) {
        return;
    }

    QEventLoop loop;
    connect(this, &Session::stateChanged,
            this, [&](Session::State state) {
                if (state == Session::Disconnected) {
                    loop.quit();
                }
            });
    d->setState(Quitting);
    d->sendData("QUIT");
    loop.exec();
}

void SessionPrivate::setState(Session::State s)
{
    m_state = s;
    Q_EMIT q->stateChanged(m_state);

    // After a handshake success or failure, exit the startup event loop if any
    if (m_startLoop && (m_state == Session::NotAuthenticated || m_state == Session::Disconnected)) {
        m_startLoop->quit();
    }
}

void SessionPrivate::sendData(const QByteArray &data)
{
    QMetaObject::invokeMethod(m_thread, "sendData",
                              Qt::QueuedConnection, Q_ARG(QByteArray, data));
}

void SessionPrivate::responseReceived(const ServerResponse &r)
{
    //qCDebug(KSMTP_LOG) << "S:: [" << r.code() << "]" << (r.isMultiline() ? "-" : " ") << r.text();

    if (m_state == Session::Quitting) {
        m_thread->closeSocket();
        return;
    }

    if (m_state == Session::Handshake) {
        if (r.isCode(500) || r.isCode(502)) {
            if (!m_ehloRejected) {
                setState(Session::Ready);
                m_ehloRejected = true;
            } else {
                qCWarning(KSMTP_LOG) << "KSmtp::Session: Handshake failed with both EHLO and HELO";
                q->quit();
                return;
            }
        } else if (r.isCode(25)) {
            if (r.text().startsWith("SIZE ")) { //krazy:exclude=strings
                m_size = r.text().remove(0, QByteArray("SIZE ").count()).toInt();
            } else if (r.text() == "STARTTLS") {
                m_allowsTls = true;
            } else if (r.text().startsWith("AUTH ")) { //krazy:exclude=strings
                const QList<QByteArray> modes = r.text().remove(0, QByteArray("AUTH ").count()).split(' ');
                for (const QByteArray &mode : modes) {
                    QString m = QString::fromLatin1(mode);
                    if (!m_authModes.contains(m)) {
                        m_authModes.append(m);
                    }
                }
            }

            if (r.isMultiline()) {
                // There will be more EHLO/HELO responses, let's wait for them
                return;
            } else {
                setState(Session::NotAuthenticated);
            }
        }
    }

    if (m_state == Session::Ready) {
        if (r.isCode(22) || m_ehloRejected) {
            QByteArray cmd;
            if (!m_ehloRejected) {
                cmd = "EHLO ";
            } else {
                cmd = "HELO ";
            }
            setState(Session::Handshake);
            sendData(cmd + QUrl::toAce(m_thread->hostName()));
            return;
        }
    }

    if (m_currentJob) {
        m_currentJob->handleResponse(r);
    }
}

void SessionPrivate::socketConnected()
{
    setState(Session::Ready);
}

void SessionPrivate::socketDisconnected()
{
    qCDebug(KSMTP_LOG) << "Socket disconnected";
    setState(Session::Disconnected);
    m_thread->closeSocket();
}

void SessionPrivate::startSsl(KTcpSocket::SslVersion version)
{
    QMetaObject::invokeMethod(m_thread, "startSsl", Qt::QueuedConnection,
                              Q_ARG(KTcpSocket::SslVersion, version));
}

KTcpSocket::SslVersion SessionPrivate::negotiatedEncryption() const
{
    return m_sslVersion;
}

void SessionPrivate::encryptionNegotiationResult(bool encrypted, KTcpSocket::SslVersion version)
{
    Q_UNUSED(encrypted);
    m_sslVersion = version;
}

void SessionPrivate::addJob(Job *job)
{
    m_queue.append(job);
    //Q_EMIT q->jobQueueSizeChanged( q->jobQueueSize() );

    connect(job, &KJob::result, this, &SessionPrivate::jobDone);
    connect(job, &KJob::destroyed, this, &SessionPrivate::jobDestroyed);

    if (m_state >= Session::NotAuthenticated) {
        startNext();
    } else {
        m_thread->reconnect();
    }
}

void SessionPrivate::startNext()
{
    QTimer::singleShot(0, this, SLOT(doStartNext()));
}

void SessionPrivate::doStartNext()
{
    if (m_queue.isEmpty() || m_jobRunning || m_state == Session::Disconnected) {
        return;
    }

    startSocketTimer();
    m_jobRunning = true;

    m_currentJob = m_queue.dequeue();
    m_currentJob->doStart();
}

void SessionPrivate::jobDone(KJob *job)
{
    Q_UNUSED(job);
    Q_ASSERT(job == m_currentJob);

    // If we're in disconnected state it's because we ended up
    // here because the inactivity timer triggered, so no need to
    // stop it (it is single shot)
    if (m_state != Session::Disconnected) {
        stopSocketTimer();
    }

    m_jobRunning = false;
    m_currentJob = nullptr;
    //Q_EMIT q->jobQueueSizeChanged( q->jobQueueSize() );
    startNext();
}

void SessionPrivate::jobDestroyed(QObject *job)
{
    m_queue.removeAll(static_cast<Job *>(job));
    if (m_currentJob == job) {
        m_currentJob = nullptr;
    }
}

void SessionPrivate::startSocketTimer()
{
    if (m_socketTimerInterval < 0) {
        return;
    }
    Q_ASSERT(!m_socketTimer.isActive());

    connect(&m_socketTimer, SIGNAL(timeout()), SLOT(onSocketTimeout()));

    m_socketTimer.setSingleShot(true);
    m_socketTimer.start(m_socketTimerInterval);
}

void SessionPrivate::stopSocketTimer()
{
    if (m_socketTimerInterval < 0) {
        return;
    }
    Q_ASSERT(m_socketTimer.isActive());

    m_socketTimer.stop();
    disconnect(&m_socketTimer, SIGNAL(timeout()), this, SLOT(onSocketTimeout()));
}

void SessionPrivate::restartSocketTimer()
{
    stopSocketTimer();
    startSocketTimer();
}

void SessionPrivate::onSocketTimeout()
{
    socketDisconnected();
}


ServerResponse::ServerResponse(int code, const QByteArray &text, bool multiline)
    : m_text(text),
      m_code(code),
      m_multiline(multiline)
{
}

bool ServerResponse::isMultiline() const
{
    return m_multiline;
}

int ServerResponse::code() const
{
    return m_code;
}

QByteArray ServerResponse::text() const
{
    return m_text;
}

bool ServerResponse::isCode(int other) const
{
    int otherCpy = other;
    int codeLength = 0;

    if (other == 0) {
        codeLength = 1;
    } else {
        while (otherCpy > 0) {
            otherCpy /= 10;
            codeLength++;
        }
    }

    int div = 1;
    for (int i = 0; i < 3 - codeLength; i++) {
        div *= 10;
    }

    return m_code / div == other;
}
