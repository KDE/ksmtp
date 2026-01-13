/*
  SPDX-FileCopyrightText: 2010 BetterInbox <contact@betterinbox.com>
  SPDX-FileContributor: Christophe Laveault <christophe@betterinbox.com>
  SPDX-FileContributor: Gregory Schlomoff <gregory.schlomoff@gmail.com>

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include "ksmtp_export.h"
#include "sessionuiproxy.h"

#include <QObject>

namespace KSmtp
{
class SessionPrivate;
class SessionThread;
/*!
 * \class KSmtp::Session
 * \inmodule KSMTP
 * \inheaderfile KSMTP/Session
 *
 * \brief The Session class
 */
class KSMTP_EXPORT Session : public QObject
{
    Q_OBJECT

public:
    enum State {
        Disconnected = 0, /*!< The Session is not connected to the server. */
        Ready, /*!< (internal) */
        Handshake, /*!< (internal) */
        NotAuthenticated, /*!< The Session is ready for login. @sa KSmtp::LoginJob */
        Authenticated, /*!< The Session is ready to send email. @sa KSmtp::SendJob */
        Quitting /*!< (internal) */
    };
    Q_ENUM(State)

    /*!
      Various possible values for the "EncryptionMode". Transport encryption for a session.
      \value Unencrypted Use no encryption.
      \value TLS Use TLS encryption on the socket.
      \value STARTTLS Use STARTTLS to upgrade an unencrypted connection to encrypted after the initial handshake.
    */
    enum EncryptionMode {
        Unencrypted,
        TLS,
        STARTTLS
    };
    Q_ENUM(EncryptionMode)

    /*!
      Creates a new SMTP session to the specified host and port.
      After creating the session, call setUseNetworkProxy() if necessary
      and then either open() to open the connection.
      @sa open()
    */
    explicit Session(const QString &hostName, quint16 port, QObject *parent = nullptr);
    ~Session() override;

    void setUiProxy(const SessionUiProxy::Ptr &uiProxy);
    [[nodiscard]] SessionUiProxy::Ptr uiProxy() const;

    /*!
      Sets whether the SMTP network connection should use the system proxy settings

      The default is to not use the proxy.
    */
    void setUseNetworkProxy(bool useProxy);

    /*!
      Returns the host name that has been provided in the Session's constructor
      @sa port()
    */
    [[nodiscard]] QString hostName() const;

    /*!
      Returns the port number that has been provided in the Session's constructor
      @sa hostName()
    */
    [[nodiscard]] quint16 port() const;

    [[nodiscard]] State state() const;

    /*! Returns the requested encryption mode for this session. */
    [[nodiscard]] EncryptionMode encryptionMode() const;

    /*! Sets the encryption mode for this session.
     *  Has to be called before open().
     */
    void setEncryptionMode(EncryptionMode mode);

    /*!
      Returns true if the SMTP server has indicated that it allows TLS connections, false otherwise.
      The session must be at least in the NotAuthenticated state. Before that, allowsTls() always
      returns false.

      @sa KSmtp::LoginJob::setUseTls()
    */
    [[nodiscard]] bool allowsTls() const;

    /*!
      Returns true if the SMTP server has indicated that it allows Delivery Status Notification (DSN) support, false otherwise.
    */
    [[nodiscard]] bool allowsDsn() const;

    /*!
      @todo: return parsed auth modes, instead of strings.
    */
    [[nodiscard]] QStringList availableAuthModes() const;

    /*!
      Returns the maximum message size in bytes that the server accepts.
      You can use SendJob::size() to get the size of the message that you are trying to send
      @sa KSmtp::SendJob::size()
    */
    [[nodiscard]] int sizeLimit() const;

    /*!
     * \brief socketTimeout
     * \return
     */
    [[nodiscard]] int socketTimeout() const;
    /*!
     * \brief setSocketTimeout
     * \param ms
     */
    void setSocketTimeout(int ms);

    /*!
     * Custom hostname to send in EHLO/HELO command
     */
    void setCustomHostname(const QString &hostname);
    [[nodiscard]] QString customHostname() const;

    /*!
      Opens the connection to the server.

      You should connect to stateChanged() before calling this method, and wait until the session's
      state is NotAuthenticated (Session is ready for a LoginJob) or Disconnected (connecting to the
      server failed)

      Make sure to call \\sa setEncryptionMode() before.

      \sa setEncryptionMode
    */
    void open();

    /*!
      Requests the server to quit the connection.

      This sends a "QUIT" command to the server and will not close the connection until
      it receives a response. That means you should not delete this object right after
      calling close, instead wait for stateChanged() to change to Disconnected.

      See RFC 821, Chapter 4.1.1, "QUIT".
    */
    void quit();

Q_SIGNALS:
    /*!
     */
    void stateChanged(KSmtp::Session::State state);
    /*!
     */
    void connectionError(const QString &error);

private:
    friend class SessionPrivate;
    friend class SessionThread;
    friend class JobPrivate;

    SessionPrivate *const d;
};
}
