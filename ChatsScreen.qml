import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    width: parent.width
    height: parent.height

    Rectangle {
        anchors.fill: parent
        color: "#0F172A"

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            ListModel {
                id: chatModel
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
                                text: model.chatName[0]
                                color: "white"
                                font.bold: true
                                font.pixelSize: 18
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: model.chatName
                                color: "white"
                                font.bold: true
                                font.pixelSize: 16
                                Layout.fillWidth: true
                            }

                            Text {
                                text: model.lastMessage
                                color: "#8FA0C0"
                                font.pixelSize: 14
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                        }

                        Text {
                            text: model.time
                            color: "#8FA0C0"
                            font.pixelSize: 12
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            console.log("Кликнули на чат с: " + model.chatName)
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
                    chatModel.clear()
                    for (let i = 0; i < chats.length; i++) {
                        console.log("чат:", JSON.stringify(chats[i]))
                        chatModel.append(chats[i])
                    }
                    console.log("в модели теперь:", chatModel.count)
                }
            }
        }
    }
}
