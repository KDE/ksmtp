/*
  Copyright 2010 BetterInbox <contact@betterinbox.com>
      Author: Gregory Schlomoff <gregory.schlomoff@gmail.com>

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

#ifndef KSMTP_JOB_P_H
#define KSMTP_JOB_P_H

#include "session.h"

namespace KSmtp
{

class SessionPrivate;

class JobPrivate
{
public:
    JobPrivate(Session *session, const QString &name) : m_session(session), m_name(name) { }
    virtual ~JobPrivate() { }

    inline SessionPrivate *sessionInternal()
    {
        return m_session->d;
    }

    inline const SessionPrivate *sessionInternal() const
    {
        return m_session->d;
    }

    Session *m_session = nullptr;
    QString m_name;
};

}

#endif //KSMTP_JOB_P_H
