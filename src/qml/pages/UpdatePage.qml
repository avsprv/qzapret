import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: page
    title: qsTr("Обновления")

    Kirigami.Dialog {
        id: confirmDialog
        property bool installing: false
        title: installing ? qsTr("Чистая установка zapret2") : qsTr("Обновление zapret2")
        standardButtons: Controls.Dialog.Yes | Controls.Dialog.No

        Controls.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: confirmDialog.installing
                  ? qsTr("Установить zapret2 %1 в /opt/zapret2? Будет проверена структура архива и SHA-256 бинарных компонентов, создана конфигурация по умолчанию с NFQWS2_ENABLE=0 и установлен systemd unit. Служба не будет запущена или добавлена в автозагрузку. Потребуется пароль администратора.")
                    .arg(updateController.latestVersion)
                  : qsTr("Обновить zapret2 с %1 до %2? Если служба запущена, она будет остановлена на время обновления и запущена снова. Потребуется пароль администратора.")
                    .arg(updateController.installedVersion).arg(updateController.latestVersion)
        }

        onAccepted: {
            if (installing)
                updateController.install()
            else
                updateController.update()
        }
    }

    ColumnLayout {
        width: page.availableWidth
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: updateController.status.length > 0
            text: updateController.status
            type: updateController.busy || updateController.checking
                  ? Kirigami.MessageType.Information
                  : (updateController.canInstall || updateController.canUpdate
                     ? Kirigami.MessageType.Positive : Kirigami.MessageType.Information)
        }

        Controls.GroupBox {
            Layout.fillWidth: true
            title: qsTr("Версия zapret2")

            GridLayout {
                anchors.fill: parent
                columns: 2

                Controls.Label { text: qsTr("Установлена:") }
                Controls.Label { text: updateController.installedVersion }

                Controls.Label { text: qsTr("Последняя версия:") }
                Controls.Label { text: updateController.latestVersion }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Controls.Button {
                text: qsTr("Проверить обновления")
                icon.name: "view-refresh"
                enabled: !updateController.busy && !updateController.checking
                onClicked: updateController.checkNow()
            }

            Controls.Button {
                text: updateController.installed
                      ? qsTr("Обновить до %1").arg(updateController.latestVersion)
                      : qsTr("Установить %1").arg(updateController.latestVersion)
                icon.name: "system-software-update"
                enabled: updateController.canInstall || updateController.canUpdate
                onClicked: {
                    confirmDialog.installing = updateController.canInstall
                    confirmDialog.open()
                }
            }

            Controls.Button {
                text: qsTr("Что нового…")
                icon.name: "document-open"
                enabled: updateController.releaseNotesUrl.length > 0
                onClicked: Qt.openUrlExternally(updateController.releaseNotesUrl)
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: Kirigami.MessageType.Information
            text: qsTr("Загрузка, проверка контрольных сумм, резервное копирование и откат выполняются привилегированным помощником. Подписи upstream-релиза отсутствуют: SHA-256 защищает от повреждения файлов, но не от компрометации репозитория.")
        }

        RowLayout {
            Layout.fillWidth: true
            Kirigami.Heading { level: 2; text: qsTr("Журнал обновления") }
            Item { Layout.fillWidth: true }
            Controls.Button {
                text: qsTr("Очистить")
                icon.name: "edit-clear"
                enabled: updateController.logText.length > 0 && !updateController.busy
                onClicked: updateController.clearLog()
            }
        }

        Controls.TextArea {
            Layout.fillWidth: true
            Layout.preferredHeight: 280
            readOnly: true
            text: updateController.logText
            wrapMode: TextEdit.NoWrap
            font.family: "monospace"
            Accessible.name: qsTr("Журнал обновления")
        }
    }
}
