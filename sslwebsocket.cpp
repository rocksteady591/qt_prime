#include "sslwebsocket.h"
#include <sodium.h>
#include <sodium/crypto_box.h>

SslWebsocket::SslWebsocket(const QString& host, quint16 port, QObject* parent)
    : QObject(parent), socket_(new QWebSocket()), host_(host), port_(port){
    connect(socket_, &QWebSocket::connected, this, &SslWebsocket::onConnected);
    connect(socket_, &QWebSocket::disconnected, this, &SslWebsocket::onDisconnected);
    connect(socket_, &QWebSocket::binaryMessageReceived, this, &SslWebsocket::onBinaryMessageReceived);
    connect(socket_, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred), this, &SslWebsocket::onError);
    connect(socket_, &QWebSocket::sslErrors, this, &SslWebsocket::onSslErrors);
    key_ = generateKaypair();
}

void SslWebsocket::connectWidthToken() {
    QUrl url(QString("wss://%1:%2/").arg(host_).arg(port_));
    QNetworkRequest request(url);
    QString token = settings_.value("token").toString();
    request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    socket_->open(request);
}

void SslWebsocket::disconnectFromServer() {
    socket_->disconnected();
}

std::vector<unsigned char> SslWebsocket::generateKaypair() {
    unsigned char pk[crypto_box_PUBLICKEYBYTES];
    unsigned char sk[crypto_box_SECRETKEYBYTES];
    crypto_box_keypair(pk, sk);
    QByteArray pkBytes(reinterpret_cast<const char*>(pk), crypto_box_PUBLICKEYBYTES);
    messenger::KeyExchange kx;
    kx.setPublickKey(pkBytes);
    messenger::WriteMessage msg;
    msg.setKeyExchange(std::move(kx));
    QByteArray binaryData = serializer_.serialize(&msg);

    socket_->sendBinaryMessage(pkBytes);
    qDebug() << "отправили pk:" << pkBytes.toHex();
    std::vector<unsigned char> sk_vec(sk, sk + crypto_box_SECRETKEYBYTES);
    return sk_vec;
}

void SslWebsocket::sendMessage(const QString& message, const QString& recip_id) {
    if(message.isEmpty()) {
        qWarning() << "Message is empty";
        return;
    }
    unsigned char nonce[crypto_box_NONCEBYTES];
    randombytes_buf(nonce, crypto_box_NONCEBYTES);
    std::vector<unsigned char> encrypted(message.size() + crypto_box_MACBYTES);
    crypto_box_easy_afternm(
        encrypted.data(),
        reinterpret_cast<const unsigned char*>(message.toUtf8().constData()),
        message.size(),
        nonce,
        shared_key_.data());

    messenger::SecureEnvelope envelope;
    QByteArray q_nonce(reinterpret_cast<const char*>(nonce), crypto_box_NONCEBYTES);
    QByteArray q_encrypted(reinterpret_cast<const char*>(encrypted.data()), static_cast<int>(encrypted.size()));
    QString sender_id = settings_.value("user_id").toString();
    envelope.setNonce(q_nonce);
    envelope.setCiphertext(q_encrypted);
    envelope.setRecipientId(recip_id);
    envelope.setSenderId(std::move(sender_id));
    messenger::WriteMessage wm;
    wm.setChatMessage(std::move(envelope));
    QByteArray binaryData = serializer_.serialize(&wm);

    socket_->sendBinaryMessage(binaryData);
}

void SslWebsocket::onBinaryMessageReceived(const QByteArray &data) {
    messenger::WriteMessage msg;

    if(!msg.deserialize(&serializer_, data)) {
        qWarning() << "Protobuf parse failed";
        return;
    }

    switch (msg.payloadField()) {
    case messenger::WriteMessage::PayloadFields::KeyExchange:
        handleServerPublickKey(msg.keyExchange());
        break;
    case messenger::WriteMessage::PayloadFields::ChatMessage:
        handleChatMessage(msg.chatMessage());
    default:
        break;
    }
}

void SslWebsocket::handleChatMessage(const messenger::SecureEnvelope& msg) {
    QByteArray ciphertext = msg.ciphertext();
    std::string sender_id = msg.senderId().toStdString();
    std::string recip_id = msg.recipientId().toStdString();
    QByteArray nonce = msg.nonce();

    unsigned char decrypted[ciphertext.size() - crypto_box_MACBYTES];
    if(crypto_box_open_easy_afternm(decrypted,
                                     reinterpret_cast<unsigned char*>(ciphertext.data()),
                                     static_cast<int>(ciphertext.size()),
                                     reinterpret_cast<unsigned char*>(nonce.data()),
                                     shared_key_.data()) != 0) {
        qWarning() << "Handle char message error";
        return;
    }
    QByteArray q_decrypted(reinterpret_cast<const char*>(decrypted), ciphertext.size() - crypto_box_MACBYTES);
    QString message(q_decrypted);
}

void SslWebsocket::handleServerPublickKey(const messenger::KeyExchange& pub_key) {
    std::vector<unsigned char> other_pk(pub_key.publickKey().constData(), pub_key.publickKey().constData() + pub_key.publickKey().size());
    unsigned char shared_key[crypto_box_BEFORENMBYTES];
    if(crypto_box_beforenm(shared_key, other_pk.data(), key_.data()) != 0) {
        m_errorMessage = "Failed to compute shared_key";
        return;
    }
    std::vector<unsigned char> shared_key_vec(shared_key, shared_key + crypto_box_BEFORENMBYTES);
    shared_key_ = std::move(shared_key_vec);
}

void SslWebsocket::onSslErrors(const QList<QSslError> &errors)
{
    for (const QSslError &e : errors) {
        qWarning() << "SSL ошибка:" << e.errorString();
    }
    // Это отключает проверку сертификата! Только для разработки.
    socket_->ignoreSslErrors();
}

void SslWebsocket::onError(QAbstractSocket::SocketError error) {
    Q_UNUSED(error);
    m_errorMessage = socket_->errorString();
    emit errorMessageChanged();
    qWarning() << "WebSocket ошибка:" << m_errorMessage;
}

void SslWebsocket::onConnected() {
    m_errorMessage.clear();
    emit errorMessageChanged();
    emit connectedChanged();
    qDebug() << "WebSocket подключён!";
}

void SslWebsocket::onDisconnected() {
    m_errorMessage = "Websocket stream disconnected";
    emit errorMessageChanged();
    emit discconnectedChanged();
    qDebug() << "WebSocket отключен!";
}