import QtQuick
import Theme

// Клавиша: так показываются назначенные хоткеи.
Rectangle {
    id: cap

    property string text: ""
    property bool large: false

    implicitWidth: Math.max(implicitHeight, capLabel.implicitWidth + (large ? 20 : 12))
    implicitHeight: large ? 32 : 22
    radius: large ? 6 : 5
    color: Theme.raised
    border.width: 1
    border.color: Theme.lineStrong

    // Нижняя грань, чтобы клавиша выглядела объёмной.
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 1
        height: 2
        radius: 2
        color: Theme.line
    }

    Text {
        id: capLabel
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -1
        text: cap.text
        color: Theme.textMuted
        font.pixelSize: cap.large ? TypeScale.body : TypeScale.caption
        font.weight: Font.Medium
    }
}
