#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include "client.h"
#include "sslwebsocket.h"
#include <sodium.h>
#include <iostream>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName("PrimalRussTechnologies");
    QCoreApplication::setApplicationName("Prime");

    if (sodium_init() < 0) {
        std::cerr << "Libsodium not init\n";
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
    }
    engine.rootContext()->setContextProperty("WssClient", &websocket);
    engine.rootContext()->setContextProperty("SslClient", &client);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("qt_prime", "Main");

    return QGuiApplication::exec();;
}
