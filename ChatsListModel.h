#ifndef CHATSLISTMODEL_H
#define CHATSLISTMODEL_H
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

class ChatsListModel : public QSqlQueryModel {
    Q_OBJECT
public:
    ChatsListModel(QObject* parent = nullptr) : QSqlQueryModel(parent) {}

    QHash<int, QByteArray> roleNames() const override {
        QHash<int, QByteArray> roles;
        roles[Qt::UserRole + 1] = "chatId";        // SELECT: id
        roles[Qt::UserRole + 2] = "user1Id";       // SELECT: user1_id
        roles[Qt::UserRole + 3] = "user2Id";       // SELECT: user2_id
        roles[Qt::UserRole + 4] = "lastMsg";       // SELECT: text
        roles[Qt::UserRole + 5] = "timestamp";     // SELECT: timestamp
        roles[Qt::UserRole + 6] = "recipUsername"; // SELECT: recip_username
        return roles;
    }

    QVariant data(const QModelIndex& index, int role) const override {
        if (role < Qt::UserRole) return QSqlQueryModel::data(index, role);
        int col = role - Qt::UserRole - 1;
        return QSqlQueryModel::data(this->index(index.row(), col), Qt::DisplayRole);
    }

    Q_INVOKABLE void refresh() {
        QSqlQuery query;
        // В вашей таблице chats нет поля recip_username, поэтому мы выводим ID или title
        query.prepare("SELECT id, user1_id, user2_id, text, timestamp, recip_username FROM chats ORDER BY timestamp DESC");
        if(!query.exec()) {
            qWarning() << "Ошибка SELECT ChatListModel:" << query.lastError().text();
            return;
        }
        setQuery(query);
    }
signals:

private:

};

#endif // CHATSLISTMODEL_H
