/*
  SPDX-FileCopyrightText: 2010 BetterInbox <contact@betterinbox.com>
  SPDX-FileContributor: Christophe Laveault <christophe@betterinbox.com>
  SPDX-FileContributor: Gregory Schlomoff <gregory.schlomoff@gmail.com>

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "session.h"
#include "job.h"
#include "ksmtp_debug.h"
#include "loginjob.h"
#include "sendjob.h"
#include "serverresponse_p.h"
#include "session_p.h"
#include "sessionthread_p.h"

#include <QEventLoop>
#include <QHostAddress>
#include <QHostInfo>
#include <QPointer>
#include <QSslSocket>
#include <QUrl>

using namespace KSmtp;

Q_DECLARE_METATYPE(QSsl::SslProtocol)
Q_DECLARE_METATYPE(KSslErrorUiData)

SessionPrivate::SessionPrivate(Session *session)
    : QObject(session)
    , q(session)
    , m_state(Session::Disconnected)
    , m_socketTimerInterval(60000)
    , m_sslVersion(QSsl::UnknownProtocol)
{
    qRegisterMetaType<QSsl::SslProtocol>();
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

void SessionPrivate::setAuthenticationMethods(const QList<QByteArray> &authMethods)
{
    for (const QByteArray &method : authMethods) {
        QString m = QString::fromLatin1(method);
        if (!m_authModes.contains(m)) {
            m_authModes.append(m);
        }
    }
}

void SessionPrivate::startHandshake()
{
    QString hostname = m_customHostname;

    if (hostname.isEmpty()) {
        // FIXME: QHostInfo::fromName can get a FQDN, but does a DNS lookup
        hostname = QHostInfo::localHostName();
        if (hostname.isEmpty()) {
            hostname = QStringLiteral("localhost.invalid");
        } else if (!hostname.contains(QLatin1Char('.'))) {
            hostname += QStringLiteral(".localnet");
        }
    }

    QByteArray cmd;
    if (!m_ehloRejected) {
        cmd = "EHLO ";
    } else {
        cmd = "HELO ";
    }
    setState(Session::Handshake);
    sendData(cmd + QUrl::toAce(hostname));
}

Session::Session(const QString &hostName, quint16 port, QObject *parent)
    : QObject(parent)
    , d(new SessionPrivate(this))
{
    qRegisterMetaType<KSmtp::ServerResponse>("KSmtp::ServerResponse");

    QHostAddress ip;
    QString saneHostName = hostName;
    if (ip.setAddress(hostName)) {
        // saneHostName = QStringLiteral("[%1]").arg(hostName);
    }

    d->m_thread = new SessionThread(saneHostName, port, this);
    d->m_thread->start();

    connect(d->m_thread, &SessionThread::sslError, d, &SessionPrivate::handleSslError);
}

Session::~Session()
{
}

void Session::setUseNetworkProxy(bool useProxy)
{
    d->m_thread->setUseNetworkProxy(useProxy);
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

void Session::setCustomHostname(const QString &hostname)
{
    d->m_customHostname = hostname;
}

QString Session::customHostname() const
{
    return d->m_customHostname;
}

void Session::open()
{
    QTimer::singleShot(0, d->m_thread, &SessionThread::reconnect);
    d->startSocketTimer();
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
    connect(this, &Session::stateChanged, this, [&](Session::State state) {
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
    if (m_state == s) {
        return;
    }

    m_state = s;
    Q_EMIT q->stateChanged(m_state);

    // After a handshake success or failure, exit the startup event loop if any
    if (m_startLoop && (m_state == Session::NotAuthenticated || m_state == Session::Disconnected)) {
        m_startLoop->quit();
    }
}

void SessionPrivate::sendData(const QByteArray &data)
{
    QMetaObject::invokeMethod(
        m_thread,
        [this, data] {
            m_thread->sendData(data);
        },
        Qt::QueuedConnection);
}

void SessionPrivate::responseReceived(const ServerResponse &r)
{
    // qCDebug(KSMTP_LOG) << "S:: [" << r.code() << "]" << (r.isMultiline() ? "-" : " ") << r.text();

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
            if (r.text().startsWith("SIZE ")) { // krazy:exclude=strings
                m_size = r.text().remove(0, QByteArray("SIZE ").count()).toInt();
            } else if (r.text() == "STARTTLS") {
                m_allowsTls = true;
            } else if (r.text().startsWith("AUTH ")) { // krazy:exclude=strings
                setAuthenticationMethods(r.text().remove(0, QByteArray("AUTH ").count()).split(' '));
            }

            if (!r.isMultiline()) {
                setState(Session::NotAuthenticated);
                startNext();
            }
        }
    }

    if (m_state == Session::Ready) {
        if (r.isCode(22) || m_ehloRejected) {
            startHandshake();
            return;
        }
    }

    if (m_currentJob) {
        m_currentJob->handleResponse(r);
    }
}

void SessionPrivate::socketConnected()
{
    stopSocketTimer();
    setState(Session::Ready);

    bool useSsl = false;
    if (!m_queue.isEmpty()) {
        if (auto login = qobject_cast<LoginJob *>(m_queue.first())) {
            useSsl = login->encryptionMode() == LoginJob::SSLorTLS;
        }
    }

    if (q->state() == Session::Ready && useSsl) {
        startNext();
    }
}

void SessionPrivate::socketDisconnected()
{
    qCDebug(KSMTP_LOG) << "Socket disconnected";
    setState(Session::Disconnected);
    m_thread->closeSocket();

    if (m_currentJob) {
        m_currentJob->connectionLost();
    } else if (!m_queue.isEmpty()) {
        m_currentJob = m_queue.takeFirst();
        m_currentJob->connectionLost();
    }

    auto copy = m_queue;
    qDeleteAll(copy);
    m_queue.clear();
}

void SessionPrivate::startSsl(QSsl::SslProtocol version)
{
    QMetaObject::invokeMethod(
        m_thread,
        [this, version] {
            m_thread->startSsl(version);
        },
        Qt::QueuedConnection);
}

QSsl::SslProtocol SessionPrivate::negotiatedEncryption() const
{
    return m_sslVersion;
}

void SessionPrivate::encryptionNegotiationResult(bool encrypted, QSsl::SslProtocol version)
{
    if (encrypted) {
        // Get the updated auth methods
        startHandshake();
    }

    m_sslVersion = version;
}

void SessionPrivate::addJob(Job *job)
{
    m_queue.append(job);
    // Q_EMIT q->jobQueueSizeChanged( q->jobQueueSize() );

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
    QTimer::singleShot(0, this, [this]() {
        doStartNext();
    });
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

    // sending can take a while depending on bandwidth - don't fail with timeout
    // if it takes longer
    if (qobject_cast<const KSmtp::SendJob *>(m_currentJob)) {
        stopSocketTimer();
    }
}

void SessionPrivate::jobDone(KJob *job)
{
    Q_UNUSED(job)
    Q_ASSERT(job == m_currentJob);

    // If we're in disconnected state it's because we ended up
    // here because the inactivity timer triggered, so no need to
    // stop it (it is single shot)
    if (m_state != Session::Disconnected) {
        if (!qobject_cast<const KSmtp::SendJob *>(m_currentJob)) {
            stopSocketTimer();
        }
    }

    m_jobRunning = false;
    m_currentJob = nullptr;
    // Q_EMIT q->jobQueueSizeChanged( q->jobQueueSize() );
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

    connect(&m_socketTimer, &QTimer::timeout, this, &SessionPrivate::onSocketTimeout);

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
    disconnect(&m_socketTimer, &QTimer::timeout, this, &SessionPrivate::onSocketTimeout);
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
    : m_text(text)
    , m_code(code)
    , m_multiline(multiline)
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
