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

#include "loginjob.h"
#include "ksmtp_debug.h"
#include "job_p.h"
#include "serverresponse_p.h"
#include "session_p.h"

#include <KLocalizedString>

namespace KSmtp
{

class LoginJobPrivate : public JobPrivate
{
public:
    LoginJobPrivate(LoginJob *job, Session *session, const QString &name)
        : JobPrivate(session, name),
          q(job),
          m_preferedAuthMode(LoginJob::Login),
          m_useTls(false)
    {
    }
    ~LoginJobPrivate() override { }

    LoginJob *q;

    void authenticate();
    void selectAuthentication();
    QByteArray authCommand(LoginJob::AuthMode mode);
    LoginJob::AuthMode authModeFromCommand(const QString &cmd);

    QString m_userName;
    QString m_password;
    QByteArray m_oauthChallenge;
    LoginJob::AuthMode m_preferedAuthMode;
    LoginJob::AuthMode m_usedAuthMode;
    bool m_useTls;
};
}

using namespace KSmtp;

LoginJob::LoginJob(Session *session)
    : Job(*new LoginJobPrivate(this, session, i18n("Login")))
{
}

LoginJob::~LoginJob()
{
}

void LoginJob::setUserName(const QString &userName)
{
    Q_D(LoginJob);
    d->m_userName = userName;
}

void LoginJob::setPassword(const QString &password)
{
    Q_D(LoginJob);
    d->m_password = password;
}

void LoginJob::setOAuthChallenge(const QByteArray &challenge)
{
    Q_D(LoginJob);
    d->m_oauthChallenge = challenge;
}

void LoginJob::setUseTls(bool useTls)
{
    Q_D(LoginJob);
    d->m_useTls = useTls;
}

void LoginJob::setPreferedAuthMode(AuthMode mode)
{
    Q_D(LoginJob);

    if (mode == UnknownAuth) {
        qCWarning(KSMTP_LOG) << "LoginJob: Cannot set preferred authentication mode to Unknown";
        return;
    }
    d->m_preferedAuthMode = mode;
}

LoginJob::AuthMode LoginJob::usedAuthMode() const
{
    Q_D(const LoginJob);
    return d->m_usedAuthMode;
}

void LoginJob::doStart()
{
    Q_D(LoginJob);

    if (session()->allowsTls() && d->m_useTls) {
        sendCommand("STARTTLS");
    } else {
        d->authenticate();
    }
}

void LoginJob::handleResponse(const ServerResponse &r)
{
    Q_D(LoginJob);

    // Handle server errors
    handleErrors(r);

    // Server accepts TLS connection
    if (r.isCode(220)) {
        d->sessionInternal()->startTls();
    }

    // Available authentication mechanisms
    if (r.isCode(25) && r.text().startsWith("AUTH")) { //krazy:exclude=strings
        d->authenticate();
    }

    // Send account data
    if (r.isCode(334) && d->m_usedAuthMode == Login) {
        QByteArray request = QByteArray::fromBase64(r.text());

        if (request.startsWith("Username")) {//krazy:exclude=strings //TODO: Compare with case insensitive
            sendCommand(QByteArray(d->m_userName.toUtf8()).toBase64());
        } else if (request.startsWith("Password")) { //krazy:exclude=strings
            sendCommand(QByteArray(d->m_password.toUtf8()).toBase64());
        }
    }

    // Final agreement
    if (r.isCode(235)) {
        d->sessionInternal()->setState(Session::Authenticated);
        emitResult();
    }
}

void LoginJobPrivate::selectAuthentication()
{
    QStringList availableModes = m_session->availableAuthModes();

    if (availableModes.contains(QString::fromLatin1(authCommand(m_preferedAuthMode)))) {
        m_usedAuthMode = m_preferedAuthMode;
    } else if (availableModes.contains(QString::fromLatin1(authCommand(LoginJob::Login)))) {
        m_usedAuthMode = LoginJob::Login;
    } else if (availableModes.contains(QString::fromLatin1(authCommand(LoginJob::Plain)))) {
        m_usedAuthMode = LoginJob::Plain;
    } else {
        qCWarning(KSMTP_LOG) << "LoginJob: Couldn't choose an authentication method. Please retry with : " << availableModes;
        q->setError(KJob::UserDefinedError);
        q->setErrorText(i18n("Could not authenticate to the SMTP server because no matching authentication method has been found"));
        q->emitResult();
        return;
    }
}

void LoginJobPrivate::authenticate()
{
    selectAuthentication();

    switch (m_usedAuthMode) {
    case LoginJob::Plain:
        q->sendCommand("AUTH PLAIN " + QByteArray('\000' + m_userName.toUtf8() + '\000' + m_password.toUtf8()).toBase64());
        break;
    case LoginJob::Login:
        q->sendCommand("AUTH LOGIN");
        break;
    case LoginJob::CramMD5:
        qCWarning(KSMTP_LOG) << "LoginJob: CramMD5: Not yet implemented";
        break;
    case LoginJob::XOAuth:
        q->sendCommand("AUTH XOAUTH " + m_oauthChallenge);
        break;
    case LoginJob::UnknownAuth:
        break; // Should not happen
    }
}

QByteArray LoginJobPrivate::authCommand(LoginJob::AuthMode mode)
{
    switch (mode) {
    case LoginJob::Plain:
        return "PLAIN";
    case LoginJob::Login:
        return "LOGIN";
    case LoginJob::CramMD5:
        return "CRAM-MD5";
    case LoginJob::XOAuth:
        return "XOAUTH";
    default:
    case LoginJob::UnknownAuth:
        return ""; // Should not happen
    }
}

LoginJob::AuthMode LoginJobPrivate::authModeFromCommand(const QString &cmd)
{
    if (cmd.compare(QLatin1String("PLAIN"), Qt::CaseInsensitive) == 0) {
        return LoginJob::Plain;
    } else if (cmd.compare(QLatin1String("LOGIN"), Qt::CaseInsensitive) == 0) {
        return LoginJob::Login;
    } else if (cmd.compare(QLatin1String("CRAM-MD5"), Qt::CaseInsensitive) == 0) {
        return LoginJob::CramMD5;
    } else if (cmd.compare(QLatin1String("XOAUTH"), Qt::CaseInsensitive) == 0) {
        return LoginJob::XOAuth;
    } else {
        return LoginJob::UnknownAuth;
    }
}

