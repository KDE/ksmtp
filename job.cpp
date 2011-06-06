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

#include "job.h"
#include "job_p.h"
#include "serverresponse_p.h"
#include "session_p.h"

#include <KDE/KLocale>

using namespace KSmtp;

Job::Job(Session *session)
  : KJob(session), 
    d_ptr(new JobPrivate(session, i18n("Job")))
{
}

Job::Job( JobPrivate &dd )
  : KJob( dd.m_session ), d_ptr(&dd)
{
}

Job::~Job()
{
  delete d_ptr;
}

Session *Job::session() const
{
  Q_D(const Job);
  return d->m_session;
}

void Job::start()
{
  Q_D(Job);
  d->sessionInternal()->addJob(this);
}

void Job::sendCommand(const QByteArray &cmd)
{
  Q_D(Job);
  d->sessionInternal()->sendData(cmd);
}

void Job::handleErrors(const ServerResponse &r)
{
  if (r.isCode(4) || r.isCode(5)) {
    setError(KJob::UserDefinedError);
    if (r.isCode(4)) {
      setErrorText(i18n("Server time out"));
    } else {
      setErrorText(i18n("Server error"));
    }
    emitResult();
  }
}

