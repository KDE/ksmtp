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

#ifndef KSMTP_SERVERRESPONSE_P_H
#define KSMTP_SERVERRESPONSE_P_H

#include <QByteArray>

namespace KSmtp
{

class ServerResponse
{
public:
    explicit ServerResponse(int code = 0, const QByteArray &text = QByteArray(),
                            bool multiline = false);
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

#endif //KSMTP_SERVERRESPONSE_P_H
