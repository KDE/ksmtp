/*
  SPDX-FileCopyrightText: 2010 BetterInbox <contact@betterinbox.com>
  SPDX-FileContributor: Christophe Laveault <christophe@betterinbox.com>
  SPDX-FileContributor: Gregory Schlomoff <gregory.schlomoff@gmail.com>

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "smtptest.h"

#include "fakeserver.h"
#include "loginjob.h"
#include "sendjob.h"
#include "session.h"
#include <QTest>

void SmtpTest::testHello_data()
{
    QTest::addColumn<QList<QByteArray>>("scenario");
    QTest::addColumn<QString>("hostname");

    QList<QByteArray> scenario;
    scenario << FakeServer::greeting() << "C: EHLO 127.0.0.1"
             << "S: 250 Localhost ready to roll" << FakeServer::bye();
    QTest::newRow("EHLO OK") << scenario << QStringLiteral("127.0.0.1");

    scenario.clear();
    scenario << FakeServer::greeting() << "C: EHLO 127.0.0.1"
             << "S: 500 Command was not recognized"
             << "C: HELO 127.0.0.1"
             << "S: 250 Localhost ready to roll" << FakeServer::bye();
    QTest::newRow("EHLO unknown") << scenario << QStringLiteral("127.0.0.1");

    scenario.clear();
    scenario << FakeServer::greeting() << "C: EHLO 127.0.0.1"
             << "S: 502 Command not implemented"
             << "C: HELO 127.0.0.1"
             << "S: 250 Localhost ready to roll" << FakeServer::bye();
    QTest::newRow("EHLO not implemented") << scenario << QStringLiteral("127.0.0.1");

    scenario.clear();
    scenario << FakeServer::greeting() << "C: EHLO 127.0.0.1"
             << "S: 502 Command not implemented"
             << "C: HELO 127.0.0.1"
             << "S: 500 Command was not recognized" << FakeServer::bye();
    QTest::newRow("ERROR") << scenario << QStringLiteral("127.0.0.1");

    scenario.clear();
    scenario << FakeServer::greeting() << "C: EHLO random.stranger"
             << "S: 250 random.stranger ready to roll" << FakeServer::bye();
    QTest::newRow("EHLO hostname") << scenario << QStringLiteral("random.stranger");
}

void SmtpTest::testHello()
{
    QFETCH(QList<QByteArray>, scenario);
    QFETCH(QString, hostname);

    FakeServer fakeServer;
    fakeServer.setScenario(scenario);
    fakeServer.startAndWait();
    KSmtp::Session session(QStringLiteral("127.0.0.1"), 5989);
    session.setCustomHostname(hostname);
    session.openAndWait();

    qDebug() << "### Session state is:" << session.state();

    QEXPECT_FAIL("ERROR", "Expected failure if HELO command not recognized", Continue);
    QVERIFY2(session.state() == KSmtp::Session::NotAuthenticated, "Handshake failed");

    session.quitAndWait();

    QVERIFY(fakeServer.isAllScenarioDone());
    fakeServer.quit();
}

void SmtpTest::testLoginJob_data()
{
    QTest::addColumn<QList<QByteArray>>("scenario");
    QTest::addColumn<QString>("authMode");
    QTest::addColumn<int>("errorCode");

    QList<QByteArray> scenario;
    scenario << FakeServer::greetingAndEhlo() << "S: 250 AUTH PLAIN LOGIN"
             << "C: AUTH PLAIN AGxvZ2luAHBhc3N3b3Jk" // [\0 + "login" + \0 + "password"].toBase64()
             << "S: 235 Authenticated" << FakeServer::bye();
    QTest::newRow("Plain auth ok") << scenario << "Plain" << 0;

    scenario.clear();
    scenario << FakeServer::greetingAndEhlo() << "S: 250 AUTH PLAIN LOGIN"
             << "C: AUTH LOGIN"
             << "S: 334 VXNlcm5hbWU6" // "Username:".toBase64()
             << "C: bG9naW4=" // "login".toBase64()
             << "S: 334 UGFzc3dvcmQ6" // "Password:".toBase64()
             << "C: cGFzc3dvcmQ=" // "password".toBase64()
             << "S: 235 Authenticated" << FakeServer::bye();
    QTest::newRow("Login auth ok") << scenario << "Login" << 0;

    scenario.clear();
    scenario << FakeServer::greetingAndEhlo() << "S: 250 AUTH PLAIN"
             << "C: AUTH PLAIN AGxvZ2luAHBhc3N3b3Jk" // [\0 + "login" + \0 + "password"].toBase64()
             << "S: 235 Authenticated" << FakeServer::bye();
    QTest::newRow("Login not supported") << scenario << "Login" << 0;

    scenario.clear();
    scenario << FakeServer::greetingAndEhlo(false)
             // The login job won't even try to send AUTH, because it does not
             // have any mechanisms to use
             //<< "C: AUTH PLAIN AGxvZ2luAHBhc3N3b3Jk" // [\0 + "login" + \0 + "password"].toBase64()
             //<< "S: 235 Authenticated"
             << FakeServer::bye();
    QTest::newRow("Auth not supported") << scenario << "Login" << 100;

    scenario.clear();
    scenario << FakeServer::greetingAndEhlo() << "S: 250 AUTH PLAIN"
             << "C: AUTH PLAIN AGxvZ2luAHBhc3N3b3Jk" // [\0 + "login" + \0 + "password"].toBase64()
             << "S: 535 Authorization failed" << FakeServer::bye();
    QTest::newRow("Wrong password") << scenario << "Plain" << 100;
}

void SmtpTest::testLoginJob()
{
    QFETCH(QList<QByteArray>, scenario);
    QFETCH(QString, authMode);
    QFETCH(int, errorCode);

    KSmtp::LoginJob::AuthMode mode = KSmtp::LoginJob::UnknownAuth;
    if (authMode == QLatin1String("Plain")) {
        mode = KSmtp::LoginJob::Plain;
    } else if (authMode == QLatin1String("Login")) {
        mode = KSmtp::LoginJob::Login;
    }

    FakeServer fakeServer;
    fakeServer.setScenario(scenario);
    fakeServer.startAndWait();
    KSmtp::Session session(QStringLiteral("127.0.0.1"), 5989);
    session.setCustomHostname(QStringLiteral("127.0.0.1"));
    session.openAndWait();

    auto login = new KSmtp::LoginJob(&session);
    login->setPreferedAuthMode(mode);
    login->setUserName(QStringLiteral("login"));
    login->setPassword(QStringLiteral("password"));
    login->exec();

    // Checking job error code:
    QVERIFY2(login->error() == errorCode, "Unexpected LoginJob error code");

    // Checking session state:
    QEXPECT_FAIL("Auth not supported", "Expected failure if not authentication method suported", Continue);
    QEXPECT_FAIL("Wrong password", "Expected failure if wrong password", Continue);
    QVERIFY2(session.state() == KSmtp::Session::Authenticated, "Authentication failed");

    session.quitAndWait();

    QVERIFY(fakeServer.isAllScenarioDone());

    fakeServer.quit();
}

void SmtpTest::testSendJob_data()
{
    QTest::addColumn<QList<QByteArray>>("scenario");
    QTest::addColumn<int>("errorCode");

    QList<QByteArray> scenario;
    scenario << FakeServer::greetingAndEhlo(false) << "C: MAIL FROM:<foo@bar.com>"
             << "S: 530 Not allowed" << FakeServer::bye();
    QTest::newRow("Send not allowed") << scenario << 100;

    scenario.clear();
    scenario << FakeServer::greetingAndEhlo(false) << "C: MAIL FROM:<foo@bar.com>"
             << "S: 250 ok"
             << "C: RCPT TO:<bar@foo.com>"
             << "S: 250 ok"
             << "C: DATA"
             << "S: 354 Ok go ahead"
             << "C: From: foo@bar.com"
             << "C: To: bar@foo.com"
             << "C: Hello world."
             << "C: .." // Single dot becomes two
             << "C: .." // Single dot becomes two
             << "C: ..." // Two dots become three
             << "C: ..Foo" // .Foo becomes ..Foo
             << "C: End"
             << "C: "
             << "C: ."
             << "S: 250 Ok transfer done" << FakeServer::bye();
    QTest::newRow("ok") << scenario << 0;

    scenario.clear();
}

void SmtpTest::testSendJob()
{
    QFETCH(QList<QByteArray>, scenario);
    QFETCH(int, errorCode);

    FakeServer fakeServer;
    fakeServer.setScenario(scenario);
    fakeServer.startAndWait();
    KSmtp::Session session(QStringLiteral("127.0.0.1"), 5989);
    session.setCustomHostname(QStringLiteral("127.0.0.1"));
    session.openAndWait();

    auto send = new KSmtp::SendJob(&session);
    send->setData("From: foo@bar.com\r\nTo: bar@foo.com\r\nHello world.\r\n.\r\n.\r\n..\r\n.Foo\r\nEnd");
    send->setFrom(QStringLiteral("foo@bar.com"));
    send->setTo({QStringLiteral("bar@foo.com")});
    send->exec();

    // Checking job error code:
    QVERIFY2(send->error() == errorCode, qPrintable(QStringLiteral("Unexpected LoginJob error: ") + send->errorString()));

    session.quitAndWait();

    QVERIFY(fakeServer.isAllScenarioDone());
    fakeServer.quit();
}

SmtpTest::SmtpTest()
{
}

void SmtpTest::initTestCase()
{
}

void SmtpTest::cleanupTestCase()
{
}

QTEST_MAIN(SmtpTest)
