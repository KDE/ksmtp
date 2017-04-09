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

namespace KSmtp {

class Job;
class SessionThread;
class ServerResponse;

class KSMTP_EXPORT SessionPrivate : public QObject
{
  Q_OBJECT

  friend class Session;

public:
  explicit SessionPrivate( Session *session );
  ~SessionPrivate() override;
  
  void addJob(Job *job);
  void sendData(const QByteArray &data);
  void setState(Session::State s);
  void startTls();
  
private Q_SLOTS:
  void doStartNext();
  void jobDone(KJob *job);
  void jobDestroyed(QObject *job);
  
  void responseReceived(const ServerResponse &response);
  void socketConnected();
  void socketDisconnected();
  void onSocketTimeout();
  
private:

  void startNext();
  void startSocketTimer();
  void stopSocketTimer();
  void restartSocketTimer();
  
  Session *const q;
  
  // Smtp session
  Session::State m_state;
  SessionThread *m_thread;
  int m_socketTimerInterval;
  QTimer m_socketTimer;
  QEventLoop *m_startLoop;

  // Jobs
  bool m_jobRunning;
  Job *m_currentJob;
  QQueue<Job*> m_queue;

  // Smtp info
  bool m_ehloRejected;
  int m_size;
  bool m_allowsTls;
  QStringList m_authModes;
};

}

#endif //KSMTP_SESSION_P_H
