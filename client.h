#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QtNetwork>

QT_REQUIRE_CONFIG(ssl);

class SslClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString errorMessageFound READ errorMessageFound NOTIFY userNotFound)
    Q_PROPERTY(QString username READ username NOTIFY userDataChanged)
    Q_PROPERTY(QString userId READ userId NOTIFY userDataChanged)
public:
    SslClient(const QString& host, quint16 port, QObject *parent = nullptr);
    Q_INVOKABLE void connectToServer();
    Q_INVOKABLE void disconnectFromServer();
    Q_INVOKABLE void login(const QString& login, const QString& password);
    Q_INVOKABLE void registerUser(const QString& login, const QString& password, const QString& passwordConfirm);
    Q_INVOKABLE void findUser(const QString& username);
    QString errorMessage() const { return m_errorMessage; }
    QString errorMessageFound() const { return m_find_status; }
    Q_INVOKABLE bool containsToken() const;
    Q_INVOKABLE QString username() const;
    Q_INVOKABLE QString userId() const;
    Q_INVOKABLE void logout();
    void getChats();
signals:
    void loginSuccess();
    void registerSuccess();
    void errorMessageChanged();
    void connectedChanged();
    void encryptedChanged();   // сигнал «TLS поднят»
    void loggedOut();
    void userDataChanged();
    void userNotFound();
    void chatsLoaded(const QVariantList& chats);
    void userFound(const QVariantMap& user);
private slots:
    void socketStateChanged(QAbstractSocket::SocketState state);
    void socketEncrypted();
    //void socketReadyRead();
    void socketError(QAbstractSocket::SocketError error);
    void sslErrors(const QList<QSslError> &errors);
private:
    //void sendData(const QString& sender_id, const QString& recip_id, const QString& message);
    void setupSecureSocket();
private:
    QSslSocket *socket_ = nullptr;
    QString m_host;
    quint16 m_port = 0;
    QNetworkAccessManager* acc_ = nullptr;
    bool handlingSocketError = false;
    QString m_errorMessage;
    QString m_find_status;
    QSettings settings_;
};

#endif // CLIENT_H
