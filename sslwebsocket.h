#ifndef SSLWEBSOCKET_H
#define SSLWEBSOCKET_H

#include <QObject>
#include <QWebSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSettings>
#include <QProtobufSerializer>
#include "message.qpb.h"

class SslWebsocket : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
public:
    SslWebsocket(const QString& host, quint16 port, QObject* parent = nullptr);
    Q_INVOKABLE void connectWidthToken();
    Q_INVOKABLE void disconnectFromServer();
    Q_INVOKABLE void sendMessage(const QString& message, const QString& recip_id);
    Q_INVOKABLE void sendImage(const QUrl& image, const QString& recip_id);
    Q_INVOKABLE void closeSocket();
    QString errorMessage() const { return m_errorMessage; }
    void generateKaypair();
    void handleServerPublickKey(const messenger::KeyExchange& pub_key);
    void handleChatMessage(const messenger::SecureEnvelope& msg);
    void handleChatImgMessage(const messenger::SecureImage& msg);
    void sendPublicKey();
private slots:
    void onConnected();
    void onDisconnected();
    void onBinaryMessageReceived(const QByteArray &data);
    void onError(QAbstractSocket::SocketError error);
    void onSslErrors(const QList<QSslError> &errors);
signals:
    void errorMessageChanged();
    void connectedChanged();
    void discconnectedChanged();
    void messageSaved(int chat_id);
private:
    QWebSocket* socket_;
    QString host_;
    quint16 port_;
    QString m_errorMessage;
    QSettings settings_;
    QProtobufSerializer serializer_;
    std::vector<unsigned char> secret_key_;
    std::vector<unsigned char> public_key_;
    std::vector<unsigned char> shared_key_;
};

#endif // SSLWEBSOCKET_H
