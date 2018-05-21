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

#ifndef KSMTP_LOGINJOB_H
#define KSMTP_LOGINJOB_H

#include "ksmtp_export.h"

#include "job.h"

namespace KSmtp
{

class LoginJobPrivate;

class KSMTP_EXPORT LoginJob : public Job
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(LoginJob)

public:
    enum EncryptionMode {
        Unencrypted,
        TlsV1,
        SslV2,
        SslV3,
        TlsV1SslV3,
        AnySslVersion
    };

    enum AuthMode {
        UnknownAuth,
        Plain,
        Login,
        CramMD5,
        DigestMD5,
        NTLM,
        GSSAPI,
        Anonymous,
        XOAuth2
    };

    enum LoginError {
        TokenExpired = KJob::UserDefinedError + 1
    };

    explicit LoginJob(Session *session);
    ~LoginJob() override;

    void setUserName(const QString &userName);
    void setPassword(const QString &password);

    void setPreferedAuthMode(AuthMode mode);
    AuthMode usedAuthMode() const;

    void setEncryptionMode(EncryptionMode mode);
    EncryptionMode encryptionMode() const;

protected:
    void doStart() override;
    void handleResponse(const ServerResponse &r) override;
};

}

#endif // KSMTP_LOGINJOB_H
