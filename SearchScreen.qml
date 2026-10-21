import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root
    width: parent.width
    height: parent.height

    Rectangle {
        anchors.fill: parent
        color: "#0F172A"

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // ------- HEADER -------
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                spacing: 10

                Text {
                    text: "Поиск"
                    color: "white"
                    font.pixelSize: 22
                    font.bold: true
                }

                TextField {
                    id: userAndChatsSearch
                    Layout.fillWidth: true
                    placeholderText: "Поиск чатов и пользователей"
                    color: "white"
                    placeholderTextColor: "#8FA0C0"
                    background: Rectangle {
                        color: "#1C2334"
                        radius: 15
                        implicitHeight: 40
                    }

                    // реакция на ввод — запрос в C++ по мере набора
                    onTextChanged: {
                        if (text.length >= 2) {
                            // WssClient.searchUsers(text)   // ← твой метод
                            console.log("поиск:", text)
                        }
                    }
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
                        console.log("нажали поиск:", userAndChatsSearch.text)
                        SslClient.findUser(userAndChatsSearch.text)
                        // WssClient.searchUsers(userAndChatsSearch.text)
                    }
                }

                Button {
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    background: Rectangle {
                        color: "#2A3447"
                        radius: 20
                    }
                    contentItem: Text {
                        text: "❌"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        console.log("закрыть поиск")
                        stackView.pop()
                    }
                }
            }

            Text {
                id: errorText
                text: SslClient.errorMessageFound
                color: "red"
                font.pixelSize: 16
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                visible: text !== ""
                wrapMode: Text.WordWrap
                //onTextChanged: searchModel.clear()
            }

            // ------- РЕЗУЛЬТАТЫ ПОИСКА -------
            ListView {
                id: resultsList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: searchModel

                delegate: Item {
                    width: resultsList.width
                    height: 75

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 20
                        anchors.rightMargin: 20
                        spacing: 15

                        // ---- аватарка с первой буквой username ----
                        Rectangle {
                            Layout.preferredWidth: 50
                            Layout.preferredHeight: 50
                            radius: 25
                            color: "#3B82F6"

                            Text {
                                anchors.centerIn: parent
                                text: {
                                    let name = model.username || model.user_login || ""
                                    return name.length > 0 ? name[0].toUpperCase() : "?"
                                }
                                color: "white"
                                font.bold: true
                                font.pixelSize: 18
                            }
                        }

                        // ---- имя + логин ----
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: model.username || "(без имени)"
                                color: "white"
                                font.bold: true
                                font.pixelSize: 16
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Text {
                                text: "@" + (model.user_login || "")
                                color: "#8FA0C0"
                                font.pixelSize: 14
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                        }

                        // ---- кнопка «Написать» ----
                        Button {
                            text: "Написать"
                            onClicked: {
                                console.log("открыть чат с user_id =", model.user_id)
                                // stackView.push("ChatScreen.qml", {
                                //     userId:    model.user_id,
                                //     userName:  model.username,
                                //     userLogin: model.user_login
                                // })
                            }
                        }
                    }
                }
            }
            Connections {
                target: SslClient
                function onUserFound(user) {
                    searchModel.clear()
                    searchModel.append(user)
                }
                function onUserNotFound() {
                    console.log("не нашли")
                    searchModel.clear()
                }
            }
        }
    }

    ListModel { id: searchModel }
}