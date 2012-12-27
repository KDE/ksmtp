#include "smtptest.h"

#include <QTest>
#include "fakeserver.h"
#include "../session.h"
#include "../loginjob.h"
#include "../sendjob.h"


void SmtpTest::testHello_data()
{
  QTest::addColumn<QList<QByteArray> >("scenario");

  QList<QByteArray> scenario;
  scenario << FakeServer::greeting()
      << "C: EHLO 127.0.0.1"
      << "S: 250 Localhost ready to roll";
  QTest::newRow("EHLO OK") << scenario;

  scenario.clear();
  scenario << FakeServer::greeting()
      << "C: EHLO 127.0.0.1"
      << "S: 500 Command was not recognized"
      << "C: HELO 127.0.0.1"
      << "S: 250 Localhost ready to roll";
  QTest::newRow("EHLO unknown") << scenario;

  scenario.clear();
  scenario << FakeServer::greeting()
      << "C: EHLO 127.0.0.1"
      << "S: 502 Command not implemented"
      << "C: HELO 127.0.0.1"
      << "S: 250 Localhost ready to roll";
  QTest::newRow("EHLO not implemented") << scenario;

  scenario.clear();
  scenario << FakeServer::greeting()
      << "C: EHLO 127.0.0.1"
      << "S: 502 Command not implemented"
      << "C: HELO 127.0.0.1"
      << "S: 500 Command was not recognized";
  QTest::newRow("ERROR") << scenario;

}

void SmtpTest::testHello()
{
  QFETCH( QList<QByteArray>, scenario );

  FakeServer fakeServer;
  fakeServer.setScenario(scenario);
  fakeServer.startAndWait();
  KSmtp::Session session("127.0.0.1", 5989);
  session.openAndWait();
  
  qDebug() << "### Session state is:" << session.state();
  
  QEXPECT_FAIL("ERROR" , "Expected failure if HELO command not recognized", Continue);
  QVERIFY2(session.state() == KSmtp::Session::NotAuthenticated, "Handshake failed");

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
      << "S: 235 Authenticated";
  QTest::newRow("Plain auth ok") << scenario << "Plain" << 0;

  scenario.clear();
  scenario << FakeServer::greetingAndEhlo()
      << "S: 250 AUTH PLAIN LOGIN"
      << "C: AUTH LOGIN"
      << "S: 334 VXNlcm5hbWU6"   // "Username:".toBase64()
      << "C: bG9naW4="           // "login".toBase64()
      << "S: 334 UGFzc3dvcmQ6"   // "Password:".toBase64()
      << "C: cGFzc3dvcmQ="       // "password".toBase64()
      << "S: 235 Authenticated";
  QTest::newRow("Login auth ok") << scenario << "Login" << 0;

  scenario.clear();
  scenario << FakeServer::greetingAndEhlo()
      << "S: 250 AUTH PLAIN"
      << "C: AUTH PLAIN AGxvZ2luAHBhc3N3b3Jk" // [\0 + "login" + \0 + "password"].toBase64()
      << "S: 235 Authenticated";
  QTest::newRow("Login not supported") << scenario << "Login" << 0;

  scenario.clear();
  scenario << FakeServer::greetingAndEhlo()
      << "C: AUTH PLAIN AGxvZ2luAHBhc3N3b3Jk" // [\0 + "login" + \0 + "password"].toBase64()
      << "S: 235 Authenticated";
  QTest::newRow("Auth not supported") << scenario << "Login" << 100;

  scenario.clear();
  scenario << FakeServer::greetingAndEhlo()
      << "S: 250 AUTH PLAIN"
      << "C: AUTH PLAIN AGxvZ2luAHBhc3N3b3Jk" // [\0 + "login" + \0 + "password"].toBase64()
      << "S: 535 Authorization failed";
  QTest::newRow("Wrong password") << scenario << "Plain" << 100;
}

void SmtpTest::testLoginJob()
{
  QFETCH( QList<QByteArray>, scenario );
  QFETCH( QString, authMode );
  QFETCH( int, errorCode );

  KSmtp::LoginJob::AuthMode mode = KSmtp::LoginJob::UnknownAuth;
  if (authMode == "Plain") {
    mode = KSmtp::LoginJob::Plain;
  } else if (authMode == "Login") {
    mode = KSmtp::LoginJob::Login;
  }

  FakeServer fakeServer;
  fakeServer.setScenario(scenario);
  fakeServer.startAndWait();
  KSmtp::Session session("127.0.0.1", 5989);
  session.openAndWait();
  
  KSmtp::LoginJob* login = new KSmtp::LoginJob(&session);
  login->setPreferedAuthMode(mode);
  login->setUserName("login");
  login->setPassword("password");
  login->exec();

  // Checking job error code:
  QVERIFY2(login->error() == errorCode, "Unexpected LoginJob error code");

  // Checking session state:
  QEXPECT_FAIL("Auth not supported" , "Expected failure if not authentication method suported", Continue);
  QEXPECT_FAIL("Wrong password" , "Expected failure if wrong password", Continue);
  QVERIFY2(session.state() == KSmtp::Session::Authenticated, "Authentication failed");

  fakeServer.quit();
}


void SmtpTest::testSendJob_data()
{
  QTest::addColumn<QList<QByteArray> >("scenario");
  QTest::addColumn<int>("errorCode");

  QList<QByteArray> scenario;
  scenario << FakeServer::greetingAndEhlo()
      << "C: MAIL FROM:<foo@bar.com>"
      << "S: 530 Not allowed";
  QTest::newRow("Send not allowed") << scenario << 100;

  scenario.clear();
  scenario << FakeServer::greetingAndEhlo()
      << "C: MAIL FROM:<foo@bar.com>"
      << "S: 250 ok"
      << "C: RCPT TO:<bar@foo.com>"
      << "S: 250 ok"
      << "C: DATA"
      << "S: 354 Ok go ahead"
      << "C: SKIP"
      << "C: ."
      << "S: 250 Ok transfer done";
  QTest::newRow("ok") << scenario << 0;

  scenario.clear();

}

void SmtpTest::testSendJob()
{
  QFETCH( QList<QByteArray>, scenario );
  QFETCH( int, errorCode );

  FakeServer fakeServer;
  fakeServer.setScenario(scenario);
  fakeServer.startAndWait();
  KSmtp::Session session("127.0.0.1", 5989);
  session.openAndWait();
  
  KSmtp::SendJob* send = new KSmtp::SendJob(&session);

  KMime::Message::Ptr m (new KMime::Message());
  m->from()->fromUnicodeString( "Foo Bar <foo@bar.com>", "utf-8" );
  m->to()->fromUnicodeString( "Bar Foo <bar@foo.com>", "utf-8" );
  m->subject()->fromUnicodeString( "Subject", "utf-8" );
  send->setMessage(m);
  send->exec();

  // Checking job error code:
  QVERIFY2(send->error() == errorCode, "Unexpected LoginJob error code");

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
