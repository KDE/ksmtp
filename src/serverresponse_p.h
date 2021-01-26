/*
  SPDX-FileCopyrightText: 2010 BetterInbox <contact@betterinbox.com>
  SPDX-FileContributor: Gregory Schlomoff <gregory.schlomoff@gmail.com>

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef KSMTP_SERVERRESPONSE_P_H
#define KSMTP_SERVERRESPONSE_P_H

#include <QByteArray>

namespace KSmtp
{
class ServerResponse
{
public:
    explicit ServerResponse(int code = 0, const QByteArray &text = QByteArray(), bool multiline = false);
    int code() const;
    QByteArray text() const;
    bool isCode(int other) const;

    bool isMultiline() const;

private:
    QByteArray m_text;
    int m_code;
    bool m_multiline;
};
}

#endif // KSMTP_SERVERRESPONSE_P_H
