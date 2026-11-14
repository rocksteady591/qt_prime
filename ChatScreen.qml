import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import Chat 1.0

Item {
    id: root
    width: parent.width
    height: parent.height

    property string username: ""
    property string userLogin: ""
    property int recipId: 0
    property int chatId: 0
    property url image_path: ""

    MessageModel {
        id: messageModel
        chatId: root.chatId
        currentUserId: SslClient.userId
    }

    Component.onCompleted: {
        console.log("Открыт чат с ID:", root.chatId, "с пользователем:", root.username)
        if (root.chatId > 0) {
            SslClient.getChatMessages(root.chatId)
        }
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
                    Text {
                        id: errorText
                        text: SslClient.errorMessage
                        color: "red"
                        font.pixelSize: 14
                        Layout.alignment: Qt.AlignHCenter
                        horizontalAlignment: Text.AlignHCenter // Центрирование самого текста внутри компонента

                        // Текст виден только тогда, когда он не пустой
                        visible: text !== ""

                        wrapMode: Text.WordWrap
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
                    height: bubble.height + 20

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
                        color: model.isOwn ? "#2A82FF" : "#334155" // [FIX 7] чужой пузырь был #1C2333 и сливался с фоном #0F172A

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

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: root.image_path.toString() !== "" ? 80 : 0
                visible: root.image_path.toString() !== ""
                color: "#1C2333"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10

                    Image {
                        source: root.image_path
                        Layout.preferredHeight: 60
                        Layout.preferredWidth: 60
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                    }

                    Button {
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 30
                        background: Rectangle {
                            radius: 15
                            color: "#FF6B6B"
                        }
                        contentItem: Text {
                            text: "✖"
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 14
                        }
                        onClicked: root.image_path = ""
                        Item { Layout.fillWidth: true }
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

                    Button {
                        Layout.preferredWidth: 44
                        Layout.preferredHeight: 44

                        background: Rectangle {
                            radius: 22
                            color: "#2A3447"
                        }

                        contentItem: Text {
                            text: "📎"
                            color: "white"
                            font.pixelSize: 18
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: imageDialog.open()
                    }

                    FileDialog {
                        id: imageDialog
                        title: "Выберите изображение"
                        nameFilters: ["Изображения (*.png *.jpg *.jpeg)"]
                        currentFolder: shortcuts.pictures
                        onAccepted: {
                            console.log("Выбран файл:", imageDialog.selectedFile)
                            root.image_path = imageDialog.selectedFile
                        }
                    }

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
            Connections {
                target: WssClient
                function onMessageSaved(chat_id) {
                    if (chat_id === root.chatId) {
                        messageModel.refresh()
                    }
                }
            }

            Connections {
                target: SslClient
                function onMessagesLoaded(messages) {
                    console.log("История сообщений загружена с сервера, обновляем модель")
                    messageModel.refresh()
                }
            }
        }
    }

    //логика
    function sendCurrentMessage() {
        let txt = messageInput.text
        let img = root.image_path
        if(txt.length === 0) return
        SslClient.sendMessageAndSaveToDb(txt, root.recipId, root.chatId, messageModel.currentUserId);
        WssClient.sendMessage(txt, root.recipId)
        //надо добавить сохранение в бд и очищение свойства пути изображения
        if (img.toString() !== "") {
            root.image_path = ""
            WssClient.sendImage(img, root.recipId)
        }

        messageInput.clear()
        messageModel.refresh()
    }
}
