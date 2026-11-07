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

        Controls.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Независимый графический интерфейс на Qt 6 для управления zapret2 в Linux.")
        }

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 2
            text: qsTr("Версия и компоненты")
        }

        Controls.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Версия приложения: %1").arg(Qt.application.version)
        }

        Controls.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Qt %1 — графическая платформа приложения.").arg(Qt.version)
        }

        Controls.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("zapret2 — отдельный upstream-компонент; он не входит в состав приложения.")
        }

        Controls.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("systemd и Polkit используются для системной интеграции и авторизации операций.")
        }

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 2
            text: qsTr("Лицензии")
        }

        Controls.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Код zapret2applet распространяется по лицензии MIT. Лицензии сторонних компонентов сохраняют собственную область действия. Подробности приведены в NOTICE и в документации upstream.")
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
