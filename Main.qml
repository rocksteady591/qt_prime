import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import qt_prime


ApplicationWindow {
    id: window
    width: 640
    height: 480
    minimumWidth: 200
    minimumHeight: 250
    visible: true
    title: qsTr("Prime")
    color: "#0F1626"

    function onAppStart() {
        if (SslClient.containsToken) {
            stackView.push("ChatsScreen.qml")
        }
    }

    Rectangle{
        anchors.fill: parent
        color: "#0B0F19"
        // Декоративные элементы фона (имитация неоновых отсветов)
        Rectangle {
            width: 300; height: 300
            radius: 150
            color: "#2A82FF"
            opacity: 0.1
            anchors { top: parent.top; left: parent.left; margins: -50 }
        }
        Rectangle {
            width: 400; height: 400
            radius: 200
            color: "#FF6B6B"
            opacity: 0.05
            anchors { bottom: parent.bottom; right: parent.right; margins: -100 }
        }
    }

    // Менеджер экранов
    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: "LoginScreen.qml" // Первый экран при запуске
    }

    Component.onCompleted: {
        onAppStart()
    }

    /*Component.onCompleted: {
        var token = ApiWebsocket.get_token();
        ApiWebsocket.set_token(token);
        ApiWebsocket.run("127.0.0.1", "9000");
        if (token !== "") {
            console.log("Токен найден, переходим к чатам!")
            stackView.replace("ChatsScreen.qml")
        } else {
            console.log("Токен не найден, остаемся на экране входа.")
        }
    }*/


    /*TabBar {
           id: tabBar
            width: parent.width
            anchors.bottom: parent.bottom
            background: Rectangle { color: "#181E2E" }

            TabButton { text: "Chats"; icon.source: "qrc:/icons/chat.svg" }
            TabButton { text: "Contacts"; icon.source: "qrc:/icons/contacts.svg" }
            TabButton { text: "Calls"; icon.source: "qrc:/icons/calls.svg" }
            TabButton { text: "Settings"; icon.source: "qrc:/icons/settings.svg" }
        }*/

}