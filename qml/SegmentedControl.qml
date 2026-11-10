import QtQuick
import QtQuick.Controls.Basic
import Theme

// options: [{ label: "...", value: "..." }, ...]
Rectangle {
    id: control

    property var options: []
    property var value
    signal picked(var value)

    implicitHeight: Theme.controlHeight
    implicitWidth: segments.implicitWidth + 4
    radius: Theme.radiusSmall
    color: Theme.raised
    border.width: 1
    border.color: Theme.line

    Row {
        id: segments
        anchors.centerIn: parent
        spacing: 0

        Repeater {
            model: control.options

            AbstractButton {
                id: segment
                required property var modelData
                readonly property bool current: control.value === modelData.value

                implicitHeight: control.height - 4
                implicitWidth: segmentLabel.implicitWidth + 2 * Theme.spaceM
                hoverEnabled: true
                activeFocusOnTab: true
                onClicked: control.picked(modelData.value)

                contentItem: Text {
                    id: segmentLabel
                    text: segment.modelData.label
                    color: segment.current ? Theme.text : Theme.textMuted
                    font.pixelSize: TypeScale.body
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 4
                    color: segment.current ? Theme.pressed
                         : segment.hovered ? Theme.hover : "transparent"
                    border.width: segment.visualFocus ? 2 : 0
                    border.color: Theme.accent
                }
            }
        }
    }
}
