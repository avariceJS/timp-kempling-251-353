#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>

class MyTcpServer : public QObject
{
    Q_OBJECT
public:
    static const int MAX_CLIENTS = 4;

    explicit MyTcpServer(quint16 port, QObject *parent = nullptr);

private slots:
    void slotNewConnection();
    void slotClientDisconnected();
    void slotServerRead();

private:
    struct ClientState {
        QByteArray buffer;
        bool finished = false;
    };

    void sendLine(QTcpSocket *socket, const QString &text);
    void broadcast(const QString &text);
    void broadcastCount();
    void handleCommand(QTcpSocket *socket, const QString &line);
    void cmdAdd(QTcpSocket *socket, const QString &item);
    void cmdRemove(QTcpSocket *socket, const QString &item);
    void cmdFinish(QTcpSocket *socket);
    void broadcastFinalListIfReady();
    QString formatList(bool finalVersion) const;
    int connectedCount() const;

    QTcpServer *mTcpServer;
    QMap<QTcpSocket *, ClientState> mClients;
    QStringList mSharedList;
};

#endif
