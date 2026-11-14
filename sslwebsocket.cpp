#include "sslwebsocket.h"
#include <QSqlQuery>
#include <QSqlError>   // [FIX 3]
#include <QDateTime>   // [FIX 3]
#include <QDebug>
#include <sodium.h>
#include <QImage>
#include <QBuffer>
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
    // Если сокет уже пытается подключиться или подключен - аварийно закрываем
    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->abort();
    }

    QUrl url(QString("wss://%1:%2/").arg(host_).arg(port_));
    QNetworkRequest request(url);
    QString token = settings_.value("token").toString();
    request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    socket_->open(request);
}

void SslWebsocket::disconnectFromServer() {
    socket_->close(); // [FIX 4] было socket_->disconnected() - это сигнал, соединение не закрывалось
}

void SslWebsocket::closeSocket() {
    qDebug() << "Закрываем WebSocket соединение...";
    if(socket_) {
        socket_->close();
    }
    shared_key_.clear();
    generateKaypair();
    m_errorMessage.clear();
    emit errorMessageChanged();
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



void SslWebsocket::sendImage(const QUrl& image_url, const QString& recip_id) {
    if (image_url.isEmpty()) { qWarning() << "Image is empty"; return; return; }
    if (shared_key_.empty()) { qWarning() << "Shared key is empty"; return; }

    bool ok = false;
    int recip_int = recip_id.toInt(&ok);
    if (!ok || recip_int <= 0) { qWarning() << "invalid recip_id:" << recip_id; return; }

    QString sender_id = settings_.value("user_id").toString();
    if (sender_id.isEmpty() || sender_id == "0") {
        qWarning() << "user_id is not set";
        return;
    }

    QString str_path = image_url.toLocalFile();

    QImage image(str_path);

    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPG", 80);

    qDebug() << "Изображение готово к отправке. Размер:" << imageData.size() << "байт";

    unsigned char nonce[crypto_box_NONCEBYTES];
    randombytes_buf(nonce, crypto_box_NONCEBYTES);

    std::vector<unsigned char> encrypted(imageData.size() + crypto_box_MACBYTES);
    crypto_box_easy_afternm(
        encrypted.data(),
        reinterpret_cast<const unsigned char*>(imageData.constData()),
        imageData.size(),
        nonce,
        shared_key_.data());

    messenger::SecureImage si;
    si.setNonce(QByteArray(reinterpret_cast<const char*>(nonce), crypto_box_NONCEBYTES));
    si.setImageData(QByteArray(reinterpret_cast<const char*>(encrypted.data(), static_cast<int>(encrypted.size()))));
    si.setRecipientId(recip_id);
    si.setSenderId(sender_id);

    messenger::WriteMessage wm;
    wm.setChatImgMessage(std::move(si));

    QByteArray binaryData = serializer_.serialize(&wm);
    socket_->sendBinaryMessage(binaryData);
    qDebug() << "sent" << binaryData.size() << "bytes to" << recip_id;
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
    wm.setChatMessage(std::move(envelope));
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
        break; // [FIX 4] не хватало break
    default:
        break;
    }
}

void SslWebsocket::handleChatImgMessage(const messenger::SecureImage& msg) {

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

    const QString sender_id = msg.senderId();
    const QString recip_id = msg.recipientId();

    if (sender_id == recip_id) {
        qDebug() << "Игнорируем эхо собственного сообщения от сервера";
        return;
    }

    // [FIX 4] VLA на стеке заменён на std::vector (VLA не стандарт C++ и опасен на больших сообщениях)
    std::vector<unsigned char> decrypted(ciphertext.size() - crypto_box_MACBYTES);
    if(crypto_box_open_easy_afternm(decrypted.data(),
                                     reinterpret_cast<const unsigned char*>(ciphertext.constData()),
                                     static_cast<unsigned long long>(ciphertext.size()),
                                     reinterpret_cast<const unsigned char*>(nonce.constData()),
                                     shared_key_.data()) != 0) {
        qWarning() << "Handle chat message error: decrypt failed"; // [FIX 4] опечатка
        m_errorMessage = "Handle chat message error";
        emit errorMessageChanged();
        return;
    }
    // [FIX 4] явный UTF-8 вместо QString(QByteArray)
    const QString message = QString::fromUtf8(reinterpret_cast<const char*>(decrypted.data()),
                                              static_cast<int>(decrypted.size()));

    // [FIX 3] Локальный chat_id определяем по паре пользователей, а не берём
    // msg.chatId(): клиент при отправке chatId в конверт не кладёт, и сервер
    // может его не подставить -> сообщение уходило в БД с chat_id = 0 (или чужим)
    // и не попадало под WHERE chat_id = :chatId в MessageModel.
    const int my_id = settings_.value("user_id").toInt();
    const int other_id = sender_id.toInt();
    int chat_id = 0;
    {
        QSqlQuery q;
        q.prepare("SELECT id FROM chats WHERE (user1_id = :a1 AND user2_id = :b1) "
                  "OR (user1_id = :b2 AND user2_id = :a2) LIMIT 1");
        q.bindValue(":a1", my_id);
        q.bindValue(":b1", other_id);
        q.bindValue(":b2", other_id);
        q.bindValue(":a2", my_id);
        if (q.exec() && q.next()) {
            chat_id = q.value(0).toInt();
        }
    }

    const QString nowIso = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    if (chat_id <= 0) {
        // чата локально нет: если сервер прислал свой chatId - создаём запись
        chat_id = msg.chatId();
        if (chat_id <= 0) {
            qWarning() << "Чат для сообщения не найден и chatId не пришёл. sender:" << sender_id;
            return;
        }
        QSqlQuery cq;
        cq.prepare("INSERT OR IGNORE INTO chats (id, user1_id, user2_id, text, timestamp, recip_username) "
                   "VALUES (:id, :u1, :u2, :text, :ts, :name)");
        cq.bindValue(":id", chat_id);
        cq.bindValue(":u1", other_id);
        cq.bindValue(":u2", my_id);
        cq.bindValue(":text", message);
        cq.bindValue(":ts", nowIso);
        cq.bindValue(":name", QString()); // имя подтянется при getChats()
        if (!cq.exec()) {
            qWarning() << "Ошибка создания чата:" << cq.lastError().text();
        }
    } else {
        // [FIX 3] обновляем последнее сообщение в списке чатов
        QSqlQuery uq;
        uq.prepare("UPDATE chats SET text = :text WHERE id = :id");
        uq.bindValue(":text", message);
        uq.bindValue(":id", chat_id);
        if (!uq.exec()) {
            qWarning() << "Ошибка обновления чата:" << uq.lastError().text();
        }
    }

    qint64 ts = QDateTime::currentSecsSinceEpoch();

    QSqlQuery query;
    query.prepare("INSERT OR IGNORE INTO messages (chat_id, sender_id, text, timestamp, status) "
                  "VALUES (:chat_id, :sender_id, :text, :ts, 0)");
    query.bindValue(":chat_id", chat_id);
    query.bindValue(":sender_id", sender_id);
    query.bindValue(":text", message);
    query.bindValue(":ts", ts);

    if (!query.exec()) {
        qWarning() << "Ошибка вставки в бд при получении с сервера:" << query.lastError().text(); // [FIX 4]
        return;
    }

    qDebug() << "Входящее сохранено, chat_id =" << chat_id << "sender =" << sender_id; // [FIX 3]
    emit messageSaved(chat_id);
}

void SslWebsocket::handleServerPublickKey(const messenger::KeyExchange& pub_key) {
    std::vector<unsigned char> other_pk(pub_key.publickKey().constData(), pub_key.publickKey().constData() + pub_key.publickKey().size());
    unsigned char shared_key[crypto_box_BEFORENMBYTES];
    if(crypto_box_beforenm(shared_key, other_pk.data(), secret_key_.data()) != 0) {
        m_errorMessage = "Failed to compute shared key"; // [FIX 4] убрано дублирование и опечатка
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