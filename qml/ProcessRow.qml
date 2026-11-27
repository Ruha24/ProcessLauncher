import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Theme

Item {
    id: row

    required property int index
    required property string procId
    required property string name
    required property string bind
    required property string path
    required property bool   running
    required property string args
    required property string profile
    required property bool   watch
    required property string note
    required property string defaultName
    required property string workDir
    required property string defaultWorkDir
    required property bool   runAsAdmin
    required property int    startDelay
    required property string kind
    required property string iconSource
    required property bool   hasCustomIcon
    required property string windowMode
    required property string env
    // Папки и ссылки только открываются — процесса, за которым следить, нет.
    readonly property bool openOnly: kind === "folder" || kind === "link"
    // Перетаскивание за ручку слева. Строку двигает список (ProgramsPage).
    property bool reorderEnabled: false
    property bool dragging: false

    signal startRequested(string id)
    signal stopRequested(string id)
    signal restartRequested(string id)
    signal toggleWatchRequested(string id, bool on)
    signal editNoteRequested(string id, string currentNote)
    signal editBindRequested(string id, string currentBind, string name)
    signal editArgsRequested(string id, string currentArgs)
    signal renameRequested(string id, string currentName, string defaultName)
    signal chooseWorkDirRequested(string id, string currentDir, string defaultDir)
    signal resetWorkDirRequested(string id)
    signal toggleAdminRequested(string id, bool on)
    signal editDelayRequested(string id, int currentDelay, string name)
    signal chooseIconRequested(string id)
    signal moveRequested(string id, string currentProfile)
    signal removeRequested(string id, string name)
    signal openFolderRequested(string path)
    signal copyPathRequested(string path)
    signal duplicateRequested(string id)
    signal windowModeRequested(string id, string currentMode)
    signal editEnvRequested(string id, string currentEnv, string name)
    signal dragStarted(string id)
    signal dragMoved(real y)        // в координатах ListView
    signal dragFinished(bool accepted)

    property string uptimeText: ""
    property string usageText: ""

    function formatUptime(secs) {
        if (secs <= 0) return ""
        var h = Math.floor(secs / 3600)
        var m = Math.floor((secs % 3600) / 60)
        var s = secs % 60
        if (h > 0) return qsTr("%1 h %2 min").arg(h).arg(m)
        if (m > 0) return qsTr("%1 min %2 s").arg(m).arg(s)
        return qsTr("%1 s").arg(s)
    }

    function formatMemory(bytes) {
        if (bytes < 0) return ""
        var mb = bytes / (1024 * 1024)
        if (mb >= 1024) return qsTr("%1 GB").arg((mb / 1024).toFixed(1))
        return qsTr("%1 MB").arg(mb >= 100 ? Math.round(mb) : mb.toFixed(1))
    }

    function refreshUptime() {
        uptimeText = running ? formatUptime(processModel.uptimeSeconds(procId)) : ""
        if (!running || openOnly) {
            usageText = ""
            return
        }
        var parts = []
        var mem = formatMemory(processModel.memoryBytes(procId))
        if (mem.length > 0) parts.push(mem)
        var cpu = processModel.cpuPercent(procId)
        if (cpu >= 0) parts.push(qsTr("CPU %1%").arg(cpu < 10 ? cpu.toFixed(1) : Math.round(cpu)))
        usageText = parts.join(" · ")
    }

    onRunningChanged: refreshUptime()
    onProcIdChanged: refreshUptime()
    Component.onCompleted: refreshUptime()

    Connections {
        target: processModel
        function onTick() { row.refreshUptime() }
    }

    implicitHeight: 56
    width: ListView.view ? ListView.view.width : implicitWidth
    opacity: dragging ? 0.45 : 1

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: Theme.spaceM
        anchors.rightMargin: Theme.spaceM
        anchors.topMargin: 1
        anchors.bottomMargin: 1
        radius: Theme.radius
        color: hover.hovered || rowMenu.visible ? Theme.hover : "transparent"
        Behavior on color { ColorAnimation { duration: Theme.animFast } }
    }

    HoverHandler { id: hover }

    // Ручка перетаскивания: шесть точек в левом поле строки.
    Item {
        id: grip
        x: Theme.spaceM
        width: Theme.spaceXL - Theme.spaceM
        height: parent.height
        visible: row.reorderEnabled
        opacity: hover.hovered || gripArea.pressed ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.animFast } }

        Grid {
            anchors.centerIn: parent
            columns: 2
            spacing: 2
            Repeater {
                model: 6
                Rectangle {
                    width: 3; height: 3; radius: 1.5
                    color: gripArea.containsMouse || gripArea.pressed ? Theme.textMuted
                                                                      : Theme.textFaint
                }
            }
        }

        MouseArea {
            id: gripArea
            anchors.fill: parent
            hoverEnabled: true
            preventStealing: true
            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
            onPressed: row.dragStarted(row.procId)
            onPositionChanged: (mouse) => {
                if (pressed && row.ListView.view)
                    row.dragMoved(mapToItem(row.ListView.view, mouse.x, mouse.y).y)
            }
            onReleased: row.dragFinished(true)
            onCanceled: row.dragFinished(false)

            ToolTip.visible: containsMouse && !pressed
            ToolTip.delay: 700
            ToolTip.text: qsTr("Drag to reorder")
        }
    }

    TapHandler {
        acceptedButtons: Qt.LeftButton
        onDoubleTapped: {
            if (!row.running)
                row.startRequested(row.procId)
        }
    }
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: rowMenu.popup()
    }

    AppMenu {
        id: rowMenu

        AppMenuItem {
            text: row.openOnly ? qsTr("Open") : row.running ? qsTr("Stop") : qsTr("Start")
            onTriggered: row.running ? row.stopRequested(row.procId)
                                     : row.startRequested(row.procId)
        }
        AppMenuItem {
            visible: !row.openOnly
            text: qsTr("Restart")
            enabled: row.running
            onTriggered: row.restartRequested(row.procId)
        }
        AppMenuSeparator { }
        AppMenuItem {
            text: qsTr("Rename…")
            onTriggered: row.renameRequested(row.procId, row.name, row.defaultName)
        }
        AppMenuItem {
            text: qsTr("Change icon…")
            onTriggered: row.chooseIconRequested(row.procId)
        }
        AppMenuItem {
            visible: row.kind === "link"
            text: qsTr("Get icon from link")
            onTriggered: processModel.fetchLinkIcon(row.procId)
        }
        AppMenuItem {
            visible: row.hasCustomIcon
            text: qsTr("Reset icon")
            onTriggered: processModel.setIcon(row.procId, "")
        }
        AppMenuItem {
            text: qsTr("Hotkey…")
            trailingText: row.bind
            onTriggered: row.editBindRequested(row.procId, row.bind, row.name)
        }
        AppMenuItem {
            visible: !row.openOnly
            text: qsTr("Launch arguments…")
            onTriggered: row.editArgsRequested(row.procId, row.args)
        }
        AppMenuItem {
            text: qsTr("Start delay…")
            trailingText: row.startDelay > 0 ? qsTr("%1 s").arg(row.startDelay) : ""
            onTriggered: row.editDelayRequested(row.procId, row.startDelay, row.name)
        }
        AppMenuItem {
            visible: !row.openOnly
            text: qsTr("Working folder…")
            trailingText: row.workDir.length > 0 ? qsTr("custom") : ""
            onTriggered: row.chooseWorkDirRequested(row.procId, row.workDir, row.defaultWorkDir)
        }
        AppMenuItem {
            visible: !row.openOnly && row.workDir.length > 0
            text: qsTr("Reset working folder")
            onTriggered: row.resetWorkDirRequested(row.procId)
        }
        AppMenuItem {
            // Размер окна при запуске задаётся только в Windows.
            visible: Qt.platform.os === "windows" && !row.openOnly
            text: qsTr("Window…")
            trailingText: row.windowMode === "minimized" ? qsTr("minimized")
                        : row.windowMode === "maximized" ? qsTr("maximized") : ""
            onTriggered: row.windowModeRequested(row.procId, row.windowMode)
        }
        AppMenuItem {
            visible: !row.openOnly
            text: qsTr("Environment variables…")
            trailingText: row.env.length > 0 ? qsTr("set") : ""
            onTriggered: row.editEnvRequested(row.procId, row.env, row.name)
        }
        AppMenuItem {
            text: row.note.length > 0 ? qsTr("Edit note…") : qsTr("Add note…")
            onTriggered: row.editNoteRequested(row.procId, row.note)
        }
        AppMenuItem {
            visible: !row.openOnly
            text: qsTr("Restart if it stops")
            checkable: true
            checked: row.watch
            onTriggered: row.toggleWatchRequested(row.procId, !row.watch)
        }
        AppMenuItem {
            // Запуск от администратора есть только в Windows (UAC).
            visible: Qt.platform.os === "windows" && !row.openOnly
            text: qsTr("Run as administrator")
            checkable: true
            checked: row.runAsAdmin
            onTriggered: row.toggleAdminRequested(row.procId, !row.runAsAdmin)
        }
        AppMenuSeparator { }
        AppMenuItem {
            text: qsTr("Duplicate")
            onTriggered: row.duplicateRequested(row.procId)
        }
        AppMenuItem {
            text: qsTr("Move to profile…")
            onTriggered: row.moveRequested(row.procId, row.profile)
        }
        AppMenuItem {
            visible: row.kind !== "link"
            text: row.kind === "folder" ? qsTr("Open folder") : qsTr("Open file location")
            onTriggered: row.openFolderRequested(row.path)
        }
        AppMenuItem {
            text: row.kind === "link" ? qsTr("Copy link") : qsTr("Copy path")
            onTriggered: row.copyPathRequested(row.path)
        }
        AppMenuSeparator { }
        AppMenuItem {
            text: qsTr("Remove from list")
            danger: true
            onTriggered: row.removeRequested(row.procId, row.name)
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spaceXL
        anchors.rightMargin: Theme.spaceL
        spacing: Theme.spaceM

        // Лампа состояния.
        Item {
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16

            Rectangle {
                anchors.centerIn: parent
                width: 16; height: 16; radius: 8
                color: Theme.runningSoft
                opacity: row.running ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: Theme.animBase } }
            }
            Rectangle {
                anchors.centerIn: parent
                width: 8; height: 8; radius: 4
                color: row.running ? Theme.running : Theme.lineStrong
                Behavior on color { ColorAnimation { duration: Theme.animBase } }
            }
        }

        Image {
            Layout.preferredWidth: 28
            Layout.preferredHeight: 28
            sourceSize.width: 28
            sourceSize.height: 28
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            source: "image://exeicons/" + encodeURIComponent(row.iconSource)
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 1

            Text {
                id: nameLabel
                Layout.fillWidth: true
                text: row.name
                color: Theme.text
                font.pixelSize: TypeScale.body
                font.weight: Font.Medium
                elide: Text.ElideRight

                HoverHandler { id: nameHover }
                ToolTip.visible: nameHover.hovered && nameLabel.truncated
                ToolTip.delay: 500
                ToolTip.text: row.name
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceS

                Text {
                    visible: row.running
                    text: row.uptimeText.length > 0
                          ? qsTr("Running for %1").arg(row.uptimeText) : qsTr("Running")
                    color: Theme.running
                    font.pixelSize: TypeScale.caption
                }
                // Память и процессор вместе с дочерними процессами.
                Text {
                    visible: row.usageText.length > 0
                    text: row.usageText
                    color: Theme.textMuted
                    font.pixelSize: TypeScale.caption
                    font.features: { "tnum": 1 }

                    HoverHandler { id: usageHover }
                    ToolTip.visible: usageHover.hovered
                    ToolTip.delay: 500
                    ToolTip.text: qsTr("Memory and CPU, including child processes")
                }
                Text {
                    Layout.fillWidth: true
                    // Заметка важнее аргументов, аргументы важнее пути.
                    text: row.note.length > 0 ? row.note
                        : row.args.length > 0 ? row.args
                        : row.path
                    color: Theme.textFaint
                    font.pixelSize: TypeScale.caption
                    elide: row.note.length > 0 || row.args.length > 0
                           ? Text.ElideRight : Text.ElideMiddle
                }
            }
        }

        Rectangle {
            visible: row.startDelay > 0
            implicitWidth: delayLabel.implicitWidth + 14
            implicitHeight: 22
            radius: 11
            color: Theme.hover
            Text {
                id: delayLabel
                anchors.centerIn: parent
                text: qsTr("+%1 s").arg(row.startDelay)
                color: Theme.textMuted
                font.pixelSize: TypeScale.caption
            }
            HoverHandler { id: delayHover }
            ToolTip.visible: delayHover.hovered
            ToolTip.delay: 400
            ToolTip.text: qsTr("Starts %1 s after the profile starts").arg(row.startDelay)
        }

        Rectangle {
            visible: row.runAsAdmin
            implicitWidth: adminLabel.implicitWidth + 14
            implicitHeight: 22
            radius: 11
            color: Theme.accentSoft
            Text {
                id: adminLabel
                anchors.centerIn: parent
                text: qsTr("Admin")
                color: Theme.accent
                font.pixelSize: TypeScale.caption
            }
        }

        Rectangle {
            visible: row.watch
            implicitWidth: watchLabel.implicitWidth + 14
            implicitHeight: 22
            radius: 11
            color: Theme.accentSoft
            Text {
                id: watchLabel
                anchors.centerIn: parent
                text: qsTr("Auto-restart")
                color: Theme.accent
                font.pixelSize: TypeScale.caption
            }
        }

        Row {
            visible: row.bind.length > 0
            spacing: 3
            Repeater {
                model: row.bind.length > 0 ? row.bind.split("+") : []
                Keycap {
                    required property string modelData
                    text: modelData
                }
            }
        }

        AppButton {
            text: row.openOnly ? qsTr("Open") : row.running ? qsTr("Stop") : qsTr("Start")
            minWidth: 68
            onClicked: row.running ? row.stopRequested(row.procId)
                                   : row.startRequested(row.procId)
        }

        IconButton {
            id: moreButton
            kind: "more"
            hint: qsTr("More actions")
            onClicked: rowMenu.popup(moreButton, moreButton.width - rowMenu.implicitWidth, moreButton.height + 4)
        }
    }
}
