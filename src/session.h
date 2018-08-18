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
#include "sessionuiproxy.h"

#include <QObject>

namespace KSmtp {
class SessionPrivate;
class SessionThread;

class KSMTP_EXPORT Session : public QObject
{
    Q_OBJECT

public:
    enum State {
        Disconnected = 0, /**< The Session is not connected to the server. */
        Ready,            /**< (internal) */
        Handshake,        /**< (internal) */
        NotAuthenticated, /**< The Session is ready for login. @sa KSmtp::LoginJob */
        Authenticated,    /**< The Session is ready to send email. @sa KSmtp::SendJob */
        Quitting          /**< (internal) */
    };
    Q_ENUM(State)

    /**
      Creates a new SMTP session to the specified host and port.
      After creating the session, call setUseProxy() if necessary
      and then either open() or openAndWait() to open the connection.
      @sa open(), openAndWait()
    */
    explicit Session(const QString &hostName, quint16 port, QObject *parent = nullptr);
    ~Session() override;

    void setUiProxy(const SessionUiProxy::Ptr &uiProxy);
    Q_REQUIRED_RESULT SessionUiProxy::Ptr uiProxy() const;

    /**
      Sets whether the SMTP network connection should use the system proxy settings

      The default is to not use the proxy.
    */
    void setUseNetworkProxy(bool useProxy);

    /**
      Returns the host name that has been provided in the Session's constructor
      @sa port()
    */
    Q_REQUIRED_RESULT QString hostName() const;

    /**
      Returns the port number that has been provided in the Session's constructor
      @sa hostName()
    */
    Q_REQUIRED_RESULT quint16 port() const;

    Q_REQUIRED_RESULT State state() const;

    /**
      Returns true if the SMTP server has indicated that it allows TLS connections, false otherwise.
      The session must be at least in the NotAuthenticated state. Before that, allowsTls() always
      returns false.

      @sa KSmtp::LoginJob::setUseTls()
    */
    Q_REQUIRED_RESULT bool allowsTls() const;

    /**
      @todo: return parsed auth modes, instead of strings.
    */
    Q_REQUIRED_RESULT QStringList availableAuthModes() const;

    /**
      Returns the maximum message size in bytes that the server accepts.
      You can use SendJob::size() to get the size of the message that you are trying to send
      @sa KSmtp::SendJob::size()
    */
    Q_REQUIRED_RESULT int sizeLimit() const;

    Q_REQUIRED_RESULT int socketTimeout() const;
    void setSocketTimeout(int ms);

    /**
     * Custom hostname to send in EHLO/HELO command
     */
    void setCustomHostname(const QString &hostname);
    Q_REQUIRED_RESULT QString customHostname() const;

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
      Requests the server to quit the connection.

      This sends a "QUIT" command to the server and will not close the connection until
      it receives a response. That means you should not delete this object right after
      calling close, instead wait for stateChanged() to change to Disconnected, or use
      quitAndWait().

      See RFC 821, Chapter 4.1.1, "QUIT".

      @sa quitAndWait()
    */
    void quit();

    /**
      Requests the server to quit the connection and blocks the execution until the
      server replies and closes the connection.

      See RFC 821, Chapter 4.1.1, "QUIT".

      @sa quit()
    */
    void quitAndWait();

Q_SIGNALS:
    void stateChanged(KSmtp::Session::State state);
    void connectionError(const QString &error);

private:
    friend class SessionPrivate;
    friend class SessionThread;
    friend class JobPrivate;

    SessionPrivate *const d;
};
}

#endif // KSMTP_SESSION_H
