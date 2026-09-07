#include "widget.h"

#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , mSocket(new QTcpSocket(this))
{
    setWindowTitle(QStringLiteral("Клиент совместного списка"));
    resize(560, 480);

    mHostEdit = new QLineEdit(QStringLiteral("127.0.0.1"), this);
    mPortEdit = new QLineEdit(QStringLiteral("5555"), this);
    mPortEdit->setValidator(new QIntValidator(1, 65535, this));
    mConnectBtn = new QPushButton(QStringLiteral("Подключиться"), this);

    auto *top = new QHBoxLayout();
    top->addWidget(new QLabel(QStringLiteral("Хост:"), this));
    top->addWidget(mHostEdit);
    top->addWidget(new QLabel(QStringLiteral("Порт:"), this));
    top->addWidget(mPortEdit);
    top->addWidget(mConnectBtn);

    mLog = new QTextEdit(this);
    mLog->setReadOnly(true);

    mItemEdit = new QLineEdit(this);
    mItemEdit->setPlaceholderText(QStringLiteral("Элемент списка"));
    mAddBtn = new QPushButton(QStringLiteral("/add"), this);
    mRemoveBtn = new QPushButton(QStringLiteral("/remove"), this);
    mFinishBtn = new QPushButton(QStringLiteral("/finish"), this);

    auto *listRow = new QHBoxLayout();
    listRow->addWidget(mItemEdit);
    listRow->addWidget(mAddBtn);
    listRow->addWidget(mRemoveBtn);
    listRow->addWidget(mFinishBtn);

    mMessageEdit = new QLineEdit(this);
    mMessageEdit->setPlaceholderText(QStringLiteral("Команда вручную, например: /add молоко"));
    mSendBtn = new QPushButton(QStringLiteral("Отправить"), this);

    auto *cmdRow = new QHBoxLayout();
    cmdRow->addWidget(mMessageEdit);
    cmdRow->addWidget(mSendBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addWidget(mLog);
    layout->addLayout(listRow);
    layout->addLayout(cmdRow);

    connect(mConnectBtn, &QPushButton::clicked, this, &Widget::onConnectClicked);
    connect(mSendBtn, &QPushButton::clicked, this, &Widget::onSendClicked);
    connect(mAddBtn, &QPushButton::clicked, this, &Widget::onAddClicked);
    connect(mRemoveBtn, &QPushButton::clicked, this, &Widget::onRemoveClicked);
    connect(mFinishBtn, &QPushButton::clicked, this, &Widget::onFinishClicked);
    connect(mMessageEdit, &QLineEdit::returnPressed, this, &Widget::onSendClicked);
    connect(mItemEdit, &QLineEdit::returnPressed, this, &Widget::onAddClicked);

    connect(mSocket, &QTcpSocket::connected, this, &Widget::onConnected);
    connect(mSocket, &QTcpSocket::disconnected, this, &Widget::onDisconnected);
    connect(mSocket, &QTcpSocket::readyRead, this, &Widget::onReadyRead);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(mSocket, &QAbstractSocket::errorOccurred, this, &Widget::onError);
#else
    connect(mSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError()));
#endif

    setConnectedUi(false);
}

void Widget::onConnectClicked()
{
    if (mSocket->state() == QAbstractSocket::ConnectedState) {
        mSocket->disconnectFromHost();
        return;
    }

    const QString host = mHostEdit->text().trimmed();
    const quint16 port = static_cast<quint16>(mPortEdit->text().toUShort());
    appendLog(QStringLiteral("Подключение к %1:%2 ...").arg(host).arg(port));
    mSocket->connectToHost(host, port);
}

void Widget::onSendClicked()
{
    const QString cmd = mMessageEdit->text().trimmed();
    if (cmd.isEmpty())
        return;
    sendCommand(cmd);
    mMessageEdit->clear();
}

void Widget::onAddClicked()
{
    const QString item = mItemEdit->text().trimmed();
    if (item.isEmpty()) {
        appendLog(QStringLiteral("Введите элемент для /add"));
        return;
    }
    sendCommand(QStringLiteral("/add %1").arg(item));
}

void Widget::onRemoveClicked()
{
    const QString item = mItemEdit->text().trimmed();
    if (item.isEmpty()) {
        appendLog(QStringLiteral("Введите элемент для /remove"));
        return;
    }
    sendCommand(QStringLiteral("/remove %1").arg(item));
}

void Widget::onFinishClicked()
{
    sendCommand(QStringLiteral("/finish"));
}

void Widget::onConnected()
{
    setConnectedUi(true);
    appendLog(QStringLiteral("Соединение установлено."));
}

void Widget::onDisconnected()
{
    setConnectedUi(false);
    mBuffer.clear();
    appendLog(QStringLiteral("Соединение закрыто."));
}

void Widget::onReadyRead()
{
    mBuffer += mSocket->readAll();
    int idx = 0;
    while ((idx = mBuffer.indexOf('\n')) != -1) {
        QByteArray raw = mBuffer.left(idx);
        mBuffer.remove(0, idx + 1);
        if (raw.endsWith('\r'))
            raw.chop(1);
        appendLog(QString::fromUtf8(raw));
    }
}

void Widget::onError()
{
    appendLog(QStringLiteral("Ошибка сокета: %1").arg(mSocket->errorString()));
}

void Widget::appendLog(const QString &text)
{
    mLog->append(text);
}

void Widget::sendCommand(const QString &cmd)
{
    if (mSocket->state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, QStringLiteral("Нет соединения"),
                             QStringLiteral("Сначала подключитесь к серверу."));
        return;
    }
    mSocket->write((cmd + QLatin1Char('\n')).toUtf8());
    appendLog(QStringLiteral(">> %1").arg(cmd));
}

void Widget::setConnectedUi(bool connected)
{
    mConnectBtn->setText(connected ? QStringLiteral("Отключиться") : QStringLiteral("Подключиться"));
    mHostEdit->setEnabled(!connected);
    mPortEdit->setEnabled(!connected);
    mSendBtn->setEnabled(connected);
    mAddBtn->setEnabled(connected);
    mRemoveBtn->setEnabled(connected);
    mFinishBtn->setEnabled(connected);
    mMessageEdit->setEnabled(connected);
    mItemEdit->setEnabled(connected);
}
