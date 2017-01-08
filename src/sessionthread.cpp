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

using namespace KSmtp;

SessionThread::SessionThread(const QString &hostName, quint16 port, Session *session)
  : QThread(),
    m_socket(0),
    m_parentSession(session),
    m_hostName(hostName),
    m_port(port)
{
  moveToThread(this);
}

SessionThread::~SessionThread()
{
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
  //kDebug() << "C:: " << payload;

  m_dataQueue.enqueue(payload + "\r\n");
  QTimer::singleShot(0, this, SLOT(writeDataQueue()));
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

  ServerResponse response = parseResponse(m_socket->readLine());

  emit responseReceived(response);

  if (m_socket->bytesAvailable()) {
    QTimer::singleShot(0, this, SLOT(readResponse()));
  }
}

void SessionThread::closeSocket()
{
  QTimer::singleShot(0, this, SLOT(doCloseSocket()));
}

void SessionThread::doCloseSocket()
{
  m_socket->close();
}

void SessionThread::reconnect()
{
  QMutexLocker locker(&m_mutex);

  if (m_socket->state() != KTcpSocket::ConnectedState &&
      m_socket->state() != KTcpSocket::ConnectingState) {

    m_socket->connectToHost(hostName(), port());
  }
}

void SessionThread::run()
{
  m_socket = new KTcpSocket;

  connect(m_socket, SIGNAL(readyRead()),
          this, SLOT(readResponse()), Qt::QueuedConnection);

  connect(m_socket, SIGNAL(disconnected()),
          m_parentSession->d, SLOT(socketDisconnected()));

  connect(m_socket, SIGNAL(connected()),
          m_parentSession->d, SLOT(socketConnected()));

  connect(this, SIGNAL(responseReceived(ServerResponse)),
          m_parentSession->d, SLOT(responseReceived(ServerResponse)));

  exec();

  delete m_socket;
}

ServerResponse SessionThread::parseResponse(const QByteArray &resp)
{
  QByteArray response(resp);
  
  // Remove useless CRLF
  int indexOfCR = response.indexOf("\r");
  int indexOfLF = response.indexOf("\n");

  if (indexOfCR > 0)
    response.truncate(indexOfCR);
  if (indexOfLF > 0)
    response.truncate(indexOfLF);

  // Server response code
  QByteArray code = response;
  code.truncate(3);
  int returnCode = code.toInt();

  if (returnCode) {
    response = response.remove(0, 4); // Keep the text part
  }

  return ServerResponse(returnCode, response);
}

void SessionThread::startTls()
{
  QMutexLocker locker(&m_mutex);

  m_socket->setAdvertisedSslVersion(KTcpSocket::TlsV1);
  m_socket->ignoreSslErrors();
  connect(m_socket, SIGNAL(encrypted()), this, SLOT(tlsConnected()));
  m_socket->startClientEncryption();
}

void SessionThread::tlsConnected()
{
  QMutexLocker locker(&m_mutex);
  KSslCipher cipher = m_socket->sessionCipher();

  if ( m_socket->sslErrors().count() > 0 || m_socket->encryptionMode() != KTcpSocket::SslClientMode
       || cipher.isNull() || cipher.usedBits() == 0) {
    qCDebug(KSMTP_LOG) << "Initial SSL handshake failed. cipher.isNull() is" << cipher.isNull()
        << ", cipher.usedBits() is" << cipher.usedBits()
        << ", the socket says:" <<  m_socket->errorString()
        << "and the list of SSL errors contains"
        << m_socket->sslErrors().count() << "items.";
    KSslErrorUiData errorData(m_socket);
    emit sslError(errorData);
  } else {
    qCDebug(KSMTP_LOG) << "TLS negotiation done.";

    QMetaObject::invokeMethod(this, "sendData", Qt::QueuedConnection, Q_ARG(QByteArray, "EHLO " + QUrl::toAce(hostName())));
  }
}
