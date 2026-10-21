import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root
    width: parent.width
    height: parent.height

    property int chatId: 0
    property string username: ""
    property int recipId: 0

    ListModel {
        id: messageModel
    }

    Rectangle {
        anchors.fill: parent
        color: "#0F172A"

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            //header назад + имя

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                color: "#1C2333"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 20
                    spacing: 10

                    Button {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        background: Rectangle { color: "transparent" }
                        contentItem: Text {
                            text: "←"
                            color: "white"
                            font.pixelSize: 22
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: stackView.pop()
                    }

                    //avatar
                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        radius: 20
                        color: "#3B82F6"

                        Text {
                            anchors.centerIn: parent
                            text: root.username.length > 0 ? root.username[0].toUpperCase() : "?"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 16
                        }
                    }

                    Text {
                        text: root.username
                        color: "white"
                        font.bold: true
                        font.pixelSize: 18
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }
            }
            //message list
            ListView {
                id: messageList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 6
                topMargin: 12
                bottomMargin: 12

                model: messageModel

                onCountChanged: {
                    Qt.callLater(() => positionViewAtEnd())
                }

                delegate: Item {
                    width: messageList.width
                    // высота = высоте bubble + отступы
                    height: bubble.height + 8

                    Rectangle {
                        id: bubble

                        //ширина не более 70%
                        width: Math.min(messageText.implicitWidth + 24, messageList.width * 0.7)
                        height: messageText.implicitHeight + 16
                        radius: 14

                        anchors.right: model.isOwn ? parent.right : undefined
                        anchors.left: model.isOwn ? undefined : parent.left
                        anchors.rightMargin: model.isOwn ? 12 : 0
                        anchors.leftMargin:  model.isOwn ? 0  : 12
                        anchors.top: parent.top
                        anchors.topMargin: 4

                        // цвет: свои — синие, чужие — тёмные
                        color: model.isOwn ? "#2A82FF" : "#1C2333"

                        Text {
                            id: messageText
                            anchors.centerIn: parent
                            text: model.text
                            color: "white"
                            font.pixelSize: 15
                            wrapMode: Text.Wrap
                            width: bubble.width - 24 //перено строк
                        }
                    }
                    Text {
                        text: model.time || ""
                        color: "#8FA0C0"
                        font.pixelSize: 11
                        anchors.top: bubble.bottom
                        anchors.topMargin: 2
                        anchors.right: model.isOwn ? parent.right : undefined
                        anchors.left:  model.isOwn ? undefined   : parent.left
                        anchors.rightMargin: 16
                        anchors.leftMargin: 16
                    }
                }
            }
            //панель ввода
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 64
                color: "#1C2333"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    anchors.topMargin: 8
                    anchors.bottomMargin: 8
                    spacing: 10

                    TextField {
                        id: messageInput
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        placeholderText: "Сообщение..."
                        color: "white"
                        placeholderTextColor: "#8FA0C0"
                        font.pixelSize: 15
                        leftPadding: 15
                        rightPadding: 15

                        background: Rectangle {
                            color: "#0F172A"
                            radius: 22
                            border.color: "#2A3447"
                            border.width: 1
                        }

                        // отправка по Enter
                        onAccepted: root.sendCurrentMessage()
                    }

                    // КРУГЛАЯ КНОПКА ОТПРАВКИ
                    Button {
                        Layout.preferredWidth: 44
                        Layout.preferredHeight: 44

                        background: Rectangle {
                            radius: 22
                            color: messageInput.text.length > 0
                                   ? "#2A82FF"
                                   : "#2A3447"
                        }

                        contentItem: Text {
                            text: "➤"                     // символ отправки
                            color: "white"
                            font.pixelSize: 18
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: root.sendCurrentMessage()
                    }
                }
            }
        }
    }
    //логика
    function sendCurrentMessage() {
        let txt = messageInput.text
        if(txt.length === 0) return
        addMessage(txt, true)
        WssClient.sendMessage(txt, root.recipId)
        messageInput.clear()
    }

    function addMessage(text, isOwn) {
        let now = Qt.formatDateTime(new Date(), "HH:mm")
        messageModel.append({
                                "text": text,
                                "isOwn": isOwn,
                                "time": now
                            })
    }

    Connections {
        target: WssClient
    }
}
