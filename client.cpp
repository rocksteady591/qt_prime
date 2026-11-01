#include "client.h"
#include "sodium.h"
#include <QCryptographicHash>
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
        }
        emit chatsLoaded(chats);
    });
}

void SslClient::getChatMessages(int chat_id, int count_messages, int offset) {
    if (chat_id <= 0) {
        emit messagesLoaded({});   // новый чат — истории нет
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

    connect(reply, &QNetworkReply::finished, this, [reply, this](){
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
            m_errorMessage = "doc don't array";
            emit errorMessageChanged();
            emit messagesLoaded({});
            return;
        }

        QJsonObject body = doc.object();

        const QJsonArray arr = doc.array();
        QVariantList res;
        res.reserve(arr.size());
        for (const QJsonValue& val : arr) {
            const QJsonObject obj = val.toObject();
            QVariantMap el;
            el["msg_id"]    = obj.value("msg_id").toInt();
            el["chat_id"]   = obj.value("chat_id").toInt();
            el["sender_id"] = obj.value("sender_id").toInt();
            el["text"]      = obj.value("text").toString();
            el["send_time"] = obj.value("send_time").toString();
            res.append(el);
        }
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


