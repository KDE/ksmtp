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
