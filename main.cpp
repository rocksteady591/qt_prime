#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include "client.h"
#include "sslwebsocket.h"
#include <sodium.h>
#include "MessageModel.h"
#include "ChatsListModel.h"

const int EXPECTED_SCHEMA_VERSION = 2;

bool runMigrations(QSqlDatabase& db) {
    QSqlQuery query(db);

    int currentVersion = 0;
    if (query.exec("PRAGMA user_version") && query.next()) {
        currentVersion = query.value(0).toInt();
    }

    qDebug() << "Текущая версия схема БД: " << currentVersion;

    if (currentVersion > EXPECTED_SCHEMA_VERSION) {
        qCritical() << "Версия базы данных" << currentVersion
                    << "новее, чем поддерживает приложение" << EXPECTED_SCHEMA_VERSION;
        return false;
    }

    if (!db.transaction()) {
        qWarning() << "Не удалось начать транзакцию:" << db.lastError().text();
        return false;
    }

    // --- МИГРАЦИЯ 0 -> 1 (Создание начальных таблиц) ---
    if (currentVersion < 1) {
        qDebug() << "Применение миграции до версии 1 (создание таблиц)";

        bool ok = query.exec("CREATE TABLE IF NOT EXISTS chats ("
                             "id INTEGER PRIMARY KEY,"
                             "user1_id INTEGER NOT NULL UNIQUE,"
                             "user2_id INTEGER NOT NULL UNIQUE,"
                             "text TEXT NOT NULL,"
                             "timestamp TEXT NOT NULL)");
        if (!ok) {
            qWarning() << "Ошибка создания таблицы chats:" << query.lastError().text();
            db.rollback();
            return false;
        }

        ok = query.exec(
            "CREATE TABLE IF NOT EXISTS messages ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    server_id INTEGER UNIQUE,"
            "    chat_id INTEGER NOT NULL,"
            "    sender_id TEXT NOT NULL,"
            "    text TEXT NOT NULL,"
            "    timestamp INTEGER NOT NULL,"
            "    status INTEGER NOT NULL DEFAULT 0"
            ")"
            );
        if (!ok) {
            qWarning() << "Ошибка создания таблицы messages:" << query.lastError().text();
            db.rollback();
            return false;
        }

        // Устанавливаем версию 1
        if (!query.exec("PRAGMA user_version = 1")) {
            qWarning() << "Не удалось установить версию 1:" << query.lastError().text();
            db.rollback();
            return false;
        }
        currentVersion = 1;
    }
    db.commit();
    qDebug() << "Миграции успешно применены. Актуальная версия:" << currentVersion;
    return true;
}

bool connectToDatabase() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    db.setDatabaseName(dataDir + "/chat_cache.sqlite");

    if(!db.open()) {
        qWarning() << "Не удалось открыть базу:" << db.lastError().text();
        return false;
    }
    qDebug() << "База данных открыта:" << db.databaseName();
    return true;
}

bool createTables() {
    QSqlQuery query;

    bool ok = query.exec("CREATE TABLE IF NOT EXISTS chats ("
                         "id INTEGER PRIMARY KEY,"
                         "user1_id INTEGER NOT NULL,"
                         "user2_id INTEGER NOT NULL,"
                         "text TEXT NOT NULL,"
                         "timestamp TEXT NOT NULL,"
                         "recip_username TEXT)");

    if (!ok) {
        qWarning() << "Ошибка создания таблицы chats:" << query.lastError().text();
        return false;
    }

    /*ok = query.exec("CREATE TABLE IF NOT EXISTS contacts ("
                    "id INTEGER PRIMARY KEY,"
                    "user1_id INTEGER NOT NULL UNIQUE,"
                    "user2_id INTEGER NOT NULL UNIQUE)");

    if (!ok) {
        qWarning() << "Ошибка создания таблицы contacts:" << query.lastError().text();
        return false;
    }*/

    ok = query.exec(
        "CREATE TABLE IF NOT EXISTS messages ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    server_id INTEGER,"
        "    chat_id INTEGER NOT NULL,"
        "    sender_id TEXT NOT NULL,"
        "    text TEXT NOT NULL,"
        "    timestamp INTEGER NOT NULL,"
        "    status INTEGER NOT NULL DEFAULT 0"
        ")"
        );
    if (!ok) {
        qWarning() << "Ошибка создания таблицы messages:" << query.lastError().text();
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName("PrimalRussTechnologies");
    QCoreApplication::setApplicationName("Prime");

    if (sodium_init() < 0) {
        qDebug() << "Libsodium not init";
        return 1;
    }

    if (!connectToDatabase()) {
        qDebug() << "Ошибка инициализации базы данных";
        return 1;
    }

    QSqlDatabase db = QSqlDatabase::database();

    // Вызываем миграции вместо createTables()
    /*if (!runMigrations(db)) {
        qDebug() << "Ошибка применения миграций";
        return 1;
    }*/

    if (!createTables()) {
        qDebug() << "Ошибка инициализации таблиц";
        return 1;
    }

    QQmlApplicationEngine engine;

    SslClient client("10.0.2.2", 8081);
    SslWebsocket websocket("10.0.2.2", 9000);
    // WebSocket подключается после успешного логина
    QObject::connect(&client, &SslClient::loginSuccess, [&]() {
        websocket.connectWidthToken();
    });
    QObject::connect(&client, &SslClient::registerSuccess, [&]() {
        websocket.connectWidthToken();
    });

    // Если токен уже есть при старте — подключаем сразу
    if (client.containsToken()) {
        websocket.connectWidthToken();
        client.getChats();
    }
    engine.rootContext()->setContextProperty("WssClient", &websocket);
    engine.rootContext()->setContextProperty("SslClient", &client);

    qmlRegisterType<MessageModel>("Chat", 1, 0, "MessageModel");
    qmlRegisterType<ChatsListModel>("Chats", 1, 0, "ChatsListModel");

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("qt_prime", "Main");

    return QGuiApplication::exec();;
}
