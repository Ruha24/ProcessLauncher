import QtQuick
import QtQuick.Controls.Basic
import Theme

TextField {
    id: field

    implicitHeight: Theme.controlHeight
    leftPadding: 10
    rightPadding: 10
    color: Theme.text
    placeholderTextColor: Theme.textFaint
    selectionColor: Theme.accent
    selectedTextColor: Theme.onAccent
    font.pixelSize: TypeScale.body
    selectByMouse: true
    verticalAlignment: TextInput.AlignVCenter

    background: Rectangle {
        radius: Theme.radiusSmall
        color: Theme.raised
        border.width: field.activeFocus ? 2 : 1
        border.color: field.activeFocus ? Theme.accent : Theme.line
    }
}
