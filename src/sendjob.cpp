/*
  SPDX-FileCopyrightText: 2010 BetterInbox <contact@betterinbox.com>
  SPDX-FileContributor: Christophe Laveault <christophe@betterinbox.com>
  SPDX-FileContributor: Gregory Schlomoff <gregory.schlomoff@gmail.com>

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "sendjob.h"
#include "job_p.h"
#include "ksmtp_debug.h"
#include "serverresponse_p.h"

#include <KLocalizedString>

namespace KSmtp
{
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
        : JobPrivate(session, name)
        , q(job)
    {
    }

    SendJob *const q;

    void sendNextRecipient();
    void addRecipients(const QStringList &rcpts);
    bool prepare();

    using MessagePart = struct {
        QString contentType;
        QString name;
        QByteArray content;
    };

    QString m_returnPath;
    QStringList m_recipients;
    QByteArray m_data;

    QStringList m_recipientsCopy;
    Status m_status = Idle;
    bool m_dsn = false;
};
}

using namespace KSmtp;

SendJob::SendJob(Session *session)
    : Job(*new SendJobPrivate(this, session, i18n("SendJob")))
{
}

void SendJob::setFrom(const QString &from)
{
    Q_D(SendJob);
    const auto start = from.indexOf(u'<');
    if (start > -1) {
        const auto end = qMax(start, from.indexOf(u'>', start));
        d->m_returnPath = u"<%1>"_s.arg(from.mid(start + 1, end - start - 1));
    } else {
        d->m_returnPath = u"<%1>"_s.arg(from);
    }
}

void SendJob::setTo(const QStringList &to)
{
    Q_D(SendJob);
    d->addRecipients(to);
}

void SendJob::setCc(const QStringList &cc)
{
    Q_D(SendJob);
    d->addRecipients(cc);
}

void SendJob::setBcc(const QStringList &bcc)
{
    Q_D(SendJob);
    d->addRecipients(bcc);
}

void SendJob::setData(const QByteArray &data)
{
    Q_D(SendJob);
    d->m_data = data;
    // A line with a single dot would make SMTP think "end of message", so use two dots in that case,
    // as per https://tools.ietf.org/html/rfc5321#section-4.5.2
    d->m_data.replace("\r\n.", "\r\n..");
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

    const int sizeLimit = session()->sizeLimit();
    if (sizeLimit > 0 && size() > sizeLimit) {
        setError(KJob::UserDefinedError);
        setErrorText(i18n("Could not send the message because it exceeds the maximum allowed size of %1 bytes. (Message size: %2 bytes.)", sizeLimit, size()));
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
    case SendJobPrivate::Idle:
        // TODO: anything to do here?
        break;

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
    const bool dsnSupport = m_session->allowsDsn() ? m_dsn : false;
    // qDebug() << " void SendJobPrivate::sendNextRecipient()" << m_session->allowsDsn() << " dsnSupport " << dsnSupport;
    q->sendCommand("RCPT TO:<" + m_recipientsCopy.takeFirst().toUtf8() + '>' + (dsnSupport ? " NOTIFY=success,failure" : ""));
}

void SendJobPrivate::addRecipients(const QStringList &rcpts)
{
    for (const auto &rcpt : rcpts) {
        if (rcpt.isEmpty()) {
            continue;
        }

        const int start = rcpt.indexOf(u'<');
        if (start > -1) {
            const int end = qMax(start, rcpt.indexOf(u'>', start));
            m_recipients.push_back(rcpt.mid(start + 1, end - start - 1));
        } else {
            m_recipients.push_back(rcpt);
        }
    }
}

bool SendJobPrivate::prepare()
{
    if (m_data.isEmpty()) {
        qCWarning(KSMTP_LOG) << "A message has to be set before starting a SendJob";
        return false;
    }

    m_recipientsCopy = m_recipients;

    if (m_recipients.isEmpty()) {
        qCWarning(KSMTP_LOG) << "Message has no recipients";
        return false;
    }

    return true;
}

int SendJob::size() const
{
    Q_D(const SendJob);

    return d->m_data.size();
}

void SendJob::setDeliveryStatusNotification(bool enabled)
{
    Q_D(SendJob);
    d->m_dsn = enabled;
}

#include "moc_sendjob.cpp"
