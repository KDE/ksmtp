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
class SendJobPrivate;
/**
 * @brief The SendJob class
 */
class KSMTP_EXPORT SendJob : public Job
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(SendJob)

public:
    explicit SendJob(Session *session);

    /**
     * Set the sender email address
     */
    void setFrom(const QString &from);

    /**
     * Add recipients.
     *
     */
    void setTo(const QStringList &to);

    /**
     * Add recipients.
     */
    void setCc(const QStringList &cc);

    /**
     * Add recipients.
     */
    void setBcc(const QStringList &bcc);

    /**
     * Set the actual message data.
     */
    void setData(const QByteArray &data);

    /**
     * Returns size of the encoded message data.
     */
    Q_REQUIRED_RESULT int size() const;

protected:
    void doStart() override;
    void handleResponse(const ServerResponse &r) override;
};
}

