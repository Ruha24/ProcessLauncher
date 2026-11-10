import QtQuick
import QtQuick.Layouts
import Theme

// Шапка страницы: заголовок, пояснение и действия справа (дочерние элементы).
RowLayout {
    id: header

    property string title: ""
    property string subtitle: ""
    property string emoji: ""
    property color subtitleColor: Theme.textMuted
    default property alias actions: actionsRow.data

    spacing: Theme.spaceM

    Text {
        visible: header.emoji.length > 0
        text: header.emoji
        font.pixelSize: 26
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2

        Text {
            Layout.fillWidth: true
            text: header.title
            color: Theme.text
            font.pixelSize: TypeScale.heading
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }
        Text {
            Layout.fillWidth: true
            visible: header.subtitle.length > 0
            text: header.subtitle
            color: header.subtitleColor
            font.pixelSize: TypeScale.body
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }
    }

    RowLayout {
        id: actionsRow
        Layout.alignment: Qt.AlignTop
        spacing: Theme.spaceS
    }
}
