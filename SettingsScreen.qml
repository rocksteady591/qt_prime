import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root
    width: parent.width
    height: parent.height

    // фон
    Rectangle {
        anchors.fill: parent
        color: "#0F1420"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // ----------exit button------
        Button {
            contentItem: Text{
                text: "Назад"
                color: "white"
                font.pixelSize: 16
            }
            onClicked: {
                stackView.pop()
            }
        }

        // ---------- title ----------
        Text {
            text: "Настройки"
            color: "white"
            font.pixelSize: 28
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 10
        }

        // ---------- user info ----------
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            color: "#1C2333"
            radius: 20

            RowLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 20

                // avatar
                Rectangle {
                    width: 72
                    height: 72
                    radius: 36
                    color: "#2A82FF"

                    Text {
                        anchors.centerIn: parent
                        text: SslClient.username.length > 0 ? SslClient.username.substring(0, 1).toUpperCase() : "?"
                        color: "white"
                        font.pixelSize: 32
                        font.bold: true
                    }
                }

                ColumnLayout {
                    spacing: 4

                    Text {
                        text: SslClient.username
                        color: "white"
                        font.pixelSize: 20
                        font.bold: true
                    }
                    Text {
                        text: "ID: " + SslClient.userId
                        color: "#8FA0C0"
                        font.pixelSize: 14
                    }
                }

                Item { Layout.fillWidth: true } // распорка
            }
        }   // ← ВОТ ЭТО ЗАКРЫТИЕ ПРОПАЛО У ТЕБЯ

        // ---------- список опций ----------
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 55
            color: "#1C2333"
            radius: 15

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20

                Text {
                    text: "🔔  Уведомления"
                    color: "white"
                    font.pixelSize: 16
                }
                Item { Layout.fillWidth: true }
                Switch { checked: true }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 55
            color: "#1C2333"
            radius: 15

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20

                Text {
                    text: "🌙  Тёмная тема"
                    color: "white"
                    font.pixelSize: 16
                }
                Item { Layout.fillWidth: true }
                Switch { checked: true }
            }
        }

        // ---------- распорка, чтобы кнопка уехала вниз ----------
        Item { Layout.fillHeight: true }

        // ---------- кнопка разлогина ----------
        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 55

            background: Rectangle {
                radius: 15
                color: "#2B1A22"
                border.color: "#FF6B6B"
                border.width: 1
            }

            contentItem: Text {
                text: "Выйти из аккаунта"
                color: "#FF6B6B"
                font.bold: true
                font.pixelSize: 16
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: confirmDialog.open()   // ← открываем диалог
        }
    }   // закрыт ColumnLayout

    // ---------- диалог подтверждения ----------
    // Вынесен из ColumnLayout — Dialog живёт поверх всего
    Dialog {
        id: confirmDialog
        title: "Выход"
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.No

        Text {
            text: "Точно хотите выйти из аккаунта?"
            color: "white"
        }

        onAccepted: {
            SslClient.logout()
        }
    }

    // ---------- реакция на сигнал loggedOut ----------
    Connections {
        target: SslClient
        function onLoggedOut() {
            stackView.pop(null)
            stackView.push("LoginScreen.qml")
        }
    }
}