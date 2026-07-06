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

    header: Rectangle {
        implicitHeight: 64
        color: Theme.surface

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.spacingL
            anchors.rightMargin: Theme.spacingL
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
    }

    ListView {
        id: list
        anchors.fill: parent
        anchors.topMargin: Theme.spacingS
        clip: true
        model: processModel
        spacing: 0
        reuseItems: true

        delegate: ProcessRow {
            onStartRequested: (id) => processModel.start(id)
            onStopRequested: (id) => processModel.stop(id)
            onRemoveRequested: (id) => processModel.removeProgram(id)
            onEditBindRequested: (id, currentBind) => {
                bindDialog.targetId = id
                bindField.text = currentBind
                bindDialog.open()
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
        title: qsTr("Set hotkey")
        anchors.centerIn: parent
        width: 340
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: processModel.setBind(targetId, bindField.text)

        background: Rectangle {
            color: Theme.surfaceElevated
            radius: Theme.radius
            border.width: 1
            border.color: Theme.outline
        }

        contentItem: ColumnLayout {
            spacing: Theme.spacingS
            Text {
                text: qsTr("Global hotkey to launch this program.")
                color: Theme.textMuted
                font.pixelSize: TypeScale.caption
            }
            TextField {
                id: bindField
                Layout.fillWidth: true
                placeholderText: qsTr("e.g. 1, F5, Ctrl+Shift+1")
                color: Theme.textPrimary
                placeholderTextColor: Theme.textMuted
                font.pixelSize: TypeScale.base
                selectByMouse: true
                onAccepted: bindDialog.accept()
                background: Rectangle {
                    radius: Theme.radiusSmall
                    color: Theme.surface
                    border.width: 1
                    border.color: bindField.activeFocus ? Theme.interactive
                                                        : Theme.outline
                    Behavior on border.color {
                        ColorAnimation { duration: Theme.animFast }
                    }
                }
            }
        }
    }
}
