/*
 * SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "loginjob.h"
#include "session.h"
#include "sessionuiproxy.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

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

void login(KSmtp::Session *session, const QString &user, const QString &pass, bool ssltls, bool starttls)
{
    auto login = new KSmtp::LoginJob(session);
    login->setUserName(user);
    login->setPassword(pass);
    if (ssltls) {
        login->setEncryptionMode(KSmtp::LoginJob::SSLorTLS);
    } else if (starttls) {
        login->setEncryptionMode(KSmtp::LoginJob::STARTTLS);
    }
    QObject::connect(login, &KJob::result, [](KJob *job) {
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
    QApplication app(argc, argv);

    QCommandLineParser parser;
    QCommandLineOption hostOption(QStringLiteral("host"), QString(), QStringLiteral("hostname"));
    QCommandLineOption portOption(QStringLiteral("port"), QString(), QStringLiteral("port"));
    QCommandLineOption userOption(QStringLiteral("user"), QString(), QStringLiteral("username"));
    QCommandLineOption passOption(QStringLiteral("pass"), QString(), QStringLiteral("password"));
    QCommandLineOption sslTlsOption(QStringLiteral("sslTls"));
    QCommandLineOption startTlsOption(QStringLiteral("starttls"));
    parser.addOption(hostOption);
    parser.addOption(portOption);
    parser.addOption(userOption);
    parser.addOption(passOption);
    parser.addOption(sslTlsOption);
    parser.addOption(startTlsOption);
    parser.addHelpOption();

    parser.process(app);

    if (!parser.isSet(hostOption) || !parser.isSet(portOption) || !parser.isSet(userOption) || !parser.isSet(passOption)) {
        parser.showHelp(1);
        return 1;
    }

    KSmtp::Session session(parser.value(hostOption), parser.value(portOption).toUInt());
    session.setUiProxy(SessionUiProxy::Ptr(new SessionUiProxy));
    QObject::connect(&session, &KSmtp::Session::stateChanged, [&](KSmtp::Session::State state) {
        switch (state) {
        case KSmtp::Session::Disconnected:
            std::cout << "Session in Disconnected state" << std::endl;
            break;
        case KSmtp::Session::Handshake:
            std::cout << "Session in Handshake state" << std::endl;
            break;
        case KSmtp::Session::Ready:
            std::cout << "Session in Ready state" << std::endl;
            std::cout << std::endl;
            login(&session, parser.value(userOption), parser.value(passOption), parser.isSet(sslTlsOption), parser.isSet(startTlsOption));
            break;
        case KSmtp::Session::Quitting:
            // Internal (avoid compile warning)
            break;
        case KSmtp::Session::NotAuthenticated: {
            std::cout << "Session in NotAuthenticated state" << std::endl;
            std::cout << "Available auth modes: ";
            const auto modes = session.availableAuthModes();
            for (const QString &mode : modes) {
                std::cout << mode.toStdString() << " ";
            }
            std::cout << std::endl;
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
