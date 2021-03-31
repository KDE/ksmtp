/*
  SPDX-FileCopyrightText: 2010 BetterInbox <contact@betterinbox.com>
  SPDX-FileContributor: Christophe Laveault <christophe@betterinbox.com>
  SPDX-FileContributor: Gregory Schlomoff <gregory.schlomoff@gmail.com>

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

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

    // TODO: (CL) Check if SendJob parses properly the data it gets before sending
};

