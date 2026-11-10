import QtQuick
import QtQuick.Controls.Basic
import Theme

// Использование: checked: <значение>; onToggled: <сохранить checked>.
AbstractButton {
    id: control

    checkable: true
    implicitWidth: 40
    implicitHeight: 22
    activeFocusOnTab: true
    hoverEnabled: true
    opacity: enabled ? 1 : 0.45

    background: Rectangle {
        radius: height / 2
        color: control.checked ? Theme.accent : Theme.pressed
        border.width: control.visualFocus ? 2 : (control.checked ? 0 : 1)
        border.color: control.visualFocus ? Theme.text : Theme.lineStrong
        Behavior on color { ColorAnimation { duration: Theme.animFast } }

        Rectangle {
            width: 16; height: 16; radius: 8
            y: 3
            x: control.checked ? parent.width - width - 3 : 3
            color: control.checked ? Theme.onAccent : Theme.textMuted
            Behavior on x {
                NumberAnimation { duration: Theme.animFast; easing.type: Easing.OutCubic }
            }
        }
    }
}
