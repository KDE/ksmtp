/*
  SPDX-FileCopyrightText: 2010 BetterInbox <contact@betterinbox.com>
  SPDX-FileContributor: Christophe Laveault <christophe@betterinbox.com>
  SPDX-FileContributor: Gregory Schlomoff <gregory.schlomoff@gmail.com>

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include "ksmtp_export.h"

#include <KJob>

namespace KSmtp
{
class Session;
class SessionPrivate;
class JobPrivate;
class ServerResponse;
/*!
 * \class KSmtp::Job
 * \inmodule KSMTP
 * \inheaderfile KSMTP/Job
 *
 * \brief The Job class
 */
class KSMTP_EXPORT Job : public KJob
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Job)

    friend class SessionPrivate;

public:
    /*!
     */
    ~Job() override;

    /*!
     */
    [[nodiscard]] Session *session() const;
    /*!
     */
    void start() override;

protected:
    /*!
     */
    void sendCommand(const QByteArray &cmd);
    /*!
     */
    virtual void doStart() = 0;
    /*!
     */
    virtual void handleResponse(const ServerResponse &response) = 0;
    /*!
     */
    void handleErrors(const ServerResponse &response);
    /*!
     */
    void connectionLost();

    explicit Job(Session *session);
    explicit Job(JobPrivate &dd);

    std::unique_ptr<JobPrivate> const d_ptr;
};
}
