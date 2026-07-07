import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Theme

ApplicationWindow {
    id: window
    visible: true
    width: 520
    height: 560
    minimumWidth: 420
    minimumHeight: 360
    title: qsTr("Process Launcher")
    color: Theme.surface

    onClosing: (close) => {
        if (tray.isAvailable()) {
            close.accepted = false
            tray.hideWindow()
        }
    }

    Connections {
        target: processModel
        function onErrorMessage(text) {
            errorBanner.text = text
            errorBanner.show()
        }
    }

    Rectangle {
        id: errorBanner
        property string text: ""
        function show() { visible = true; opacity = 1; hideTimer.restart() }

        z: 100
        visible: false
        opacity: 0
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.spacing
        implicitHeight: 44
        radius: Theme.radiusSmall
        color: Theme.dangerMuted
        border.width: 1
        border.color: Theme.danger

        Behavior on opacity { NumberAnimation { duration: Theme.animBase } }
        onOpacityChanged: if (opacity === 0) visible = false

        Timer {
            id: hideTimer
            interval: 4000
            onTriggered: errorBanner.opacity = 0
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.spacing
            anchors.rightMargin: Theme.spacingS
            spacing: Theme.spacingS
            Text {
                Layout.fillWidth: true
                text: errorBanner.text
                color: Theme.danger
                font.pixelSize: TypeScale.caption
                elide: Text.ElideRight
                wrapMode: Text.WordWrap
                maximumLineCount: 2
            }
            AppButton {
                text: "✕"
                variant: "secondary"
                Layout.preferredWidth: 32
                implicitHeight: 30
                onClicked: errorBanner.opacity = 0
            }
        }
    }

    header: Rectangle {
        implicitHeight: 112
        color: Theme.surface

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.spacingL
            anchors.rightMargin: Theme.spacingL
            anchors.topMargin: Theme.spacingS
            anchors.bottomMargin: Theme.spacingS
            spacing: Theme.spacingS

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacing

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Text {
                        text: qsTr("Programs")
                        color: Theme.textPrimary
                        font.pixelSize: TypeScale.h1
                        font.weight: Font.Medium
                    }
                    Text {
                        text: qsTr("%n item(s)", "", list.count)
                        color: Theme.textMuted
                        font.pixelSize: TypeScale.caption
                    }
                }

                AppButton {
                    text: qsTr("Stop all")
                    variant: "secondary"
                    onClicked: processModel.stopAll()
                }
                AppButton {
                    text: qsTr("+ Add")
                    variant: "primary"
                    onClicked: fileDialog.open()
                }
            }

            TextField {
                id: searchField
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                placeholderText: qsTr("Search programs…")
                color: Theme.textPrimary
                placeholderTextColor: Theme.textMuted
                font.pixelSize: TypeScale.base
                selectByMouse: true
                onTextChanged: filteredModel.setSearchText(text)
                background: Rectangle {
                    radius: Theme.radiusSmall
                    color: Theme.surfaceElevated
                    border.width: 1
                    border.color: searchField.activeFocus ? Theme.interactive
                                                          : Theme.outline
                    Behavior on border.color {
                        ColorAnimation { duration: Theme.animFast }
                    }
                }
            }
        }
    }

    ListView {
        id: list
        anchors.fill: parent
        anchors.topMargin: Theme.spacingS
        clip: true
        model: filteredModel
        spacing: 0
        reuseItems: true

        delegate: ProcessRow {
            onStartRequested: (id) => processModel.start(id)
            onStopRequested: (id) => processModel.stop(id)
            onRemoveRequested: (id) => processModel.removeProgram(id)
            onEditBindRequested: (id, currentBind) => {
                bindDialog.targetId = id
                bindDialog.captured = currentBind
                bindDialog.open()
            }
            onEditArgsRequested: (id, currentArgs) => {
                argsDialog.targetId = id
                argsField.text = currentArgs
                argsDialog.open()
            }
        }

        Item {
            anchors.centerIn: parent
            width: parent.width - 2 * Theme.spacingL
            visible: list.count === 0

            ColumnLayout {
                anchors.centerIn: parent
                spacing: Theme.spacing
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("No programs yet")
                    color: Theme.textPrimary
                    font.pixelSize: TypeScale.h2
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.maximumWidth: 320
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: qsTr("Add an executable to launch and stop it with one click.")
                    color: Theme.textMuted
                    font.pixelSize: TypeScale.base
                }
                AppButton {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("+ Add program")
                    variant: "primary"
                    onClicked: fileDialog.open()
                }
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Choose a program")
        nameFilters: Qt.platform.os === "windows"
                     ? [qsTr("Executables (*.exe)"), qsTr("All files (*)")]
                     : [qsTr("All files (*)")]
        onAccepted: processModel.addProgramFromUrl(selectedFile, "")
    }

    Dialog {
        id: bindDialog
        property string targetId: ""
        property string captured: ""

        title: qsTr("Set hotkey")
        anchors.centerIn: parent
        width: 360
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel | Dialog.Reset

        onOpened: captureArea.forceActiveFocus()
        onAccepted: processModel.setBind(targetId, captured)
        onReset: {
            captured = ""
            processModel.setBind(targetId, "")
        }

        function isModifierKey(k) {
            return k === Qt.Key_Control || k === Qt.Key_Shift
                || k === Qt.Key_Alt || k === Qt.Key_Meta
                || k === Qt.Key_AltGr
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
            if (mods & Qt.ControlModifier) parts.push("Ctrl")
            if (mods & Qt.AltModifier)     parts.push("Alt")
            if (mods & Qt.ShiftModifier)   parts.push("Shift")
            if (mods & Qt.MetaModifier)    parts.push("Win")
            parts.push(name)
            return parts.join("+")
        }

        background: Rectangle {
            color: Theme.surfaceElevated
            radius: Theme.radius
            border.width: 1
            border.color: Theme.outline
        }

        contentItem: ColumnLayout {
            spacing: Theme.spacingS
            Text {
                text: qsTr("Press a key or combination to launch this program.")
                color: Theme.textMuted
                font.pixelSize: TypeScale.caption
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            Rectangle {
                id: captureArea
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                radius: Theme.radiusSmall
                color: Theme.surface
                border.width: 1
                border.color: activeFocus ? Theme.interactive : Theme.outline
                focus: true
                activeFocusOnTab: true
                Keys.enabled: true

                Behavior on border.color {
                    ColorAnimation { duration: Theme.animFast }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: captureArea.forceActiveFocus()
                }

                Keys.onShortcutOverride: (event) => {
                    event.accepted = true
                }

                Keys.onPressed: (event) => {
                    if (bindDialog.isModifierKey(event.key)) {
                        event.accepted = true
                        return
                    }
                    var s = bindDialog.toBindString(event.key, event.modifiers)
                    if (s.length > 0)
                        bindDialog.captured = s
                    event.accepted = true
                }

                Text {
                    anchors.centerIn: parent
                    text: bindDialog.captured.length > 0
                          ? bindDialog.captured
                          : qsTr("Press keys…")
                    color: bindDialog.captured.length > 0
                           ? Theme.textPrimary : Theme.textMuted
                    font.pixelSize: TypeScale.h2
                }
            }

            Text {
                text: qsTr("Allowed: letters, digits, F1–F24, alone or with Ctrl/Alt/Shift.")
                color: Theme.textMuted
                font.pixelSize: TypeScale.caption
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
        }
    }

    Dialog {
        id: argsDialog
        property string targetId: ""

        title: qsTr("Launch arguments")
        anchors.centerIn: parent
        width: 380
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        onOpened: argsField.forceActiveFocus()
        onAccepted: processModel.setArgs(targetId, argsField.text)

        background: Rectangle {
            color: Theme.surfaceElevated
            radius: Theme.radius
            border.width: 1
            border.color: Theme.outline
        }

        contentItem: ColumnLayout {
            spacing: Theme.spacingS
            Text {
                text: qsTr("Command-line arguments passed to the program.")
                color: Theme.textMuted
                font.pixelSize: TypeScale.caption
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
            TextField {
                id: argsField
                Layout.fillWidth: true
                placeholderText: qsTr("e.g. --fullscreen --profile dev")
                color: Theme.textPrimary
                placeholderTextColor: Theme.textMuted
                font.pixelSize: TypeScale.base
                selectByMouse: true
                onAccepted: argsDialog.accept()
                background: Rectangle {
                    radius: Theme.radiusSmall
                    color: Theme.surface
                    border.width: 1
                    border.color: argsField.activeFocus ? Theme.interactive
                                                        : Theme.outline
                    Behavior on border.color {
                        ColorAnimation { duration: Theme.animFast }
                    }
                }
            }
        }
    }
}
