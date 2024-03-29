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
    [[nodiscard]] int code() const;
    [[nodiscard]] QByteArray text() const;
    [[nodiscard]] bool isCode(int other) const;

    [[nodiscard]] bool isMultiline() const;

private:
    QByteArray m_text;
    int m_code;
    bool m_multiline;
};
}
