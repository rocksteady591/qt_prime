import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import qt_prime

Item {
    width: parent.width
    height: parent.height

    Connections{
        target: SslClient
        function onLoginSuccess(){
            stackView.push("ChatsScreen.qml")
        }
    }

    ColumnLayout{
            anchors.centerIn: parent
            width: parent.width * 0.8
            spacing: 25
            //Текстовый элемент для заголовка приложения
            Text {
                text: "Prime"                 // Сам текст
                color: "white"                 // Цвет текста
                font.pixelSize: 42             // Размер шрифта в пикселях
                font.bold: true                // Жирное начертание

                // Выравнивание этого конкретного текста по центру колонки по горизонтали
                Layout.alignment: Qt.AlignHCenter

                // Дополнительный отступ снизу от заголовка до следующего элемента
                Layout.bottomMargin: 20
            }
            TextField{
                id: loginField
                Layout.fillWidth: true
                placeholderText: "Email, логин или номер телефона"
                color: "white"
                placeholderTextColor: "#8FA0C0"
                background: Rectangle{
                    color: "#1C2333"
                    radius: 15
                    implicitHeight: 50
                }
                leftPadding: 15
            }
            TextField{
                id: passwordField
                Layout.fillWidth: true
                placeholderText: "Пароль"
                echoMode: TextInput.Password // скрывает вводимый текст
                color: "white"
                placeholderTextColor: "#8FA0C0"
                background: Rectangle {
                    color: "#1C2333"
                    radius: 15
                    implicitHeight: 50
                }
                leftPadding: 15
            }
            Button{
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                Layout.topMargin: 10

                background: Rectangle{
                    radius: 25
                    gradient: Gradient {
                        orientation: Gradient.Horizontal // Градиент идет слева направо
                        GradientStop { position: 0.0; color: "#FF6B6B" } // Начальный цвет (розово-красный)
                        GradientStop { position: 1.0; color: "#FF8E53" } // Конечный цвет (оранжевый)
                    }

                }
                contentItem: Text{
                    text: "ВОЙТИ"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter // Выравнивание текста по центру по горизонтали
                    verticalAlignment: Text.AlignVCenter   // Выравнивание текста по центру по вертикали
                }
                onClicked: {
                    SslClient.login(loginField.text, passwordField.text);
                }
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
            Text {
                text: "Нет аккаунта? Создать"
                color: "#8FA0C0"
                font.pixelSize: 14
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 10

                // Невидимая зона поверх текста, которая ловит клики мыши или касания экрана
                MouseArea {
                    anchors.fill: parent       // Занимает всю площадь текста

                    onClicked: {
                        // Команда менеджеру экранов (StackView) открыть страницу регистрации поверх текущей
                        stackView.push("RegisterScreen.qml")
                    }
                }
            }
        }
}
