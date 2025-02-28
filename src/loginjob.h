/*
  SPDX-FileCopyrightText: 2010 BetterInbox <contact@betterinbox.com>
  SPDX-FileContributor: Christophe Laveault <christophe@betterinbox.com>
  SPDX-FileContributor: Gregory Schlomoff <gregory.schlomoff@gmail.com>

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include "ksmtp_export.h"

#include "job.h"

namespace KSmtp
{
class LoginJobPrivate;
/**
 * @brief The LoginJob class
 */
class KSMTP_EXPORT LoginJob : public Job
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(LoginJob)

public:
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
    [[nodiscard]] AuthMode usedAuthMode() const;

protected:
    void doStart() override;
    void handleResponse(const ServerResponse &r) override;
};
}
