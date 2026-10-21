import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    width: parent.width
    height: parent.height

    Connections{
        target: SslClient
        function onRegisterSuccess(){
            stackView.pop()
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: parent.width * 0.8
        spacing: 20

        Text {
            text: "Регистрация"
            color: "white"
            font.pixelSize: 32
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 20
        }

        TextField {
            id: regLoginField
            Layout.fillWidth: true
            placeholderText: "Придумайте логин"
            color: "white"
            placeholderTextColor: "#8FA0C0"
            background: Rectangle { color: "#1C2333"; radius: 15; implicitHeight: 50 }
            leftPadding: 15
        }

        TextField {
            id: regPasswordField
            Layout.fillWidth: true
            placeholderText: "Пароль"
            echoMode: TextInput.Password
            color: "white"
            placeholderTextColor: "#8FA0C0"
            background: Rectangle { color: "#1C2333"; radius: 15; implicitHeight: 50 }
            leftPadding: 15
        }

        TextField {
            id: regPasswordConfirmField
            Layout.fillWidth: true
            placeholderText: "Повторите пароль"
            echoMode: TextInput.Password
            color: "white"
            placeholderTextColor: "#8FA0C0"
            background: Rectangle { color: "#1C2333"; radius: 15; implicitHeight: 50 }
            leftPadding: 15
        }

        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            Layout.topMargin: 10

            background: Rectangle {
                radius: 25
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#2A82FF" }
                    GradientStop { position: 1.0; color: "#00D4FF" } // Синий градиент
                }
            }

            contentItem: Text {
                text: "ЗАРЕГИСТРИРОВАТЬСЯ"
                color: "white"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: {
                SslClient.registerUser(regLoginField.text, regPasswordField.text, regPasswordConfirmField.text);
            }
            Text {
                        id: errorText
                        text: SslClient.errorMessage
                        color: "red"
                        font.pixelSize: 14

                        // Исправление: вместо anchors используем Layout-свойства
                        Layout.alignment: Qt.AlignHCenter
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter // Центрирование самого текста внутри компонента

                        // Текст виден только тогда, когда он не пустой
                        visible: text !== ""

                        wrapMode: Text.WordWrap
                    }

        }

        Text {
            text: "Назад ко входу"
            color: "#8FA0C0"
            font.pixelSize: 14
            Layout.alignment: Qt.AlignHCenter

            MouseArea {
                anchors.fill: parent
                onClicked: stackView.pop() // Возвращаемся на предыдущий экран
            }
        }
    }
}