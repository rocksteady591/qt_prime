#ifndef MESSAGEMODEL_H
#define MESSAGEMODEL_H
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

class MessageModel : public QSqlQueryModel {
    Q_OBJECT
    Q_PROPERTY(int chatId READ chatId WRITE setChatId NOTIFY chatIdChanged)
    Q_PROPERTY(QString currentUserId READ currentUserId WRITE setCurrentUserId NOTIFY currentUserIdChanged)
public:
    MessageModel(QObject* parent = nullptr) : QSqlQueryModel(parent) {}

    int chatId() const {return m_chat_id;}

    void setChatId(int new_id) {
        if (new_id == m_chat_id) {
            return;
        }
        m_chat_id = new_id;
        refresh();
        emit chatIdChanged();
    }

    QString currentUserId() const {return m_user_id;}
    void setCurrentUserId(const QString& new_id) {
        if (m_user_id == new_id) {
            return;
        }
        m_user_id = new_id;
        refresh();
        emit currentUserIdChanged();
    }

    QVariant data(const QModelIndex& index, int role) const override {
        if (role < Qt::UserRole) {
            return QSqlQueryModel::data(index, role);
        }

        int col = role - Qt::UserRole - 1;
        QVariant rawData = QSqlQueryModel::data(this->index(index.row(), col), Qt::DisplayRole);

        if (role == Qt::UserRole + 1) {
            return rawData.toString();
        }else if(role == Qt::UserRole + 2) {
            QString senderId = rawData.toString();
            return (m_user_id == senderId);
        }else if(role == Qt::UserRole + 3) {
            qint64 ts = rawData.toLongLong();
            return QDateTime::fromSecsSinceEpoch(ts).toString("HH:mm");
        }
        return QVariant();
    }

    QHash<int, QByteArray> roleNames() const override {
        QHash<int, QByteArray> roles;
        roles[Qt::UserRole + 1] = "text";
        roles[Qt::UserRole + 2] = "isOwn";
        roles[Qt::UserRole + 3] = "time";
        return roles;
    }

    Q_INVOKABLE void refresh() {
        if (m_chat_id < 0) { return; }

        QSqlQuery query;
        query.prepare("SELECT text, sender_id, timestamp FROM messages WHERE chat_id = :chatId ORDER BY timestamp ASC, id ASC"); // [FIX 5] id как второй ключ: стабильный порядок
        query.bindValue(":chatId", m_chat_id);

        if(!query.exec()) {
            qWarning() << "Ошибка SELECT:" << query.lastError().text();
            return;
        }
        setQuery(query);

        qDebug() << "MessageModel::refresh() для chatId =" << m_chat_id << "загружено строк:" << rowCount();
    }
signals:
    void chatIdChanged();
    void currentUserIdChanged();
private:
    int m_chat_id = -1;
    QString m_user_id;
};

#endif // MESSAGEMODEL_H