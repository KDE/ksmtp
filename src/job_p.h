/*
  SPDX-FileCopyrightText: 2010 BetterInbox <contact@betterinbox.com>
  SPDX-FileContributor: Gregory Schlomoff <gregory.schlomoff@gmail.com>

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include "session.h"

namespace KSmtp
{
class SessionPrivate;

class JobPrivate
{
public:
    JobPrivate(Session *session, const QString &name)
        : m_session(session)
        , m_name(name)
    {
    }

    virtual ~JobPrivate()
    {
    }

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

