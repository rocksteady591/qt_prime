import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Chats 1.0

Item {
    width: parent.width
    height: parent.height

    Rectangle {
        anchors.fill: parent
        color: "#0F172A"

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            ChatsListModel {
                id: chatModel
                Component.onCompleted: refresh()
            }

            //Shader Effect
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                color: "#1C2333"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20

                    Text {
                        text: "Чаты"
                        color: "white"
                        font.pixelSize: 22
                        font.bold: true
                    }

                    Button {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        background: Rectangle {
                            color: "#2A3447"
                            radius: 20
                        }

                        contentItem: Text {
                            text: "🔍"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            console.log("The search button pressed");
                            stackView.push("SearchScreen.qml")
                        }
                    }
                    Text {
                        id: errorText
                        text: SslClient.errorMessage
                        color: "red"
                        font.pixelSize: 14

                        Layout.alignment: Qt.AlignHCenter
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter // Центрирование самого текста внутри компонента

                        // Текст виден только тогда, когда он не пустой
                        visible: text !== ""

                        wrapMode: Text.WordWrap
                    }
                }
            }

            //СПИСОК ЧАТОВ
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true // Забирает всё оставшееся пространство посередине
                clip: true
                model: chatModel


                delegate: Item {
                    width: parent.width
                    height: 75

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 20
                        anchors.rightMargin: 20
                        spacing: 15

                        Rectangle {
                            Layout.preferredWidth: 50
                            Layout.preferredHeight: 50
                            radius: 25
                            color: "#3B82F6"

                            Text {
                                anchors.centerIn: parent
                                text: {
                                    // [FIX 6] было name.length / name[0] - переменной name нет
                                    const uname = model.recipUsername || ""
                                    return uname.length > 0 ? uname[0].toUpperCase() : "?"
                                }

                                color: "white"
                                font.bold: true
                                font.pixelSize: 18
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: model.recipUsername
                                color: "white"
                                font.bold: true
                                font.pixelSize: 16
                                Layout.fillWidth: true
                            }

                            Text {
                                text: model.lastMsg || ""
                                color: "#8FA0C0"
                                font.pixelSize: 14
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                        }

                        Text {
                            text: model.timestamp || "" // [FIX 6] было model.model.create_timestamp
                            color: "#8FA0C0"
                            font.pixelSize: 12
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            console.log("Кликнули на чат с: " + model.recipUsername) // [FIX 6] chatName не существует
                            stackView.push("ChatScreen.qml", {
                                chatId: model.chatId,
                                username : model.recipUsername,
                                recipId:   (model.user1Id === parseInt(SslClient.userId))
                                                    ? model.user2Id
                                                    : model.user1Id
                            })
                            // [FIX 6] errorText = "" присваивало значение id-объекту (ошибка); текст берётся из SslClient.errorMessage
                        }
                    }
                }
            }

            //
            //НИЖНЯЯ ПАНЕЛЬ НАВИГАЦИИ (Футер)
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 65
                color: "#1C2333"

                RowLayout {
                    anchors.fill: parent

                    Button {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        background: Rectangle { color: "transparent" }
                        contentItem: Text {
                            text: "💬 Чаты"
                            color: "#2A82FF"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        background: Rectangle { color: "transparent" }
                        contentItem: Text {
                            text: "👤 Контакты"
                            color: "#8FA0C0"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            console.log("Переход в контакты")
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        background: Rectangle { color: "transparent" }
                        contentItem: Text {
                            text: "⚙️ Настройки"
                            color: "#8FA0C0"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            stackView.push("SettingsScreen.qml")
                        }
                    }
                }
            }

            Connections {
                target: SslClient

                function onChatsLoaded(chats) {
                    console.log("пришло чатов:", chats.length)
                    // [FIX 6] у QSqlQueryModel нет append(); чаты уже записаны в БД в getChats(), достаточно перечитать
                    chatModel.refresh()
                }
            }

            // [FIX 6] новое входящее сообщение обновляет последнее сообщение/создаёт чат -> перечитать список
            Connections {
                target: WssClient
                function onMessageSaved(chat_id) {
                    chatModel.refresh()
                }
            }
        }
    }
}
