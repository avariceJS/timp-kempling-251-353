#ifndef WIDGET_H
#define WIDGET_H

#include <QLineEdit>
#include <QPushButton>
#include <QTcpSocket>
#include <QTextEdit>
#include <QWidget>

class Widget : public QWidget
{
    Q_OBJECT
public:
    explicit Widget(QWidget *parent = nullptr);

private slots:
    void onConnectClicked();
    void onSendClicked();
    void onAddClicked();
    void onRemoveClicked();
    void onFinishClicked();
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError();

private:
    void appendLog(const QString &text);
    void sendCommand(const QString &cmd);
    void setConnectedUi(bool connected);

    QTcpSocket *mSocket;
    QLineEdit *mHostEdit;
    QLineEdit *mPortEdit;
    QPushButton *mConnectBtn;
    QTextEdit *mLog;
    QLineEdit *mItemEdit;
    QLineEdit *mMessageEdit;
    QPushButton *mSendBtn;
    QPushButton *mAddBtn;
    QPushButton *mRemoveBtn;
    QPushButton *mFinishBtn;
    QByteArray mBuffer;
};

#endif
