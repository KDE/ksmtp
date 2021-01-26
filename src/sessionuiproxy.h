/*
    SPDX-FileCopyrightText: 2009 Andras Mantia <amantia@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSMTP_SESSIONUIPROXY_H
#define KSMTP_SESSIONUIPROXY_H

#include "ksmtp_export.h"

#include "job.h"

#include <QSharedPointer>

class KSslErrorUiData;

namespace KSmtp
{
/** @short Interface to display communication errors and wait for user feedback. */
class KSMTP_EXPORT SessionUiProxy
{
public:
    using Ptr = QSharedPointer<SessionUiProxy>;

    virtual ~SessionUiProxy();
    /**
     * Show an SSL error and ask the user whether it should be ignored or not.
     * The recommended KDE UI is the following:
     * @code
     * #include <kio/ksslui.h>
     * class UiProxy: public SessionUiProxy {
     *   public:
     *     bool ignoreSslError(const KSslErrorUiData& errorData) {
     *       if (KIO::SslUi::askIgnoreSslErrors(errorData)) {
     *         return true;
     *       } else {
     *        return false;
     *       }
     *     }
     * };
     * [...]
     * Session session(server, port);
     * UiProxy *proxy = new UiProxy();
     * session.setUiProxy(proxy);
     * @endcode
     * @param errorData contains details about the error.
     * @return true if the error can be ignored
     */
    virtual bool ignoreSslError(const KSslErrorUiData &errorData) = 0;
};
}

#endif
