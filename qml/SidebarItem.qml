import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Theme

AbstractButton {
    id: item

    property bool selected: false
    // Для профилей: значок-эмодзи либо цветная точка.
    property bool showMarker: false
    property string emoji: ""
    property color markerColor: Theme.lineStrong
    // Счётчик справа; active подсвечивает его зелёным («что-то запущено»).
    property string badge: ""
    property bool badgeActive: false

    signal contextRequested()

    implicitHeight: 34
    padding: 0
    hoverEnabled: true
    activeFocusOnTab: true

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: item.contextRequested()
    }

    background: Rectangle {
        radius: Theme.radiusSmall
        color: item.selected ? Theme.pressed
             : item.hovered ? Theme.hover : "transparent"
        border.width: item.visualFocus ? 2 : 0
        border.color: Theme.accent
    }

    contentItem: RowLayout {
        spacing: Theme.spaceS

        Item {
            Layout.leftMargin: Theme.spaceM
            Layout.preferredWidth: 18
            Layout.preferredHeight: 18
            visible: item.showMarker

            Text {
                anchors.centerIn: parent
                visible: item.emoji.length > 0
                text: item.emoji
                font.pixelSize: 14
            }
            Rectangle {
                anchors.centerIn: parent
                visible: item.emoji.length === 0
                width: 8; height: 8; radius: 4
                color: item.markerColor
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.leftMargin: item.showMarker ? 0 : Theme.spaceM
            text: item.text
            color: item.selected ? Theme.text : Theme.textMuted
            font.pixelSize: TypeScale.body
            font.weight: item.selected ? Font.DemiBold : Font.Normal
            elide: Text.ElideRight
        }

        Text {
            Layout.rightMargin: Theme.spaceM
            visible: item.badge.length > 0
            text: item.badge
            color: item.badgeActive ? Theme.running : Theme.textFaint
            font.pixelSize: TypeScale.caption
            font.weight: item.badgeActive ? Font.DemiBold : Font.Normal
        }
    }
}
