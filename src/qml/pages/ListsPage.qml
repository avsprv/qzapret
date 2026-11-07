import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: page

    title: qsTr("Списки")

    property int selectedIndex: 0
    property int pendingIndex: -1

    readonly property var listEntries: [
        { label: qsTr("Исходный включающий хостлист"), file: "zapret-hosts-user.txt" },
        { label: qsTr("Исходный исключающий хостлист"), file: "zapret-hosts-user-exclude.txt" },
        { label: qsTr("Исходный IP-ban хостлист"), file: "zapret-hosts-user-ipban.txt" },
        { label: qsTr("Результирующий IPv4"), file: "zapret-ip-user.txt" },
        { label: qsTr("Результирующий IPv6"), file: "zapret-ip-user6.txt" },
        { label: qsTr("Результирующий IPv4 exclude"), file: "zapret-ip-exclude.txt" },
        { label: qsTr("Результирующий IPv6 exclude"), file: "zapret-ip-exclude6.txt" },
        { label: qsTr("Результирующий IP-ban IPv4"), file: "zapret-ip-user-ipban.txt" },
        { label: qsTr("Результирующий IP-ban IPv6"), file: "zapret-ip-user-ipban6.txt" }
    ]

    Component.onCompleted: listsController.readList(listEntries[selectedIndex].file)

    ColumnLayout {
        width: parent.width
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Information
            text: qsTr("Редактировать можно только три исходных пользовательских хостлиста. Производные IP-списки доступны только для чтения и формируются zapret2. Сохранение создаёт резервную копию и атомарно заменяет файл. Служба автоматически не перезапускается.")
        }

        RowLayout {
            Layout.fillWidth: true

            Controls.ComboBox {
                id: listSelector
                Layout.fillWidth: true
                model: page.listEntries.map(entry => entry.label)
                currentIndex: page.selectedIndex
                enabled: !listsController.busy
                Accessible.name: qsTr("Выбранный список")
                onActivated: index => {
                    if (index === page.selectedIndex)
                        return
                    if (listsController.dirty) {
                        page.pendingIndex = index
                        switchDialog.open()
                    } else {
                        page.selectedIndex = index
                        listSelector.currentIndex = index
                        listsController.readList(page.listEntries[index].file)
                    }
                }
            }

            Controls.Button {
                text: qsTr("Прочитать")
                icon.name: "view-refresh"
                enabled: !listsController.busy
                onClicked: {
                    if (listsController.dirty) {
                        page.pendingIndex = page.selectedIndex
                        switchDialog.open()
                    } else {
                        listsController.readList(page.listEntries[page.selectedIndex].file)
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Controls.Label {
                Layout.fillWidth: true
                text: listsController.loaded
                      ? (listsController.editable ? qsTr("Исходный список · редактирование доступно")
                                                  : qsTr("Производный список · только чтение"))
                      : qsTr("Список не загружен")
                wrapMode: Text.WordWrap
            }

            Controls.Button {
                text: qsTr("Сохранить")
                icon.name: "document-save"
                enabled: listsController.loaded && listsController.editable &&
                         listsController.dirty && !listsController.busy
                onClicked: saveDialog.open()
            }
        }

        Controls.TextArea {
            id: editor
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 300
            text: listsController.content
            readOnly: !listsController.loaded || !listsController.editable || listsController.busy
            wrapMode: TextEdit.NoWrap
            selectByMouse: true
            font.family: "monospace"
            Accessible.name: qsTr("Содержимое пользовательского списка")
            Accessible.description: qsTr("Одна запись на строку. Допускаются домены ASCII или punycode, IP-адреса, CIDR и комментарии.")
            onTextChanged: listsController.setContent(text)
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: listsController.status.length > 0
            type: listsController.status.startsWith(qsTr("Не удалось")) ||
                  listsController.status.startsWith(qsTr("Некоррект")) ||
                  listsController.status.startsWith(qsTr("Файл превышает")) ||
                  listsController.status.startsWith(qsTr("Слишком")) ||
                  listsController.status.startsWith(qsTr("Строка"))
                  ? Kirigami.MessageType.Error : Kirigami.MessageType.Information
            text: listsController.status
        }
    }

    Controls.Dialog {
        id: switchDialog
        modal: true
        title: qsTr("Несохранённые изменения")
        standardButtons: Controls.Dialog.Yes | Controls.Dialog.No
        Controls.Label {
            width: parent.width
            wrapMode: Text.WordWrap
            text: page.pendingIndex === page.selectedIndex
                  ? qsTr("Текущий хостлист изменён. Отбросить изменения и перечитать его?")
                  : qsTr("Текущий хостлист изменён. Отбросить изменения и переключиться на другой список?")
        }
        onAccepted: {
            page.selectedIndex = page.pendingIndex
            listSelector.currentIndex = page.pendingIndex
            page.pendingIndex = -1
            listsController.readList(page.listEntries[page.selectedIndex].file)
        }
        onRejected: {
            page.pendingIndex = -1
            listSelector.currentIndex = page.selectedIndex
        }
    }

    Controls.Dialog {
        id: saveDialog
        modal: true
        title: qsTr("Сохранить хостлист")
        standardButtons: Controls.Dialog.Save | Controls.Dialog.Cancel
        Controls.Label {
            width: parent.width
            wrapMode: Text.WordWrap
            text: qsTr("Создать резервную копию и атомарно сохранить %1? Производные IP-списки будут обновлены штатными механизмами zapret2. Служба автоматически не перезапускается.")
                    .arg(listsController.loadedName)
        }
        onAccepted: listsController.saveList()
    }
}
