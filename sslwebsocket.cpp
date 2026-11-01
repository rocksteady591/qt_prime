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
    generateKaypair();
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

void SslWebsocket::generateKaypair() {
    unsigned char pk[crypto_box_PUBLICKEYBYTES];
    unsigned char sk[crypto_box_SECRETKEYBYTES];
    crypto_box_keypair(pk, sk);
    public_key_.assign(pk, pk + crypto_box_PUBLICKEYBYTES);
    secret_key_.assign(sk, sk + crypto_box_SECRETKEYBYTES);
}

void SslWebsocket::sendPublicKey() {
    messenger::KeyExchange kx;
    kx.setPublickKey(QByteArray(
        reinterpret_cast<const char*>(public_key_.data()),
        static_cast<int>(public_key_.size())));

    messenger::WriteMessage msg;
    msg.setKeyExchange(kx);

    QByteArray binaryData = serializer_.serialize(&msg);
    socket_->sendBinaryMessage(binaryData);
    qDebug() << "отправили pk" << binaryData.size() << "байт";
}

void SslWebsocket::sendMessage(const QString& message, const QString& recip_id) {
    if (message.isEmpty()) { qWarning() << "Message is empty"; return; }
    if (shared_key_.empty()) { qWarning() << "Shared key is empty"; return; }

    bool ok = false;
    int recip_int = recip_id.toInt(&ok);
    if (!ok || recip_int <= 0) { qWarning() << "invalid recip_id:" << recip_id; return; }

    QString sender_id = settings_.value("user_id").toString();
    if (sender_id.isEmpty() || sender_id == "0") {
        qWarning() << "user_id is not set";
        return;
    }

    QByteArray utf8 = message.toUtf8();
    unsigned char nonce[crypto_box_NONCEBYTES];
    randombytes_buf(nonce, crypto_box_NONCEBYTES);

    std::vector<unsigned char> encrypted(utf8.size() + crypto_box_MACBYTES);
    crypto_box_easy_afternm(
        encrypted.data(),
        reinterpret_cast<const unsigned char*>(utf8.constData()),
        utf8.size(),
        nonce,
        shared_key_.data());

    messenger::SecureEnvelope envelope;
    envelope.setNonce(QByteArray(reinterpret_cast<const char*>(nonce), crypto_box_NONCEBYTES));
    envelope.setCiphertext(QByteArray(reinterpret_cast<const char*>(encrypted.data()),
                                      static_cast<int>(encrypted.size())));
    envelope.setSenderId(sender_id);
    envelope.setRecipientId(recip_id);

    messenger::WriteMessage wm;
    wm.setChatMessage(envelope);
    QByteArray binaryData = serializer_.serialize(&wm);
    socket_->sendBinaryMessage(binaryData);
    qDebug() << "sent" << binaryData.size() << "bytes to" << recip_id;
}

void SslWebsocket::onBinaryMessageReceived(const QByteArray &data) {
    messenger::WriteMessage msg;

    if(!msg.deserialize(&serializer_, data)) {
        qWarning() << "Protobuf parse failed";
        m_errorMessage = "Protobuf parse failed";
        emit errorMessageChanged();
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
    if(shared_key_.empty()) {
        qWarning() << "Shared key is empty";
        m_errorMessage = "Shared key is empty";
        emit errorMessageChanged();
        return;
    }

    QByteArray ciphertext = msg.ciphertext();
    QByteArray nonce = msg.nonce();

    if (nonce.size() != crypto_box_NONCEBYTES) {
        qWarning() << "bad nonce size:" << nonce.size();
        m_errorMessage = "bad nonce size";
        emit errorMessageChanged();
        return;
    }
    if (ciphertext.size() < crypto_box_MACBYTES) {
        qWarning() << "ciphertext too small";
        m_errorMessage = "ciphertext too small";
        emit errorMessageChanged();
        return;
    }

    std::string sender_id = msg.senderId().toStdString();
    std::string recip_id = msg.recipientId().toStdString();


    unsigned char decrypted[ciphertext.size() - crypto_box_MACBYTES];
    if(crypto_box_open_easy_afternm(decrypted,
                                     reinterpret_cast<unsigned char*>(ciphertext.data()),
                                     static_cast<int>(ciphertext.size()),
                                     reinterpret_cast<unsigned char*>(nonce.data()),
                                     shared_key_.data()) != 0) {
        qWarning() << "Handle char message error";
        m_errorMessage = "Jandle char message error";
        emit errorMessageChanged();
        return;
    }
    QByteArray q_decrypted(reinterpret_cast<const char*>(decrypted), ciphertext.size() - crypto_box_MACBYTES);
    QString message(q_decrypted);
    emit messageReeceived(std::stoi(sender_id), std::stoi(recip_id), message);
}

void SslWebsocket::handleServerPublickKey(const messenger::KeyExchange& pub_key) {
    std::vector<unsigned char> other_pk(pub_key.publickKey().constData(), pub_key.publickKey().constData() + pub_key.publickKey().size());
    unsigned char shared_key[crypto_box_BEFORENMBYTES];
    if(crypto_box_beforenm(shared_key, other_pk.data(), secret_key_.data()) != 0) {
        m_errorMessage = "Failed to compute shared_key";
        m_errorMessage = "Fialed to compute shared key";
        emit errorMessageChanged();
        return;
    }
    std::vector<unsigned char> shared_key_vec(shared_key, shared_key + crypto_box_BEFORENMBYTES);
    shared_key_ = std::move(shared_key_vec);
}

void SslWebsocket::onSslErrors(const QList<QSslError> &errors)
{
    for (const QSslError &e : errors) {
        qWarning() << "SSL ошибка:" << e.errorString();
        m_errorMessage = "Ssl error";
        emit errorMessageChanged();
    }
    // Это отключает проверку сертификата! Только для разработки.
    socket_->ignoreSslErrors();
}

void SslWebsocket::onError(QAbstractSocket::SocketError error) {
    Q_UNUSED(error);
    m_errorMessage = socket_->errorString();
    emit errorMessageChanged();
    qWarning() << "WebSocket ошибка:" << m_errorMessage;
    m_errorMessage = "Websocket error";
    emit errorMessageChanged();
}

void SslWebsocket::onConnected() {
    m_errorMessage.clear();
    sendPublicKey();
    emit errorMessageChanged();
    emit connectedChanged();
    qDebug() << "WebSocket подключён!";
    m_errorMessage = "Websocket is connected";
    emit errorMessageChanged();
}

void SslWebsocket::onDisconnected() {
    m_errorMessage = "Websocket stream disconnected";
    emit errorMessageChanged();
    emit discconnectedChanged();
    qDebug() << "WebSocket отключен!";
}