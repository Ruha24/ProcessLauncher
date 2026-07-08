import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Theme

Item {
    id: row

    required property string procId
    required property string name
    required property string bind
    required property string path
    required property bool    running
    required property string args
    required property string profile
    required property bool    watch

    signal startRequested(string id)
    signal stopRequested(string id)
    signal restartRequested(string id)
    signal toggleWatchRequested(string id, bool on)
    signal editBindRequested(string id, string currentBind)
    signal editArgsRequested(string id, string currentArgs)
    signal moveRequested(string id, string currentProfile)
    signal removeRequested(string id)
    signal activated(string id, bool running)
    signal openFolderRequested(string path)
    signal copyPathRequested(string path)

    property string uptimeText: ""

    function formatUptime(secs) {
        if (secs <= 0) return ""
        var h = Math.floor(secs / 3600)
        var m = Math.floor((secs % 3600) / 60)
        var s = secs % 60
        if (h > 0) return h + "h " + m + "m"
        if (m > 0) return m + "m " + s + "s"
        return s + "s"
    }

    function refreshUptime() {
        uptimeText = running ? formatUptime(processModel.uptimeSeconds(procId)) : ""
    }

    onRunningChanged: refreshUptime()
    Component.onCompleted: refreshUptime()

    Connections {
        target: processModel
        function onTick() { row.refreshUptime() }
    }

    implicitHeight: 60
    width: ListView.view ? ListView.view.width : implicitWidth

    Rectangle {
        id: card
        anchors.fill: parent
        anchors.leftMargin: Theme.spacing
        anchors.rightMargin: Theme.spacing
        anchors.topMargin: Theme.spacingS / 2
        anchors.bottomMargin: Theme.spacingS / 2
        radius: Theme.radius
        color: hover.hovered ? Theme.surfaceHover : Theme.surfaceElevated
        border.width: 1
        border.color: Theme.outline

        Behavior on color { ColorAnimation { duration: Theme.animFast } }

        HoverHandler { id: hover }

        TapHandler {
            acceptedButtons: Qt.LeftButton
            onDoubleTapped: row.activated(row.procId, row.running)
        }

        TapHandler {
            acceptedButtons: Qt.RightButton
            onTapped: rowMenu.open()
        }

        Menu {
            id: rowMenu
            MenuItem {
                text: row.running ? qsTr("Stop") : qsTr("Start")
                onTriggered: row.running ? row.stopRequested(row.procId)
                                         : row.startRequested(row.procId)
            }
            MenuItem {
                text: qsTr("Restart")
                enabled: row.running
                onTriggered: row.restartRequested(row.procId)
            }
            MenuItem {
                text: row.watch ? qsTr("✓ Auto-restart if it stops")
                                : qsTr("Auto-restart if it stops")
                onTriggered: row.toggleWatchRequested(row.procId, !row.watch)
            }
            MenuItem {
                text: qsTr("Open file location")
                onTriggered: row.openFolderRequested(row.path)
            }
            MenuItem {
                text: qsTr("Copy path")
                onTriggered: row.copyPathRequested(row.path)
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.spacing
            anchors.rightMargin: Theme.spacing
            spacing: Theme.spacing

            Rectangle {
                Layout.preferredWidth: 10
                Layout.preferredHeight: 10
                radius: 5
                color: row.running ? Theme.running : Theme.textMuted
                Behavior on color { ColorAnimation { duration: Theme.animBase } }
            }

            Image {
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                sourceSize.width: 28
                sourceSize.height: 28
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                cache: true
                source: "image://exeicons/" + encodeURIComponent(row.path)
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    Layout.fillWidth: true
                    text: row.name
                    color: Theme.textPrimary
                    font.pixelSize: TypeScale.h2
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    visible: row.bind.length > 0 || row.running || row.args.length > 0 || row.watch
                    text: {
                        var left = row.running
                            ? (row.bind.length > 0 ? qsTr("Running · %1").arg(row.bind)
                                                   : qsTr("Running"))
                            : row.bind
                        if (row.running && row.uptimeText.length > 0)
                            left += "  ·  " + row.uptimeText
                        if (row.args.length > 0)
                            left = left.length > 0 ? left + "  ·  " + row.args : row.args
                        if (row.watch)
                            left = left.length > 0 ? left + "  ·  ⭮ auto" : "⭮ auto"
                        return left
                    }
                    color: row.running ? Theme.running : Theme.textMuted
                    font.pixelSize: TypeScale.caption
                    elide: Text.ElideRight
                }
            }

            AppButton {
                text: row.running ? qsTr("Stop") : qsTr("Start")
                variant: row.running ? "danger" : "primary"
                Layout.preferredWidth: 92
                onClicked: row.running ? row.stopRequested(row.procId)
                                       : row.startRequested(row.procId)
            }

            AppButton {
                text: qsTr("Bind")
                variant: "secondary"
                Layout.preferredWidth: 66
                onClicked: row.editBindRequested(row.procId, row.bind)
            }

            AppButton {
                text: qsTr("Args")
                variant: "secondary"
                Layout.preferredWidth: 66
                onClicked: row.editArgsRequested(row.procId, row.args)
            }

            AppButton {
                text: "⇄"
                variant: "secondary"
                Layout.preferredWidth: 40
                onClicked: row.moveRequested(row.procId, row.profile)
            }

            AppButton {
                text: "✕"
                variant: "secondary"
                Layout.preferredWidth: 40
                onClicked: row.removeRequested(row.procId)
            }
        }
    }
}
