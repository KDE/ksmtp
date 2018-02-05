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

#include "smtptest.h"

#include <QTest>
#include "fakeserver.h"
#include "session.h"
#include "loginjob.h"
#include "sendjob.h"


void SmtpTest::testHello_data()
{
    QTest::addColumn<QList<QByteArray> >("scenario");
    QTest::addColumn<QString>("hostname");

    QList<QByteArray> scenario;
    scenario << FakeServer::greeting()
             << "C: EHLO 127.0.0.1"
             << "S: 250 Localhost ready to roll"
             << FakeServer::bye();
    QTest::newRow("EHLO OK") << scenario << QStringLiteral("127.0.0.1");

    scenario.clear();
    scenario << FakeServer::greeting()
             << "C: EHLO 127.0.0.1"
             << "S: 500 Command was not recognized"
             << "C: HELO 127.0.0.1"
             << "S: 250 Localhost ready to roll"
             << FakeServer::bye();
    QTest::newRow("EHLO unknown") << scenario << QStringLiteral("127.0.0.1");

    scenario.clear();
    scenario << FakeServer::greeting()
             << "C: EHLO 127.0.0.1"
             << "S: 502 Command not implemented"
             << "C: HELO 127.0.0.1"
             << "S: 250 Localhost ready to roll"
             << FakeServer::bye();
    QTest::newRow("EHLO not implemented") << scenario << QStringLiteral("127.0.0.1");

    scenario.clear();
    scenario << FakeServer::greeting()
             << "C: EHLO 127.0.0.1"
             << "S: 502 Command not implemented"
             << "C: HELO 127.0.0.1"
             << "S: 500 Command was not recognized"
             << FakeServer::bye();
    QTest::newRow("ERROR") << scenario << QStringLiteral("127.0.0.1");

    scenario.clear();
    scenario << FakeServer::greeting()
             << "C: EHLO random.stranger"
             << "S: 250 random.stranger ready to roll"
             << FakeServer::bye();
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

    QEXPECT_FAIL("ERROR" , "Expected failure if HELO command not recognized", Continue);
    QVERIFY2(session.state() == KSmtp::Session::NotAuthenticated, "Handshake failed");

    session.quitAndWait();

    QVERIFY(fakeServer.isAllScenarioDone());
    fakeServer.quit();
}


void SmtpTest::testLoginJob_data()
{
    QTest::addColumn<QList<QByteArray> >("scenario");
    QTest::addColumn<QString>("authMode");
    QTest::addColumn<int>("errorCode");


    QList<QByteArray> scenario;
    scenario << FakeServer::greetingAndEhlo()
             << "S: 250 AUTH PLAIN LOGIN"
             << "C: AUTH PLAIN AGxvZ2luAHBhc3N3b3Jk" // [\0 + "login" + \0 + "password"].toBase64()
             << "S: 235 Authenticated"
             << FakeServer::bye();
    QTest::newRow("Plain auth ok") << scenario << "Plain" << 0;

    scenario.clear();
    scenario << FakeServer::greetingAndEhlo()
             << "S: 250 AUTH PLAIN LOGIN"
             << "C: AUTH LOGIN"
             << "S: 334 VXNlcm5hbWU6"   // "Username:".toBase64()
             << "C: bG9naW4="           // "login".toBase64()
             << "S: 334 UGFzc3dvcmQ6"   // "Password:".toBase64()
             << "C: cGFzc3dvcmQ="       // "password".toBase64()
             << "S: 235 Authenticated"
             << FakeServer::bye();
    QTest::newRow("Login auth ok") << scenario << "Login" << 0;

    scenario.clear();
    scenario << FakeServer::greetingAndEhlo()
             << "S: 250 AUTH PLAIN"
             << "C: AUTH PLAIN AGxvZ2luAHBhc3N3b3Jk" // [\0 + "login" + \0 + "password"].toBase64()
             << "S: 235 Authenticated"
             << FakeServer::bye();
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
    scenario << FakeServer::greetingAndEhlo()
             << "S: 250 AUTH PLAIN"
             << "C: AUTH PLAIN AGxvZ2luAHBhc3N3b3Jk" // [\0 + "login" + \0 + "password"].toBase64()
             << "S: 535 Authorization failed"
             << FakeServer::bye();
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

    KSmtp::LoginJob *login = new KSmtp::LoginJob(&session);
    login->setPreferedAuthMode(mode);
    login->setUserName(QStringLiteral("login"));
    login->setPassword(QStringLiteral("password"));
    login->exec();

    // Checking job error code:
    QVERIFY2(login->error() == errorCode, "Unexpected LoginJob error code");

    // Checking session state:
    QEXPECT_FAIL("Auth not supported" , "Expected failure if not authentication method suported", Continue);
    QEXPECT_FAIL("Wrong password" , "Expected failure if wrong password", Continue);
    QVERIFY2(session.state() == KSmtp::Session::Authenticated, "Authentication failed");

    session.quitAndWait();

    QVERIFY(fakeServer.isAllScenarioDone());

    fakeServer.quit();
}


void SmtpTest::testSendJob_data()
{
    QTest::addColumn<QList<QByteArray> >("scenario");
    QTest::addColumn<int>("errorCode");

    QList<QByteArray> scenario;
    scenario << FakeServer::greetingAndEhlo(false)
             << "C: MAIL FROM:<foo@bar.com>"
             << "S: 530 Not allowed"
             << FakeServer::bye();
    QTest::newRow("Send not allowed") << scenario << 100;

    scenario.clear();
    scenario << FakeServer::greetingAndEhlo(false)
             << "C: MAIL FROM:<foo@bar.com>"
             << "S: 250 ok"
             << "C: RCPT TO:<bar@foo.com>"
             << "S: 250 ok"
             << "C: DATA"
             << "S: 354 Ok go ahead"
             << "C: SKIP"
             << "C: ."
             << "S: 250 Ok transfer done"
             << FakeServer::bye();
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

    KSmtp::SendJob *send = new KSmtp::SendJob(&session);

    KMime::Message::Ptr m(new KMime::Message());
    m->from()->fromUnicodeString(QStringLiteral("Foo Bar <foo@bar.com>"), "utf-8");
    m->to()->fromUnicodeString(QStringLiteral("Bar Foo <bar@foo.com>"), "utf-8");
    m->subject()->fromUnicodeString(QStringLiteral("Subject"), "utf-8");
    send->setMessage(m);
    send->exec();

    // Checking job error code:
    QVERIFY2(send->error() == errorCode, "Unexpected LoginJob error code");

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
