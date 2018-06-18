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

#ifndef KSMTP_JOB_H
#define KSMTP_JOB_H

#include "ksmtp_export.h"

#include <KJob>

namespace KSmtp
{

class Session;
class SessionPrivate;
class JobPrivate;
class ServerResponse;

class KSMTP_EXPORT Job : public KJob
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Job)

    friend class SessionPrivate;

public:
    ~Job() override;

    Q_REQUIRED_RESULT Session *session() const;
    void start() override;

protected:
    void sendCommand(const QByteArray &cmd);
    virtual void doStart() = 0;
    virtual void handleResponse(const ServerResponse &response) = 0;
    void handleErrors(const ServerResponse &response);
    void connectionLost();

    explicit Job(Session *session);
    explicit Job(JobPrivate &dd);


    JobPrivate *const d_ptr;
};

}

#endif
