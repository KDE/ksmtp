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

extern "C" {
#include <sasl/sasl.h>
}

namespace {

static const sasl_callback_t callbacks[] = {
    { SASL_CB_ECHOPROMPT, nullptr, nullptr },
    { SASL_CB_NOECHOPROMPT, nullptr, nullptr },
    { SASL_CB_GETREALM, nullptr, nullptr },
    { SASL_CB_USER, nullptr, nullptr },
    { SASL_CB_AUTHNAME, nullptr, nullptr },
    { SASL_CB_PASS, nullptr, nullptr },
    { SASL_CB_CANON_USER, nullptr, nullptr },
    { SASL_CB_LIST_END, nullptr , nullptr }
};

}

namespace KSmtp
{

class LoginJobPrivate : public JobPrivate
{
public:
    LoginJobPrivate(LoginJob *job, Session *session, const QString &name)
        : JobPrivate(session, name)
        , m_preferedAuthMode(LoginJob::Login)
        , m_actualAuthMode(LoginJob::UnknownAuth)
        , m_encryptionMode(LoginJob::Unencrypted)
        , m_saslConn(nullptr)
        , m_saslClient(nullptr)
        , q(job)
    {
    }

    ~LoginJobPrivate() override { }

    bool sasl_interact();
    bool sasl_init();
    bool sasl_challenge(const QByteArray &data);

    bool authenticate();
    bool selectAuthentication();

    LoginJob::AuthMode authModeFromCommand(const QByteArray &mech) const;
    QByteArray authCommand(LoginJob::AuthMode mode) const;
    LoginJob::EncryptionMode sslVersionToEncryption(KTcpSocket::SslVersion version) const;
    KTcpSocket::SslVersion encryptionToSslVersion(LoginJob::EncryptionMode mode) const;

    QString m_userName;
    QString m_password;
    LoginJob::AuthMode m_preferedAuthMode;
    LoginJob::AuthMode m_actualAuthMode;
    LoginJob::EncryptionMode m_encryptionMode;

    sasl_conn_t *m_saslConn = nullptr;
    sasl_interact_t *m_saslClient = nullptr;

private:
    LoginJob * const q;
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

void LoginJob::setPreferedAuthMode(AuthMode mode)
{
    Q_D(LoginJob);

    if (mode == UnknownAuth) {
        qCWarning(KSMTP_LOG) << "LoginJob: Cannot set preferred authentication mode to Unknown";
        return;
    }
    d->m_preferedAuthMode = mode;
}

void LoginJob::setEncryptionMode(EncryptionMode mode)
{
    Q_D(LoginJob);
    d->m_encryptionMode = mode;
}

LoginJob::EncryptionMode LoginJob::encryptionMode() const
{
    return d_func()->m_encryptionMode;
}

void LoginJob::doStart()
{
    Q_D(LoginJob);

    const auto negotiatedEnc = d->sessionInternal()->negotiatedEncryption();
    if (negotiatedEnc != KTcpSocket::UnknownSslVersion) {
        // Socket already encrypted, pretend we did not want any
        if (d->m_encryptionMode == d->sslVersionToEncryption(negotiatedEnc)) {
            d->m_encryptionMode = Unencrypted;
        }
    }

    if (d->m_encryptionMode == SslV2 || d->m_encryptionMode == SslV3
        || d->m_encryptionMode == TlsV1SslV3 || d->m_encryptionMode == AnySslVersion) {
        d->sessionInternal()->startSsl(d->encryptionToSslVersion(d->m_encryptionMode));
    } else if (d->m_encryptionMode == TlsV1 && session()->allowsTls()) {
        sendCommand(QByteArrayLiteral("STARTTLS"));
    } else {
        if (!d->authenticate()) {
            emitResult();
        }
    }
}

void LoginJob::handleResponse(const ServerResponse &r)
{
    Q_D(LoginJob);

    // Handle server errors
    handleErrors(r);

    // Server accepts TLS connection
    if (r.isCode(220)) {
        d->sessionInternal()->startSsl(d->encryptionToSslVersion(d->m_encryptionMode));
        return;
    }

    // Available authentication mechanisms
    if (r.isCode(25) && r.text().startsWith("AUTH ")) { //krazy:exclude=strings
        d->sessionInternal()->setAuthenticationMethods(r.text().remove(0, QByteArray("AUTH ").count()).split(' '));
        d->authenticate();
        return;
    }

    // Send account data
    if (r.isCode(334)) {
        if (d->m_actualAuthMode == Plain) {
            const QByteArray challengeResponse = '\0' + d->m_userName.toUtf8() +
                                                 '\0' + d->m_password.toUtf8();
            sendCommand(challengeResponse.toBase64());
        } else {
            if (!d->sasl_challenge(QByteArray::fromBase64(r.text()))) {
                emitResult();
            }
        }
        return;
    }

    // Final agreement
    if (r.isCode(235)) {
        d->sessionInternal()->setState(Session::Authenticated);
        emitResult();
    }
}

bool LoginJobPrivate::selectAuthentication()
{
    const QStringList availableModes = m_session->availableAuthModes();

    if (availableModes.contains(QString::fromLatin1(authCommand(m_preferedAuthMode)))) {
        m_actualAuthMode = m_preferedAuthMode;
    } else if (availableModes.contains(QString::fromLatin1(authCommand(LoginJob::Login)))) {
        m_actualAuthMode = LoginJob::Login;
    } else if (availableModes.contains(QString::fromLatin1(authCommand(LoginJob::Plain)))) {
        m_actualAuthMode = LoginJob::Plain;
    } else {
        qCWarning(KSMTP_LOG) << "LoginJob: Couldn't choose an authentication method. Please retry with : " << availableModes;
        q->setError(KJob::UserDefinedError);
        q->setErrorText(i18n("Could not authenticate to the SMTP server because no matching authentication method has been found"));
        return false;
    }

    return true;
}

bool LoginJobPrivate::sasl_init()
{
    if (sasl_client_init(nullptr) != SASL_OK) {
        qCWarning(KSMTP_LOG) << "Failed to initialize SASL";
        return false;
    }
    return true;
}

bool LoginJobPrivate::sasl_interact()
{
    sasl_interact_t *interact = m_saslClient;

    while (interact->id != SASL_CB_LIST_END) {
        qCDebug(KSMTP_LOG) << "SASL_INTERACT Id" << interact->id;
        switch (interact->id) {
        case SASL_CB_AUTHNAME:
        //case SASL_CB_USER:
            qCDebug(KSMTP_LOG) << "SASL_CB_[USER|AUTHNAME]: '" << m_userName << "'";
            interact->result = strdup(m_userName.toUtf8().constData());
            interact->len = strlen((const char *) interact->result);
            break;
        case SASL_CB_PASS:
            qCDebug(KSMTP_LOG) << "SASL_CB_PASS: [hidden]";
            interact->result = strdup(m_password.toUtf8().constData());
            interact->len = strlen((const char *) interact->result);
            break;
        default:
            interact->result = nullptr;
            interact->len = 0;
            break;
        }
        ++interact;
    }

    return true;
}

bool LoginJobPrivate::sasl_challenge(const QByteArray &challenge)
{
    int result = -1;
    const char *out = nullptr;
    uint outLen = 0;

    Q_FOREVER {
        result = sasl_client_step(m_saslConn, challenge.isEmpty() ? nullptr : challenge.constData(),
                                  challenge.size(), &m_saslClient, &out, &outLen);
        if (result == SASL_INTERACT) {
            if (!sasl_interact()){
                q->setError(LoginJob::UserDefinedError);
                sasl_dispose(&m_saslConn);
                return false;
            }
        }  else {
            break;
        }
    }

    if (result != SASL_OK && result != SASL_CONTINUE) {
        const QString saslError = QString::fromUtf8(sasl_errdetail(m_saslConn));
        qCWarning(KSMTP_LOG) << "sasl_client_step failed: " << result << saslError;
        q->setError(LoginJob::UserDefinedError);
        q->setErrorText(saslError);
        sasl_dispose(&m_saslConn);
        return false;
    }

    q->sendCommand(QByteArray::fromRawData(out, outLen).toBase64());

    return true;
}

bool LoginJobPrivate::authenticate()
{
    if (!selectAuthentication()) {
        return false;
    }

    if (!sasl_init()) {
        q->setError(LoginJob::UserDefinedError);
        q->setErrorText(i18n("Login failed, cannot initialize the SASL library"));
        return false;
    }

    int result = sasl_client_new("smtp", m_session->hostName().toUtf8().constData(),
                                 nullptr, nullptr, callbacks, 0, &m_saslConn);
    if (result != SASL_OK) {
        const auto saslError = QString::fromUtf8(sasl_errdetail(m_saslConn));
        q->setError(LoginJob::UserDefinedError);
        q->setErrorText(saslError);
        return false;
    }

    uint outLen = 0;
    const char *out = nullptr;
    const char *actualMech = nullptr;
    const auto authMode = authCommand(m_actualAuthMode);

    Q_FOREVER {
        qCDebug(KSMTP_LOG) << "Trying authmod" << authMode;
        result = sasl_client_start(m_saslConn, authMode.constData(),
                                   &m_saslClient, &out, &outLen, &actualMech);
        if (result == SASL_INTERACT) {
            if (!sasl_interact()) {
                sasl_dispose(&m_saslConn);
                q->setError(LoginJob::UserDefinedError);
                return false;
            }
        } else {
            break;
        }
    }

    m_actualAuthMode = authModeFromCommand(actualMech);

    if (result != SASL_CONTINUE && result != SASL_OK) {
        const auto saslError = QString::fromUtf8(sasl_errdetail(m_saslConn));
        qCWarning(KSMTP_LOG) << "sasl_client_start failed with:" << result << saslError;
        q->setError(LoginJob::UserDefinedError);
        q->setErrorText(saslError);
        sasl_dispose(&m_saslConn);
        return false;
    }

    if (outLen == 0) {
        q->sendCommand("AUTH " + authMode);
    } else {
        q->sendCommand("AUTH " + authMode + ' ' + QByteArray::fromRawData(out, outLen).toBase64());
    }

    return true;
}

LoginJob::AuthMode LoginJobPrivate::authModeFromCommand(const QByteArray &mech) const
{
    if (qstrnicmp(mech.constData(), "PLAIN", 5) == 0) {
        return LoginJob::Plain;
    } else if (qstrnicmp(mech.constData(), "LOGIN", 5) == 0) {
        return LoginJob::Login;
    } else if (qstrnicmp(mech.constData(), "CRAM-MD5", 8) == 0) {
        return LoginJob::CramMD5;
    } else if (qstrnicmp(mech.constData(), "DIGEST-MD5", 10) == 0) {
        return LoginJob::DigestMD5;
    } else if (qstrnicmp(mech.constData(), "GSSAPI", 6) == 0) {
        return LoginJob::GSSAPI;
    } else if (qstrnicmp(mech.constData(), "NTLM", 4) == 0) {
        return LoginJob::NTLM;
    } else if (qstrnicmp(mech.constData(), "ANONYMOUS", 9) == 0) {
        return LoginJob::Anonymous;
    } else if (qstrnicmp(mech.constData(), "XOAUTH", 6) == 0) {
        return LoginJob::XOAuth;
    } else {
        return LoginJob::UnknownAuth;
    }
}

QByteArray LoginJobPrivate::authCommand(LoginJob::AuthMode mode) const
{
    switch (mode) {
    case LoginJob::Plain:
        return QByteArrayLiteral("PLAIN");
    case LoginJob::Login:
        return QByteArrayLiteral("LOGIN");
    case LoginJob::CramMD5:
        return QByteArrayLiteral("CRAM-MD5");
    case LoginJob::DigestMD5:
        return QByteArrayLiteral("DIGEST-MD5");
    case LoginJob::GSSAPI:
        return QByteArrayLiteral("GSSAPI");
    case LoginJob::NTLM:
        return QByteArrayLiteral("NTLM");
    case LoginJob::Anonymous:
        return QByteArrayLiteral("ANONYMOUS");
    case LoginJob::XOAuth:
        return QByteArrayLiteral( "XOAUTH");
    default:
    case LoginJob::UnknownAuth:
        return ""; // Should not happen
    }
}

LoginJob::EncryptionMode LoginJobPrivate::sslVersionToEncryption(KTcpSocket::SslVersion version) const
{
    switch (version) {
    case KTcpSocket::TlsV1:
        return LoginJob::TlsV1;
    case KTcpSocket::SslV2:
        return LoginJob::SslV2;
    case KTcpSocket::SslV3:
        return LoginJob::SslV3;
    case KTcpSocket::TlsV1SslV3:
        return LoginJob::TlsV1SslV3;
    case KTcpSocket::AnySslVersion:
        return LoginJob::AnySslVersion;
    default:
        qCWarning(KSMTP_LOG) << "Unkown SSL version" << version;
        return LoginJob::Unencrypted;
    }
}

KTcpSocket::SslVersion LoginJobPrivate::encryptionToSslVersion(LoginJob::EncryptionMode mode) const
{
    switch (mode) {
    case LoginJob::SslV2:
        return KTcpSocket::SslV2;
    case LoginJob::SslV3:
        return KTcpSocket::SslV3;
    case LoginJob::TlsV1SslV3:
        return KTcpSocket::TlsV1SslV3;
    case LoginJob::TlsV1:
        return KTcpSocket::TlsV1;
    case LoginJob::AnySslVersion:
        return KTcpSocket::AnySslVersion;
    default:
        return KTcpSocket::UnknownSslVersion;
    }
}
