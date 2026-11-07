import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    title: qsTr("Настройки")

    ColumnLayout {
        width: parent.width
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 1
            text: qsTr("Внешний вид")
        }

        Controls.Label {
            text: qsTr("Тема оформления")
        }

        Controls.ComboBox {
            Layout.fillWidth: true
            model: [
                { text: qsTr("Системная"), value: "system" },
                { text: qsTr("Светлая"), value: "light" },
                { text: qsTr("Тёмная"), value: "dark" }
            ]
            textRole: "text"
            currentIndex: {
                const index = model.findIndex(item => item.value === settingsManager.theme)
                return Math.max(0, index)
            }
            onActivated: settingsManager.theme = model[currentIndex].value
        }

        Controls.Label {
            text: qsTr("Язык интерфейса")
        }

        Controls.ComboBox {
            Layout.fillWidth: true
            model: [
                { text: qsTr("Системный"), value: "system" },
                { text: qsTr("Русский"), value: "ru" },
                { text: qsTr("English"), value: "en" }
            ]
            textRole: "text"
            currentIndex: {
                const index = model.findIndex(item => item.value === settingsManager.language)
                return Math.max(0, index)
            }
            onActivated: settingsManager.language = model[currentIndex].value
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 2
            text: qsTr("Обновления")
        }

        Controls.CheckBox {
            Layout.fillWidth: true
            text: qsTr("Автоматически проверять обновления")
            checked: settingsManager.autoCheckUpdates
            onToggled: settingsManager.autoCheckUpdates = checked
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 2
            text: qsTr("Поведение")
        }

        Controls.CheckBox {
            Layout.fillWidth: true
            text: qsTr("Сворачивать приложение в системный трей при закрытии окна")
            checked: settingsManager.closeToTray
            onToggled: settingsManager.closeToTray = checked
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Information
            text: qsTr("Настройки сохраняются в прежние ключи QSettings. Изменение языка будет применяться после реализации перезапуска интерфейса.")
        }
    }
}
