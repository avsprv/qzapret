import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: page

    title: qsTr("О программе")

    ColumnLayout {
        width: parent.width
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 1
            text: qsTr("zapret2applet")
        }

        Kirigami.UrlButton {
            Layout.fillWidth: true
            text: qsTr("Репозиторий zapret2applet")
            url: "https://github.com/avsprv/zapret2applet"
        }

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 2
            text: qsTr("Версия и компоненты")
        }

        Kirigami.UrlButton {
            Layout.fillWidth: true
            text: qsTr("Документация приложения")
            url: "https://github.com/avsprv/zapret2applet/blob/main/README.md"
        }

        Kirigami.UrlButton {
            Layout.fillWidth: true
            text: qsTr("Релизы приложения")
            url: "https://github.com/avsprv/zapret2applet/releases"
        }

        Kirigami.UrlButton {
            Layout.fillWidth: true
            text: qsTr("Сообщить об ошибке")
            url: "https://github.com/avsprv/zapret2applet/issues"
        }

        Kirigami.UrlButton {
            Layout.fillWidth: true
            text: qsTr("Уведомления о сторонних компонентах")
            url: "https://github.com/avsprv/zapret2applet/blob/main/NOTICE.md"
        }

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 2
            text: qsTr("Лицензии")
        }

        Kirigami.UrlButton {
            Layout.fillWidth: true
            text: qsTr("Upstream-проект zapret2")
            url: "https://github.com/bol-van/zapret2"
        }

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 2
            text: qsTr("Проект и документация")
        }

        Controls.Label {
            Layout.fillWidth: true
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            Accessible.name: qsTr("Ссылки на проект и документацию")
            text: qsTr("<a href=\"https://github.com/avsprv/zapret2applet\">Репозиторий zapret2applet</a>")
            onLinkActivated: Qt.openUrlExternally(link)
        }

        Controls.Label {
            Layout.fillWidth: true
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            text: qsTr("<a href=\"https://github.com/avsprv/zapret2applet/blob/main/README.md\">Документация приложения</a>")
            onLinkActivated: Qt.openUrlExternally(link)
        }

        Controls.Label {
            Layout.fillWidth: true
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            text: qsTr("<a href=\"https://github.com/avsprv/zapret2applet/releases\">Релизы приложения</a>")
            onLinkActivated: Qt.openUrlExternally(link)
        }

        Controls.Label {
            Layout.fillWidth: true
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            text: qsTr("<a href=\"https://github.com/avsprv/zapret2applet/issues\">Сообщить об ошибке</a>")
            onLinkActivated: Qt.openUrlExternally(link)
        }

        Controls.Label {
            Layout.fillWidth: true
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            text: qsTr("<a href=\"https://github.com/avsprv/zapret2applet/blob/main/NOTICE.md\">Уведомления о сторонних компонентах</a>")
            onLinkActivated: Qt.openUrlExternally(link)
        }

        Controls.Label {
            Layout.fillWidth: true
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            text: qsTr("<a href=\"https://github.com/bol-van/zapret2\">Upstream-проект zapret2</a>")
            onLinkActivated: Qt.openUrlExternally(link)
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Information
            text: qsTr("zapret2applet — независимый проект и не связан с автором или сопровождающими upstream-проекта zapret2.")
        }
    }
}
