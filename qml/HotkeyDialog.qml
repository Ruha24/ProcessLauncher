import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Theme

// Один диалог назначения хоткея и для программ, и для профилей.
AppDialog {
    id: dialog

    property string description: ""
    property string captured: ""
    readonly property bool isMac: Qt.platform.os === "osx"

    signal saved(string bind)

    preferredWidth: 380
    acceptText: qsTr("Save")
    // Убирать нечего, пока хоткей не назначен.
    property bool hadBind: false
    clearText: hadBind ? qsTr("Remove hotkey") : ""
    onAboutToShow: hadBind = captured.length > 0

    onOpened: captureArea.forceActiveFocus()
    onAccepted: saved(captured)
    onCleared: {
        captured = ""
        saved("")
        close()
    }

    function isModifierKey(k) {
        return k === Qt.Key_Control || k === Qt.Key_Shift
            || k === Qt.Key_Alt || k === Qt.Key_Meta || k === Qt.Key_AltGr
    }

    function toBindString(key, mods) {
        var name = ""
        var shiftPairs = {
            "!":"1","@":"2","#":"3","$":"4","%":"5",
            "^":"6","&":"7","*":"8","(":"9",")":"0"
        }
        if (key >= Qt.Key_0 && key <= Qt.Key_9)
            name = String.fromCharCode("0".charCodeAt(0) + (key - Qt.Key_0))
        else if (key >= Qt.Key_A && key <= Qt.Key_Z)
            name = String.fromCharCode("A".charCodeAt(0) + (key - Qt.Key_A))
        else if (key >= Qt.Key_F1 && key <= Qt.Key_F24)
            name = "F" + (key - Qt.Key_F1 + 1)
        else if (shiftPairs[String.fromCharCode(key)] !== undefined)
            name = shiftPairs[String.fromCharCode(key)]
        else
            return ""

        var parts = []
        // На macOS Qt меняет их местами: ControlModifier — это клавиша Cmd,
        // MetaModifier — клавиша Control.
        if (dialog.isMac) {
            if (mods & Qt.MetaModifier)    parts.push("Ctrl")
            if (mods & Qt.AltModifier)     parts.push("Alt")
            if (mods & Qt.ShiftModifier)   parts.push("Shift")
            if (mods & Qt.ControlModifier) parts.push("Cmd")
        } else {
            if (mods & Qt.ControlModifier) parts.push("Ctrl")
            if (mods & Qt.AltModifier)     parts.push("Alt")
            if (mods & Qt.ShiftModifier)   parts.push("Shift")
            if (mods & Qt.MetaModifier)    parts.push("Win")
        }
        parts.push(name)
        return parts.join("+")
    }

    contentItem: ColumnLayout {
        spacing: Theme.spaceM

        Text {
            Layout.fillWidth: true
            text: dialog.description
            color: Theme.textMuted
            font.pixelSize: TypeScale.body
            wrapMode: Text.WordWrap
        }

        Rectangle {
            id: captureArea
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            radius: Theme.radius
            color: Theme.surface
            border.width: activeFocus ? 2 : 1
            border.color: activeFocus ? Theme.accent : Theme.line
            focus: true
            activeFocusOnTab: true

            MouseArea {
                anchors.fill: parent
                onClicked: captureArea.forceActiveFocus()
            }

            Keys.onShortcutOverride: (event) => { event.accepted = true }
            Keys.onPressed: (event) => {
                event.accepted = true
                if (dialog.isModifierKey(event.key))
                    return
                var s = dialog.toBindString(event.key, event.modifiers)
                if (s.length > 0)
                    dialog.captured = s
            }

            Row {
                anchors.centerIn: parent
                spacing: 6
                visible: dialog.captured.length > 0
                Repeater {
                    model: dialog.captured.length > 0 ? dialog.captured.split("+") : []
                    Keycap {
                        required property string modelData
                        text: modelData
                        large: true
                    }
                }
            }
            Text {
                anchors.centerIn: parent
                visible: dialog.captured.length === 0
                text: qsTr("Press a key or a combination")
                color: Theme.textFaint
                font.pixelSize: TypeScale.body
            }
        }

        Text {
            Layout.fillWidth: true
            text: dialog.isMac
                  ? qsTr("Letters, digits and F1–F20, alone or with Cmd, Ctrl, Option or Shift. The hotkey works even when the launcher is in the background.")
                  : qsTr("Letters, digits and F1–F24, alone or with Ctrl, Alt, Shift or Win. The hotkey works even when the launcher is in the background.")
            color: Theme.textFaint
            font.pixelSize: TypeScale.caption
            wrapMode: Text.WordWrap
        }
    }
}
