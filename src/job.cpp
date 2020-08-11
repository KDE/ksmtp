/*
  SPDX-FileCopyrightText: 2010 BetterInbox <contact@betterinbox.com>
  SPDX-FileContributor: Christophe Laveault <christophe@betterinbox.com>
  SPDX-FileContributor: Gregory Schlomoff <gregory.schlomoff@gmail.com>

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "job.h"
#include "job_p.h"
#include "serverresponse_p.h"
#include "session_p.h"

#include <KLocalizedString>

using namespace KSmtp;

Job::Job(Session *session)
    : KJob(session)
    , d_ptr(new JobPrivate(session, QStringLiteral("Job")))
{
}

Job::Job(JobPrivate &dd)
    : KJob(dd.m_session)
    , d_ptr(&dd)
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
        // https://www.ietf.org/rfc/rfc2821.txt
        // We could just use r.text(), but that might not be in the user's language, so try and prepend a translated message.
        const QString serverText = QString::fromUtf8(r.text());
        if (r.code() == 421) {
            setErrorText(i18n("Service not available")); // e.g. the server is shutting down
        } else if (r.code() == 450 || r.code() == 550) {
            setErrorText(i18n("Mailbox unavailable. The server said: %1", serverText));
        } else if (r.code() == 452 || r.code() == 552) {
            setErrorText(i18n("Insufficient storage space on server. The server said: %1", serverText));
        } else {
            setErrorText(i18n("Server error: %1", serverText));
        }
        emitResult();
    }
}

void Job::connectionLost()
{
    setError(KJob::UserDefinedError);
    setErrorText(i18n("Connection to server lost."));
    emitResult();
}
