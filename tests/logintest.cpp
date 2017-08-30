/*
 * Copyright (C) 2017  Daniel Vrátil <dvratil@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "loginjob.h"
#include "session.h"
#include "sessionuiproxy.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include <KIO/SslUi>

#include <iostream>

class SessionUiProxy : public KSmtp::SessionUiProxy
{
public:
    bool ignoreSslError(const KSslErrorUiData &errorData) override
    {
        return KIO::SslUi::askIgnoreSslErrors(errorData);
    }
};

void login(KSmtp::Session *session, const QString &user, const QString &pass, bool tls, bool ssl)
{
    auto login = new KSmtp::LoginJob(session);
    login->setUserName(user);
    login->setPassword(pass);
    if (tls) {
        login->setEncryptionMode(KSmtp::LoginJob::TlsV1);
    } else if (ssl) {
        login->setEncryptionMode(KSmtp::LoginJob::SslV3);
    }
    QObject::connect(
        login, &KJob::result,
        [](KJob *job) {
            if (job->error()) {
                std::cout << "Login error: " << job->errorString().toStdString() << std::endl;
                qApp->quit();
            } else {
                std::cout << "Login job finished" << std::endl;
            }
        });
    login->start();
    std::cout << "Login job started" << std::endl;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    
    QCommandLineParser parser;
    QCommandLineOption hostOption(QStringLiteral("host"), QString(), QStringLiteral("hostname"));
    QCommandLineOption portOption(QStringLiteral("port"), QString(), QStringLiteral("port"));
    QCommandLineOption userOption(QStringLiteral("user"), QString(), QStringLiteral("username"));
    QCommandLineOption passOption(QStringLiteral("pass"), QString(), QStringLiteral("password"));
    QCommandLineOption tlsOption(QStringLiteral("tls"));
    QCommandLineOption sslOption(QStringLiteral("ssl"));
    parser.addOption(hostOption);
    parser.addOption(portOption);
    parser.addOption(userOption);
    parser.addOption(passOption);
    parser.addOption(tlsOption);
    parser.addOption(sslOption);
    parser.addHelpOption();

    parser.process(app);

    if (!parser.isSet(hostOption) || !parser.isSet(portOption)
        || !parser.isSet(userOption) || !parser.isSet(passOption)) {
        parser.showHelp(1);
        return 1;
    }

    KSmtp::Session session(parser.value(hostOption),
                           parser.value(portOption).toUInt());
    QObject::connect(
        &session, &KSmtp::Session::stateChanged,
        [&](KSmtp::Session::State state) {
            switch (state) {
            case KSmtp::Session::Disconnected:
                std::cout << "Session in Disconnected state" << std::endl;
                break;
            case KSmtp::Session::Handshake:
                std::cout << "Session in Handshake state" << std::endl;
                break;
            case KSmtp::Session::Ready:
                std::cout << "Session in Ready state" << std::endl;
                break;
            case KSmtp::Session::Quitting:
                //Internal (avoid compile warning)
                break;
            case KSmtp::Session::NotAuthenticated: {
                std::cout << "Session in NotAuthenticated state" << std::endl;
                std::cout << "Available auth modes: ";
                const auto modes = session.availableAuthModes();
                for (const QString &mode : modes) {
                    std::cout << mode.toStdString() << " ";
                }
                std::cout << std::endl;
                login(&session, parser.value(userOption), parser.value(passOption),
                      parser.isSet(tlsOption), parser.isSet(sslOption));
            } break;
            case KSmtp::Session::Authenticated:
                std::cout << "Session entered Authenticated state, we are done" << std::endl;
                app.quit();
            }
        });
    std::cout << "Opening session ..." << std::endl;
    session.open();

    return app.exec();
}
