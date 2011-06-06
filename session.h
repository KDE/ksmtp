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

#ifndef KSMTP_SESSION_H
#define KSMTP_SESSION_H

#include "ksmtp_export.h"

#include <QObject>

class KJob;
class QEventLoop;

namespace KSmtp {

class SessionPrivate;

class KSMTP_EXPORT Session : public QObject
{
  Q_OBJECT

public:
  enum State { 
    Disconnected = 0, /**< The Session is not connected to the server. */
    Ready,            /**< (internal) */
    Handshake,        /**< (internal) */
    NotAuthenticated, /**< The Session is ready for login. @sa KSmtp::LoginJob */
    Authenticated     /**< The Session is ready to send email. @sa KSmtp::SendJob */
  };

  /**
    Creates a new Smtp session to the specified host and port.
    After creating the session, you should call either open() or openAndWait() to open the connection.
    @sa open(), openAndWait()
  */
  explicit Session(QString hostName, quint16 port, QObject *parent = 0);
  ~Session();

  /**
    Returns the host name that has been provided in the Session's constructor
    @sa port()
  */
  QString hostName() const;
  
  /**
    Returns the port number that has been provided in the Session's constructor
    @sa hostName()
  */
  quint16 port() const;
  
  State state() const;
  
  /**
    Returns true if the SMTP server has indicated that it allows TLS connections, false otherwise.
    The session must be at least in the NotAuthenticated state. Before that, allowsTls() always 
    returns false.
    
    @sa KSmtp::LoginJob::setUseTls()
  */
  bool allowsTls() const;
  
  /**
    @todo: return parsed auth modes, instead of strings.
  */
  QStringList availableAuthModes() const;
  
  /**
    Returns the maximum message size in bytes that the server accepts.
    You can use SendJob::size() to get the size of the message that you are trying to send
    @sa KSmtp::SendJob::size()
  */
  int sizeLimit() const;

  int socketTimeout() const;
  void setSocketTimeout(int ms);
  
  /**
    Opens the connection to the server.
    
    You should connect to stateChanged() before calling this method, and wait until the session's
    state is NotAuthenticated (Session is ready for a LoginJob) or Disconnected (connecting to the 
    server failed)
    
    @sa openAndWait()
  */
  void open();
  
  /**
    Opens the connection to the server and blocks the execution until the Session is in the 
    NotAuthenticated state (ready for a LoginJob) or Disconnected (connecting to the server failed)
    
    @sa open()
  */
  void openAndWait();
  
  /**
    Closes the connection.
  */
  void close();

signals:
  void stateChanged(KSmtp::Session::State state);

private:
  friend class SessionPrivate;
  friend class SessionThread;
  friend class JobPrivate;
  
  SessionPrivate *const d;
};

}

#endif // KSMTP_SESSION_H
