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
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Тема пока следует системным настройкам. Выбор светлой и тёмной темы будет добавлен позже.")
            Accessible.name: qsTr("Тема оформления")
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
            Accessible.name: qsTr("Язык интерфейса")
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
            text: qsTr("Параметры сохраняются в прежние ключи QSettings. Переключение языка применяется сразу; принудительные светлая и тёмная схемы будут подключены на следующем этапе.")
        }
    }
}
