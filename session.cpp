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

#include <KDE/KDebug>
#include <QHostAddress>
#include <QMetaType>
#include <QUrl>
#include <QEventLoop>

#include "sessionthread_p.h"
#include "job.h"
#include "serverresponse_p.h"

using namespace KSmtp;

SessionPrivate::SessionPrivate(Session *session)
  : QObject(session),
    q(session),
    m_state(Session::Disconnected),
    m_thread(0),
    m_socketTimerInterval(10000),
    m_startLoop(0),
    m_jobRunning(false),
    m_currentJob(0),
    m_ehloRejected(false),
    m_size(0),
    m_allowsTls(false)
{
}

Session::Session(const QString &hostName, quint16 port, QObject *parent)
  : QObject(parent),
    d(new SessionPrivate(this))

{
  qRegisterMetaType<ServerResponse>("ServerResponse");

  QHostAddress ip;
  QString saneHostName = hostName;
  if (ip.setAddress(hostName)) {
    saneHostName = '[' + hostName + ']';
  }

  d->m_thread = new SessionThread(saneHostName, port, this);
  d->m_thread->start();
}

Session::~Session()
{
}

SessionPrivate::~SessionPrivate()
{
  m_thread->quit();
  m_thread->wait(10000);
  delete m_thread;
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
  QEventLoop loop(0);
  d->m_startLoop = &loop;
  open();
  d->m_startLoop->exec();
  d->m_startLoop = 0;
}

void Session::close()
{
  d->sendData("QUIT");
  d->socketDisconnected();
}

void SessionPrivate::setState(Session::State s)
{
  m_state = s;
  emit q->stateChanged(m_state);

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
  kDebug() << "S:: [" << r.code() << "] " << r.text();

  if (m_state == Session::Handshake) {
    if (r.isCode(500) || r.isCode(502)) {
      if (!m_ehloRejected) {
        setState(Session::Ready);
        m_ehloRejected = true;
      } else {
        kWarning() << "KSmtp::Session: Handshake failed with both EHLO and HELO";
        m_thread->closeSocket();
      }
    }

    if (r.isCode(25)) {
      setState(Session::NotAuthenticated);
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
    }
  }

  if (m_state == Session::NotAuthenticated && r.isCode(25)) {
    if (r.text().startsWith("SIZE ")) {
      m_size = r.text().remove(0, QByteArray("SIZE ").count()).toInt();
    }
    else if (r.text() == "STARTTLS") {
      m_allowsTls = true;
    }
    else if (r.text().startsWith("AUTH ")) {
      QList<QByteArray> modes = r.text().remove(0, QByteArray("AUTH ").count()).split(' ');
      foreach (QByteArray mode, modes) {
        QString m(mode);
        if (!m_authModes.contains(m)) {
          m_authModes.append(m);
        }
      }
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
  kDebug() << "Socket disconnected";
  setState(Session::Disconnected);
  m_thread->closeSocket();
}

void SessionPrivate::startTls()
{
  QMetaObject::invokeMethod(m_thread, "startTls", Qt::QueuedConnection);
}

void SessionPrivate::addJob(Job *job)
{
  m_queue.append(job);
  //emit q->jobQueueSizeChanged( q->jobQueueSize() );

  connect(job, SIGNAL(result(KJob*)), SLOT(jobDone(KJob*)));
  connect(job, SIGNAL(destroyed(QObject*)), SLOT(jobDestroyed(QObject*)));

  if (m_state != Session::Disconnected) {
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
  m_currentJob = 0;
  //emit q->jobQueueSizeChanged( q->jobQueueSize() );
  startNext();
}

void SessionPrivate::jobDestroyed(QObject *job)
{
  m_queue.removeAll(static_cast<Job*>(job));
  if (m_currentJob == job)
    m_currentJob = 0;
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
  if (m_socketTimerInterval<0) {
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


ServerResponse::ServerResponse(int code, const QByteArray &text)
  : m_code(code),
  m_text(text)
{
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
    while(otherCpy > 0) {
      otherCpy = otherCpy / 10;
      codeLength++;
    }
  }

  int div = 1;
  for (int i = 0; i < 3 - codeLength; i++) {
    div *= 10;
  }

  return m_code / div == other;
}
