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
  License along with this library.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef KSMTP_SENDJOB_H
#define KSMTP_SENDJOB_H

#include "ksmtp_export.h"

#include "job.h"

namespace KSmtp {
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

#endif // KSMTP_SENDJOB_H
