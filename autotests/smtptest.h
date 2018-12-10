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
  License along with this library.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef KSMTP_SMTPTEST_H
#define KSMTP_SMTPTEST_H

#include "QObject"

class SmtpTest : public QObject
{
    Q_OBJECT

public:
    SmtpTest();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testHello();
    void testHello_data();

    void testLoginJob();
    void testLoginJob_data();

    void testSendJob();
    void testSendJob_data();

    //TODO: (CL) Check if SendJob parses properly the data it gets before sending
};

#endif // KSMTP_SMTPTEST_H
