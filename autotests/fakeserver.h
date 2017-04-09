#ifndef KSMTP_FAKESERVER_H
#define KSMTP_FAKESERVER_H

#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMutex>
#include <QStringList>

Q_DECLARE_METATYPE(QList<QByteArray>)

class FakeServer : public QThread
{
    Q_OBJECT

public:
    FakeServer(QObject *parent = nullptr);
    ~FakeServer() Q_DECL_OVERRIDE;

    void startAndWait();
    void run() Q_DECL_OVERRIDE;

    static QByteArray greeting();
    static QList<QByteArray> greetingAndEhlo();

    void setScenario(const QList<QByteArray> &scenario);
    void addScenario(const QList<QByteArray> &scenario);
    void addScenarioFromFile(const QString &fileName);
    bool isScenarioDone(int scenarioNumber) const;
    bool isAllScenarioDone() const;

private Q_SLOTS:
    void newConnection();
    void dataAvailable();
    void started();

private:
    void writeServerPart(int scenarioNumber);
    void readClientPart(int scenarioNumber);

    QList< QList<QByteArray> > m_scenarios;
    QTcpServer *m_tcpServer;
    mutable QMutex m_mutex;
    QList<QTcpSocket *> m_clientSockets;
};

#endif // KSMTP_FAKESERVER_H
