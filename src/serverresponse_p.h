/*
  SPDX-FileCopyrightText: 2010 BetterInbox <contact@betterinbox.com>
  SPDX-FileContributor: Gregory Schlomoff <gregory.schlomoff@gmail.com>

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QByteArray>

namespace KSmtp
{
class ServerResponse
{
public:
    explicit ServerResponse(int code = 0, const QByteArray &text = QByteArray(), bool multiline = false);
    Q_REQUIRED_RESULT int code() const;
    Q_REQUIRED_RESULT QByteArray text() const;
    Q_REQUIRED_RESULT bool isCode(int other) const;

    Q_REQUIRED_RESULT bool isMultiline() const;

private:
    QByteArray m_text;
    int m_code;
    bool m_multiline;
};
}

