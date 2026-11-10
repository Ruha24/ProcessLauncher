import QtQuick
import QtQuick.Layouts
import Theme

// Строка настройки: название и пояснение слева, элемент управления справа.
RowLayout {
    id: row

    property string title: ""
    property string description: ""
    default property alias control: controlSlot.data

    spacing: Theme.spaceL

    ColumnLayout {
        Layout.fillWidth: true
        Layout.topMargin: Theme.spaceM
        Layout.bottomMargin: Theme.spaceM
        spacing: 2

        Text {
            Layout.fillWidth: true
            text: row.title
            color: Theme.text
            font.pixelSize: TypeScale.body
            wrapMode: Text.WordWrap
        }
        Text {
            Layout.fillWidth: true
            visible: row.description.length > 0
            text: row.description
            color: Theme.textMuted
            font.pixelSize: TypeScale.caption
            wrapMode: Text.WordWrap
        }
    }

    RowLayout {
        id: controlSlot
        Layout.alignment: Qt.AlignVCenter
        spacing: Theme.spaceS
    }
}
