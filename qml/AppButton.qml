import QtQuick
import QtQuick.Controls.Basic
import Theme

// variant: "primary" | "secondary" | "ghost" | "danger"
Button {
    id: control

    property string variant: "secondary"
    property int minWidth: 0

    implicitHeight: Theme.controlHeight
    implicitWidth: Math.max(minWidth, label.implicitWidth + 2 * Theme.spaceL)
    padding: 0
    font.pixelSize: TypeScale.body
    hoverEnabled: true
    activeFocusOnTab: true
    opacity: enabled ? 1 : 0.45

    contentItem: Text {
        id: label
        text: control.text
        font: control.font
        color: control.variant === "primary" ? Theme.onAccent
             : control.variant === "danger"  ? Theme.danger
             : Theme.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: Theme.radiusSmall
        color: {
            if (control.variant === "primary")
                return control.down ? Theme.accent
                     : control.hovered ? Theme.accentHover : Theme.accent
            if (control.variant === "danger")
                return control.down || control.hovered ? Theme.dangerSoft : "transparent"
            if (control.variant === "ghost")
                return control.down ? Theme.pressed
                     : control.hovered ? Theme.hover : "transparent"
            return control.down ? Theme.pressed
                 : control.hovered ? Theme.hover : Theme.raised
        }
        border.width: control.visualFocus ? 2
                    : (control.variant === "secondary" || control.variant === "danger") ? 1 : 0
        border.color: control.visualFocus ? Theme.accent : Theme.line

        Behavior on color { ColorAnimation { duration: Theme.animFast } }
    }
}
