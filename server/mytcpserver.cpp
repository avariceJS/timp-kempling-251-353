#include "mytcpserver.h"

#include <QDebug>
#include <QHostAddress>

MyTcpServer::MyTcpServer(quint16 port, QObject *parent)
    : QObject(parent)
    , mTcpServer(new QTcpServer(this))
{
    connect(mTcpServer, &QTcpServer::newConnection, this, &MyTcpServer::slotNewConnection);

    if (!mTcpServer->listen(QHostAddress::Any, port)) {
        qCritical().noquote() << QStringLiteral("Не удалось запустить сервер:") << mTcpServer->errorString();
        return;
    }

    qInfo().noquote() << QStringLiteral("QT TCP Server запущен на порту %1. Лимит клиентов: %2.")
                             .arg(port)
                             .arg(MAX_CLIENTS);
    qInfo().noquote() << QStringLiteral("Ожидание подключений...");
}

void MyTcpServer::slotNewConnection()
{
    while (mTcpServer->hasPendingConnections()) {
        QTcpSocket *socket = mTcpServer->nextPendingConnection();
        const QString peer = socket->peerAddress().toString() + QLatin1Char(':')
                             + QString::number(socket->peerPort());

        if (connectedCount() >= MAX_CLIENTS) {
            sendLine(socket, QStringLiteral("Сервер занят. Подключитесь позже."));
            socket->flush();
            socket->disconnectFromHost();
            connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
            qInfo().noquote() << QStringLiteral("Отклонено подключение %1: сервер занят (%2/%2).")
                                     .arg(peer)
                                     .arg(MAX_CLIENTS);
            continue;
        }

        mClients.insert(socket, ClientState());
        connect(socket, &QTcpSocket::readyRead, this, &MyTcpServer::slotServerRead);
        connect(socket, &QTcpSocket::disconnected, this, &MyTcpServer::slotClientDisconnected);

        sendLine(socket, QStringLiteral("Добро пожаловать на сервер совместного редактируемого списка!"));
        sendLine(socket, QStringLiteral("Команды: /add <элемент>  |  /remove <элемент>  |  /finish"));
        sendLine(socket, formatList(false));

        qInfo().noquote() << QStringLiteral("Клиент подключён: %1. Сейчас клиентов: %2")
                                 .arg(peer)
                                 .arg(connectedCount());

        broadcastCount();
    }
}

void MyTcpServer::slotClientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket)
        return;

    const QString peer = socket->peerAddress().toString() + QLatin1Char(':')
                         + QString::number(socket->peerPort());
    mClients.remove(socket);
    socket->deleteLater();

    qInfo().noquote() << QStringLiteral("Клиент отключился: %1. Сейчас клиентов: %2")
                             .arg(peer)
                             .arg(connectedCount());

    if (connectedCount() > 0)
        broadcastFinalListIfReady();
}

void MyTcpServer::slotServerRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket || !mClients.contains(socket))
        return;

    ClientState &state = mClients[socket];
    state.buffer += socket->readAll();

    int idx = 0;
    while ((idx = state.buffer.indexOf('\n')) != -1) {
        QByteArray raw = state.buffer.left(idx);
        state.buffer.remove(0, idx + 1);
        if (raw.endsWith('\r'))
            raw.chop(1);

        const QString line = QString::fromUtf8(raw).trimmed();
        if (line.isEmpty())
            continue;

        qInfo().noquote() << QStringLiteral("От %1:%2: %3")
                                 .arg(socket->peerAddress().toString())
                                 .arg(socket->peerPort())
                                 .arg(line);
        handleCommand(socket, line);
    }
}

void MyTcpServer::handleCommand(QTcpSocket *socket, const QString &line)
{
    if (line.startsWith(QLatin1String("/add"), Qt::CaseInsensitive)) {
        const QString item = line.mid(4).trimmed();
        if (item.isEmpty()) {
            sendLine(socket, QStringLiteral("Использование: /add <элемент>"));
            return;
        }
        cmdAdd(socket, item);
        return;
    }

    if (line.startsWith(QLatin1String("/remove"), Qt::CaseInsensitive)) {
        const QString item = line.mid(7).trimmed();
        if (item.isEmpty()) {
            sendLine(socket, QStringLiteral("Использование: /remove <элемент>"));
            return;
        }
        cmdRemove(socket, item);
        return;
    }

    if (line.compare(QLatin1String("/finish"), Qt::CaseInsensitive) == 0) {
        cmdFinish(socket);
        return;
    }

    sendLine(socket, QStringLiteral("Неизвестная команда. Доступны: /add, /remove, /finish"));
}

void MyTcpServer::cmdAdd(QTcpSocket *socket, const QString &item)
{
    if (mClients[socket].finished) {
        sendLine(socket, QStringLiteral("Вы уже отправили /finish. Дождитесь остальных клиентов."));
        return;
    }

    mSharedList.append(item);
    const QString msg = QStringLiteral("Добавлено: «%1»").arg(item);
    qInfo().noquote() << msg;
    broadcast(msg);
    broadcast(formatList(false));
}

void MyTcpServer::cmdRemove(QTcpSocket *socket, const QString &item)
{
    if (mClients[socket].finished) {
        sendLine(socket, QStringLiteral("Вы уже отправили /finish. Дождитесь остальных клиентов."));
        return;
    }

    const int idx = mSharedList.indexOf(item);
    if (idx < 0) {
        sendLine(socket, QStringLiteral("Элемент «%1» не найден в списке.").arg(item));
        return;
    }

    mSharedList.removeAt(idx);
    const QString msg = QStringLiteral("Удалено: «%1»").arg(item);
    qInfo().noquote() << msg;
    broadcast(msg);
    broadcast(formatList(false));
}

void MyTcpServer::cmdFinish(QTcpSocket *socket)
{
    if (mClients[socket].finished) {
        sendLine(socket, QStringLiteral("Вы уже завершили редактирование. Ожидайте остальных."));
        return;
    }

    mClients[socket].finished = true;

    int finishedCount = 0;
    for (auto it = mClients.constBegin(); it != mClients.constEnd(); ++it) {
        if (it.value().finished)
            ++finishedCount;
    }

    sendLine(socket, QStringLiteral("Вы завершили редактирование. Ожидание остальных клиентов..."));
    qInfo().noquote() << QStringLiteral("Клиент завершил редактирование (%1/%2).")
                             .arg(finishedCount)
                             .arg(connectedCount());

    broadcastFinalListIfReady();
}

void MyTcpServer::broadcastFinalListIfReady()
{
    if (mClients.isEmpty())
        return;

    for (auto it = mClients.constBegin(); it != mClients.constEnd(); ++it) {
        if (!it.value().finished)
            return;
    }

    qInfo().noquote() << QStringLiteral("Все клиенты отправили /finish. Рассылка итогового списка.");
    broadcast(QStringLiteral("=== Редактирование завершено ==="));
    broadcast(formatList(true));

    for (auto it = mClients.begin(); it != mClients.end(); ++it)
        it.value().finished = false;
}

QString MyTcpServer::formatList(bool finalVersion) const
{
    const QString title = finalVersion ? QStringLiteral("Итоговый список:")
                                       : QStringLiteral("Текущий список:");
    if (mSharedList.isEmpty())
        return title + QStringLiteral(" (пусто)");

    QString result = title;
    for (int i = 0; i < mSharedList.size(); ++i)
        result += QStringLiteral("\n  %1. %2").arg(i + 1).arg(mSharedList.at(i));
    return result;
}

void MyTcpServer::sendLine(QTcpSocket *socket, const QString &text)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState)
        return;
    socket->write((text + QLatin1Char('\n')).toUtf8());
    socket->flush();
}

void MyTcpServer::broadcast(const QString &text)
{
    for (auto it = mClients.constBegin(); it != mClients.constEnd(); ++it)
        sendLine(it.key(), text);
}

void MyTcpServer::broadcastCount()
{
    broadcast(QStringLiteral("Подключено клиентов: %1").arg(connectedCount()));
}

int MyTcpServer::connectedCount() const
{
    return mClients.size();
}
