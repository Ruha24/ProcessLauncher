import QtQuick
import QtQuick.Controls.Basic
import Theme

// Квадратная кнопка без подписи. kind: "more" (три точки) | "close" | "plus".
// Значки рисуются фигурами, чтобы не зависеть от шрифтов системы.
AbstractButton {
    id: control

    property string kind: "more"
    property string hint: ""
    property bool danger: false

    implicitWidth: Theme.controlHeight
    implicitHeight: Theme.controlHeight
    hoverEnabled: true
    activeFocusOnTab: true
    opacity: enabled ? 1 : 0.4

    readonly property color ink: danger && hovered ? Theme.danger
                               : hovered ? Theme.text : Theme.textMuted

    background: Rectangle {
        radius: Theme.radiusSmall
        color: control.down ? Theme.pressed
             : control.hovered ? (control.danger ? Theme.dangerSoft : Theme.hover)
             : "transparent"
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.accent
    }

    contentItem: Item {
        Row {
            anchors.centerIn: parent
            spacing: 3
            visible: control.kind === "more"
            Repeater {
                model: 3
                Rectangle { width: 3; height: 3; radius: 1.5; color: control.ink }
            }
        }
        Item {
            anchors.centerIn: parent
            width: 12; height: 12
            visible: control.kind === "close" || control.kind === "plus"
            rotation: control.kind === "close" ? 45 : 0
            Rectangle {
                anchors.centerIn: parent
                width: 12; height: 1.5; radius: 1; color: control.ink
            }
            Rectangle {
                anchors.centerIn: parent
                width: 1.5; height: 12; radius: 1; color: control.ink
            }
        }
    }

    ToolTip.visible: hint.length > 0 && hovered
    ToolTip.delay: 600
    ToolTip.text: hint
}
