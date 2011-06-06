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

#include "sendjob.h"

#include <KDE/KDebug>
#include <KDE/KLocale>
#include "kmime/kmime_util.h"

#include "job_p.h"
#include "serverresponse_p.h"

namespace KSmtp {

class SendJobPrivate : public JobPrivate 
{  
public:
  enum Status {
    Idle,
    SendingReturnPath,
    SendingRecipients,
    SendingData
  };
  
  SendJobPrivate(SendJob *job, Session *session, const QString &name) 
    : JobPrivate(session, name), 
      q(job),
      m_status(Idle)
  {
  }
  
  SendJob* q;

  void sendNextRecipient();
  bool prepare();
  
  typedef struct {
    QString contentType;
    QString name;
    QByteArray content;
  } MessagePart;
  
  QString m_returnPath;
  QStringList m_recipients;
  QByteArray m_data;
  
  KMime::Message::Ptr m_message;
  
  QStringList m_recipientsCopy;
  Status m_status;
};
}

using namespace KSmtp;

SendJob::SendJob(Session *session)
  : Job(*new SendJobPrivate(this, session, i18n("SendJob")))
{
}

void SendJob::doStart()
{
  Q_D(SendJob);
  
  if (!d->prepare()) {
    setError(KJob::UserDefinedError);
    setErrorText(i18n("Could not send the message because either the sender or recipient field is missing or invalid"));
    emitResult();
    return;
  }

  int sizeLimit = session()->sizeLimit();
  if (sizeLimit > 0 && size() > sizeLimit) {
    setError(KJob::UserDefinedError);
    setErrorText(QString(i18n("Could not send the message because it exceeds the maximum allowed size of %1 bytes. (Message size: %2 bytes.)")).arg(sizeLimit, size()));
    emitResult();
    return;
  }

  d->m_status = SendJobPrivate::SendingReturnPath;
  sendCommand("MAIL FROM:" + d->m_returnPath.toUtf8());
}

void SendJob::handleResponse(const ServerResponse &r)
{
  Q_D(SendJob);
  
  // Handle server errors
  handleErrors(r);

  switch (d->m_status) {

  case SendJobPrivate::SendingReturnPath:

    // Expected response: server agreement
    if (r.isCode(25)) {
      d->m_status = SendJobPrivate::SendingRecipients;
      d->sendNextRecipient();
    }
    break;

case SendJobPrivate::SendingRecipients:

    // Expected response: server agreement
    if (r.isCode(25)) {
      if (d->m_recipientsCopy.isEmpty()) {
        sendCommand("DATA");
        d->m_status = SendJobPrivate::SendingData;
      } else {
        d->sendNextRecipient();
      }
    }
    break;

case SendJobPrivate::SendingData:

    // Expected responses:
    // 354: Go ahead sending data
    if (r.isCode(354)) {
      sendCommand(d->m_data);
      sendCommand("\r\n.");
    }

    // 25x: Data received correctly
    if (r.isCode(25)) {
      emitResult();
    }
    break;
  }
}

void SendJobPrivate::sendNextRecipient()
{
  q->sendCommand("RCPT TO:<" + m_recipientsCopy.takeFirst().toUtf8() + ">");
}


bool SendJobPrivate::prepare()
{
  if (!m_message) {
    kWarning() << "A message has to be set before starting a SendJob";
    return false;
  }

  if (m_message->from()->isEmpty()) {
    kWarning() << "Message has no sender";
    return false;
  }

  if (m_message->headerByType("Return-Path")) {
    m_returnPath = m_message->headerByType("Return-Path")->asUnicodeString();
  } else {
    m_returnPath = m_message->from()->asUnicodeString();
    if (m_returnPath.contains('<')) {
      m_returnPath = m_returnPath.split('<')[1].split('>')[0];
    }
    m_returnPath = "<" + m_returnPath + ">";
  }

  QStringList rec = m_message->to()->asUnicodeString().split(", ") + m_message->cc()->asUnicodeString().split(", ") + m_message->bcc()->asUnicodeString().split(", ");
  rec.removeAll("");
  foreach (QString r, rec) {
    if (r.contains('<')) {
      m_recipients.append(r.split('<')[1].split('>')[0]); //TODO: (CL) Replace by a regex
    } else {
      m_recipients.append(r);
    }
  }
  m_recipientsCopy = m_recipients;

  if (m_recipients.isEmpty()) {
    kWarning() << "Message has no recipients";
    return false;
  }

  m_data = m_message->encodedContent();
  return true;
}


void SendJob::setMessage(KMime::Message::Ptr message)
{
  Q_D(SendJob);
  d->m_message = message;
}

int SendJob::size() const
{
  Q_D(const SendJob);
  
  if (d->m_message) {
    return d->m_message->encodedContent().size();
  }
  return 0;
}
