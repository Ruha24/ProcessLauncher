import QtQuick
import QtQuick.Controls.Basic
import Theme

Button {
    id: control

    property string variant: "secondary"
    property int minWidth: 96
    property color accent: variant === "primary" ? Theme.interactive
                         : variant === "danger"  ? Theme.danger
                         : Theme.surfaceElevated

    implicitHeight: 38
    implicitWidth: Math.max(minWidth, implicitContentWidth + 2 * Theme.spacing)
    padding: 0
    font.pixelSize: TypeScale.base

    activeFocusOnTab: true
    hoverEnabled: true

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.variant === "secondary" ? Theme.textPrimary
                                                : Theme.textOnAccent
        opacity: control.enabled ? 1.0 : 0.4
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        radius: Theme.radiusSmall
        color: {
            if (!control.enabled)
                return Qt.darker(control.accent, 1.4)
            if (control.down)
                return Qt.darker(control.accent, 1.15)
            if (control.hovered)
                return control.variant === "secondary" ? Theme.surfaceHover
                                                        : Qt.lighter(control.accent, 1.12)
            return control.accent
        }
        border.width: control.variant === "secondary" ? 1 : 0
        border.color: Theme.outline

        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: parent.radius + 3
            color: "transparent"
            border.width: 2
            border.color: Theme.interactive
            visible: control.activeFocus
        }

        Behavior on color {
            ColorAnimation { duration: Theme.animFast }
        }
    }

    scale: down ? 0.97 : 1.0
    Behavior on scale {
        NumberAnimation { duration: Theme.animFast; easing.type: Easing.OutCubic }
    }
}
