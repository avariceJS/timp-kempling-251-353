#include <QCoreApplication>
#include <cstdio>

#include "mytcpserver.h"

static void consoleHandler(QtMsgType, const QMessageLogContext &, const QString &msg)
{
    const QByteArray line = msg.toUtf8();
    fprintf(stderr, "%s\n", line.constData());
    fflush(stderr);
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(consoleHandler);

    QCoreApplication app(argc, argv);

    quint16 port = 5555;
    if (argc >= 2) {
        bool ok = false;
        const quint16 parsed = QString::fromLocal8Bit(argv[1]).toUShort(&ok);
        if (ok && parsed > 0)
            port = parsed;
    }

    fprintf(stderr, "============================================\n");
    fprintf(stderr, "  qt_tcp_server — совместный список\n");
    fprintf(stderr, "  Порт: %u | Максимум клиентов: %d\n", port, MyTcpServer::MAX_CLIENTS);
    fprintf(stderr, "============================================\n");
    fflush(stderr);

    MyTcpServer server(port);
    return app.exec();
}
