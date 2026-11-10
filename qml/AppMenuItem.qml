import QtQuick
import QtQuick.Controls.Basic
import Theme

MenuItem {
    id: item

    property bool danger: false
    property string trailingText: ""

    implicitHeight: 30
    padding: 0
    font.pixelSize: TypeScale.body

    indicator: null
    arrow: null

    contentItem: Item {
        Text {
            id: itemLabel
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.right: trailing.left
            anchors.rightMargin: Theme.spaceS
            anchors.verticalCenter: parent.verticalCenter
            text: item.text
            font: item.font
            color: item.danger ? Theme.danger : Theme.text
            opacity: item.enabled ? 1 : 0.4
            elide: Text.ElideRight
        }
        Text {
            id: trailing
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            // Галочка у включённых пунктов, иначе — подсказка (например, хоткей).
            text: item.checkable ? (item.checked ? "✓" : "") : item.trailingText
            font.pixelSize: TypeScale.caption
            color: item.checkable ? Theme.accent : Theme.textFaint
        }
    }

    background: Rectangle {
        radius: 5
        color: item.highlighted && item.enabled
               ? (item.danger ? Theme.dangerSoft : Theme.hover) : "transparent"
    }
}
