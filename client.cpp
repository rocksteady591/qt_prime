#include "client.h"
#include "sodium.h"
#include <QCryptographicHash>
#include <QSqlQuery>
#include <QSqlError>   // [FIX 1] для lastError()
#include <QTimeZone>   // [FIX 1] для разбора времени сервера
#include "message.qpb.h"

SslClient::SslClient(const QString& host, quint16 port, QObject *parent) : QObject(parent), m_host(host), m_port(port) {
    acc_ = new QNetworkAccessManager(this);
    //отключение проверки ssl
    connect(acc_, &QNetworkAccessManager::sslErrors, this,
            [](QNetworkReply *reply, const QList<QSslError> &errors) {
                for (const QSslError &e : errors)
                    qWarning() << "SSL error:" << e.errorString();
                reply->ignoreSslErrors();
            });
    setupSecureSocket();
}

bool SslClient::containsToken() const {
    return settings_.contains("token");
}

void SslClient::sendMessageAndSaveToDb(const QString &text, int recipId, int chatId, int sender_id) {
    qint64 ts = QDateTime::currentSecsSinceEpoch();

    QSqlQuery query;
    query.prepare("INSERT INTO messages (chat_id, sender_id, text, timestamp, status) "
                  "VALUES (:chat_id, :sender_id, :text, :ts, 0)");

    query.bindValue(":chat_id", chatId);
    query.bindValue(":sender_id", sender_id);
    query.bindValue(":text", text);
    query.bindValue(":ts", ts);

    if (!query.exec()) {
        qWarning() << "Ошибка вставки в бд";
    }

}

void SslClient::getChats() {
    QUrl url(QString("https://%1:%2/api/get_chats").arg(m_host).arg(m_port));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", "Bearer " + settings_.value("token").toString().toUtf8());
    QJsonObject body;
    QByteArray json = QJsonDocument(body).toJson(QJsonDocument::Compact);
    QNetworkReply* reply = acc_->post(req, json);

    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        reply->deleteLater();
        if(reply->error() != QNetworkReply::NoError) {
            m_errorMessage = reply->errorString();
            emit errorMessageChanged();
            return;
        }
        QByteArray body = reply->readAll();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(body, &err);

        if(err.error != QJsonParseError::NoError) {
            m_errorMessage = err.errorString();
            emit errorMessageChanged();
            return;
        }

        if (doc.isNull() || doc.isEmpty()) {
            emit chatsLoaded({});
            return;
        }

        QJsonArray chatsArr;

        if(doc.isArray()) {
            chatsArr = doc.array();
        }else if(doc.isObject()) {
            QJsonObject data = doc.object();

            QVariantList chats;

            for(const auto& val : data) {
                chatsArr.append(val);
            }

        }else {
            m_errorMessage = "unexpected server response";
            emit errorMessageChanged();
            return;
        }

        QSqlDatabase db = QSqlDatabase::database();
        db.transaction();

        QSqlQuery query;
        query.prepare("INSERT OR REPLACE INTO chats (id, user1_id, user2_id, text, timestamp, recip_username) "
                      "VALUES (:id, :user1_id, :user2_id, :text, :timestamp, :recip_username)");

        QVariantList chats;
        for (const QJsonValue &val : chatsArr) {
            QJsonObject obj = val.toObject();
            QVariantMap el;
            el["id"]               = obj.value("id").toInt();
            el["user1_id"]         = obj.value("user1_id").toInt();
            el["user2_id"]         = obj.value("user2_id").toInt();
            el["recip_username"]   = obj.value("recip_username").toString();
            el["last_msg"]         = obj.value("last_msg").toString();
            el["create_timestamp"] = obj.value("create_timestamp").toString();
            chats.append(el);

            query.bindValue(":id", obj.value("id").toInt());
            query.bindValue(":user1_id", obj.value("user1_id").toInt());
            query.bindValue(":user2_id", obj.value("user2_id").toInt());
            query.bindValue(":text", obj.value("last_msg").toString());
            query.bindValue(":timestamp", obj.value("create_timestamp").toString());
            query.bindValue(":recip_username", obj.value("recip_username").toString());

            if (!query.exec()) {
                qWarning() << "Ошибка вставки в цикле: чаты";
                db.rollback();
                return;
            }
        }
        db.commit();
        emit chatsLoaded(chats);
    });
}

void SslClient::getChatMessages(int chat_id, int count_messages, int offset) {
    if (chat_id <= 0) {
        emit messagesLoaded({});
        return;
    }
    QUrl url(QString("https://%1:%2/api/get_messages").arg(m_host).arg(m_port));
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", "Bearer " + settings_.value("token").toString().toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body {
        {"count_messages", count_messages},
        {"offset", offset},
        {"chat_id", chat_id}
    };

    QByteArray data = QJsonDocument(body).toJson(QJsonDocument::Compact);
    QNetworkReply* reply = acc_->post(req, data);

    // 👈 ВАЖНО: Захватываем chat_id в лямбду, чтобы использовать его при сохранении!
    connect(reply, &QNetworkReply::finished, this, [reply, this, chat_id](){
        reply->deleteLater();

        if(reply->error() != QNetworkReply::NoError) {
            m_errorMessage = reply->errorString();
            emit errorMessageChanged();
            emit messagesLoaded({});
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);

        if (err.error != QJsonParseError::NoError) {
            m_errorMessage = err.errorString();
            emit errorMessageChanged();
            emit messagesLoaded({});
            return;
        }

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode != 200) {
            m_errorMessage = QString("GetMessages failed: HTTP %1").arg(statusCode);
            emit errorMessageChanged();
            emit messagesLoaded({});
            return;
        }

        if (!doc.isArray()) {
            m_errorMessage = "Server response is not a JSON Array!";
            emit errorMessageChanged();
            emit messagesLoaded({});
            return;
        }

        QSqlDatabase db = QSqlDatabase::database();
        db.transaction();

        QSqlQuery query;
        query.prepare("INSERT OR IGNORE INTO messages (server_id, chat_id, sender_id, text, timestamp, status) "
                      "VALUES (:server_id, :chat_id, :sender_id, :text, :timestamp, :status)");

        const QJsonArray arr = doc.array();
        QVariantList res;
        res.reserve(arr.size());

        // [FIX 1] Разбор JSON не через toInt()/toString(): они возвращают 0 / "",
        // если тип поля в JSON другой (число вместо строки и наоборот).
        // toVariant() конвертирует и то и другое.
        auto parseTime = [](const QJsonValue &v) -> qint64 {
            if (v.isString()) {
                const QString s = v.toString();
                bool isNum = false;
                const qint64 n = s.toLongLong(&isNum);
                if (isNum) return n;                                   // "1731320000"
                QDateTime dt = QDateTime::fromString(s, Qt::ISODate);  // "2026-11-11T10:00:00Z"
                if (!dt.isValid()) {
                    dt = QDateTime::fromString(s, "yyyy-MM-dd HH:mm:ss"); // подстрой под формат сервера
                    if (dt.isValid())
                        dt = QDateTime(dt.date(), dt.time(), QTimeZone::UTC);
                }
                return dt.isValid() ? dt.toSecsSinceEpoch() : 0;
            }
            return v.toVariant().toLongLong();
        };

        for (const QJsonValue& val : arr) {
            const QJsonObject obj = val.toObject();
            qDebug() << "get_messages item:" << obj; // [FIX 1] смотри, какие поля реально приходят

            const qint64 serverId  = obj.value("msg_id").toVariant().toLongLong();  // [FIX 1]
            const QString senderId = obj.value("sender_id").toVariant().toString(); // [FIX 1]
            const qint64 ts        = parseTime(obj.value("send_time"));             // [FIX 1]
            QString text = obj.value("text").toString();
            if (text.isEmpty()) {                                                   // [FIX 1] запасные имена поля
                text = obj.value("message").toString();
                if (text.isEmpty()) text = obj.value("content").toString();
            }
            if (text.isEmpty())
                qWarning() << "get_messages: пустой text, поля объекта:" << obj.keys(); // [FIX 1]

            QVariantMap el;
            el["msg_id"]    = serverId;
            el["chat_id"]   = chat_id;
            el["sender_id"] = senderId;
            el["text"]      = text;
            el["send_time"] = obj.value("send_time").toVariant().toString();
            res.append(el);

            query.bindValue(":server_id", serverId > 0 ? QVariant(serverId) : QVariant()); // [FIX 1] 0 -> NULL, чтобы не ловить UNIQUE
            query.bindValue(":chat_id", chat_id);
            query.bindValue(":sender_id", senderId);
            query.bindValue(":text", text);
            query.bindValue(":timestamp", ts);
            query.bindValue(":status", 0);

            if (!query.exec()) {
                qWarning() << "Ошибка вставки сообщения:" << query.lastError().text(); // [FIX 1] теперь видна причина
            }
        }
        db.commit();
        emit messagesLoaded(res);
    });
}

void SslClient::findUser(const QString& username) {
    if (username.isEmpty()) {
        m_errorMessage = "Username is empty";
        emit errorMessageChanged();
        return;
    }
    QUrl url(QString("https://%1:%2/api/find_user").arg(m_host).arg(m_port));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", "Bearer " + settings_.value("token").toString().toUtf8());
    QJsonObject body {
        {"user_name", username}
    };
    QByteArray json = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QNetworkReply* reply = acc_->post(req, json);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        reply->deleteLater();

        if(reply->error() != QNetworkReply::NoError) {
            m_errorMessage = reply->errorString();
            emit errorMessageChanged();
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);

        if(err.error != QJsonParseError::NoError) {
            m_errorMessage = err.errorString();
            emit errorMessageChanged();
            return;
        }

        if(!doc.isObject()) {
            m_errorMessage = "doc don't object";
            emit errorMessageChanged();
            return;
        }

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if(statusCode != 200) {
            m_find_status = "User not found";
            emit userNotFound();
            return;
        }
        m_errorMessage = "";
        emit errorMessageChanged();
        m_find_status = "";
        emit userNotFound();
        QJsonObject body = doc.object();
        qDebug() << body["user_id"].toInt();
        qDebug() << body["username"].toString();
        qDebug() << body["user_login"].toString();
        QVariantMap variant = body.toVariantMap();
        emit userFound(variant);
    });
}

void SslClient::login(const QString& login, const QString& password) {
    QUrl url(QString("https://%1:%2/api/login").arg(m_host).arg(m_port));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);

    QJsonObject body{
        {"login", login},
        {"password", QString::fromUtf8(hash)}
    };

    QByteArray json = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QNetworkReply* reply = acc_->post(req, json);

    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError){
            m_errorMessage = reply->errorString();
            emit errorMessageChanged();
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError) {
            m_errorMessage = err.errorString();
            emit errorMessageChanged();
            return;
        }

        if (!doc.isObject()) {
            m_errorMessage = "doc don't object";
            emit errorMessageChanged();
            return;
        }

        QJsonObject body = doc.object();

        QString token = body.value("token").toString();
        QString user_id = body.value("user_id").toString();
        QString username = body.value("username").toString();
        settings_.setValue("token", token);
        settings_.setValue("user_id", user_id);
        settings_.setValue("username", username);
        settings_.sync();
        emit userDataChanged();
        emit loginSuccess();
        getChats();
    });
}

void SslClient::registerUser(const QString& login, const QString& password, const QString& passwordConfirm) {
    if (password != passwordConfirm) {
        m_errorMessage = "Password1 != password2";
        emit errorMessageChanged();
        return;
    }
    QUrl url(QString("https://%1:%2/api/register").arg(m_host).arg(m_port));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);

    QJsonObject body {
        {"login", login},
        {"password", QString::fromUtf8(hash)}
    };

    QByteArray json = QJsonDocument(body).toJson(QJsonDocument::Compact);
    QNetworkReply* reply = acc_->post(req, json);

    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        reply->deleteLater();

        if(reply->error() != QNetworkReply::NoError) {
            m_errorMessage = reply->errorString();
            emit errorMessageChanged();
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);

        if (err.error != QJsonParseError::NoError) {
            m_errorMessage = err.errorString();
            emit errorMessageChanged();
            return;
        }

        if (!doc.isObject()) {
            m_errorMessage = "doc don't object";
            emit errorMessageChanged();
            return;
        }

        QJsonObject body = doc.object();
        QString token = body.value("token").toString();
        QString user_id = body.value("user_id").toString();
        QString username = body.value("username").toString();
        settings_.setValue("token", token);
        settings_.setValue("user_id", user_id);
        settings_.setValue("username", username);
        emit userDataChanged();
        emit registerSuccess();
    });
}

QString SslClient::username() const {
    return settings_.value("username").toString();
}

QString SslClient::userId() const {
    return settings_.value("user_id").toString();
}

void SslClient::logout() {
    QSqlQuery query;
    query.exec("DELETE FROM chats");
    query.exec("DELETE FROM message");

    settings_.remove("token");
    settings_.remove("user_id");
    settings_.remove("username");
    settings_.sync();

    emit userDataChanged();
    emit loggedOut();
}

void SslClient::connectToServer() {
    if(socket_->state() != QAbstractSocket::UnconnectedState){
        socket_->abort();
    }
    socket_->connectToHostEncrypted(m_host, m_port);
}

void SslClient::disconnectFromServer(){
    if (socket_ && socket_->state() != QAbstractSocket::UnconnectedState){
        socket_->disconnectFromHost();
    }
}

void SslClient::socketStateChanged(QAbstractSocket::SocketState state) {
    emit connectedChanged();
}

void SslClient::socketEncrypted() {
    emit encryptedChanged();
}

void SslClient::setupSecureSocket(){
    if (socket_) return;
    socket_ = new QSslSocket(this);

    connect(socket_, &QSslSocket::stateChanged, this, &SslClient::socketStateChanged);
    connect(socket_, &QSslSocket::encrypted,    this, &SslClient::socketEncrypted);
    connect(socket_, &QSslSocket::errorOccurred,this, &SslClient::socketError);
    connect(socket_, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors), this, &SslClient::sslErrors);
}

void SslClient::sslErrors(const QList<QSslError> &errors) {
    if(handlingSocketError){
        return;
    }
    handlingSocketError = true;
    m_errorMessage = "Ssl connection failed";
    emit errorMessageChanged();
}

void SslClient::socketError(QAbstractSocket::SocketError){
    if(handlingSocketError){
        return;
    }
    handlingSocketError = true;
    m_errorMessage = "Tcp connection failed";
    emit errorMessageChanged();
}