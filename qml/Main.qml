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

    Component.onCompleted: {
        var g = windowState.load()
        if (g.width > 0 && g.height > 0) {
            window.width = g.width
            window.height = g.height
        }
        if (g.x !== -1 && g.y !== -1) {
            window.x = g.x
            window.y = g.y
        }
        refreshProfiles()
        refreshCounts()
    }

    onXChanged: windowState.save(window.x, window.y, window.width, window.height)
    onYChanged: windowState.save(window.x, window.y, window.width, window.height)
    onWidthChanged: windowState.save(window.x, window.y, window.width, window.height)
    onHeightChanged: windowState.save(window.x, window.y, window.width, window.height)

    property string activeProfile: ""
    property var profileList: []
    property int profileRunning: 0
    property int profileTotal: 0

    function refreshCounts() {
        profileRunning = processModel.profileRunningCount(activeProfile)
        profileTotal = processModel.profileTotalCount(activeProfile)
    }

    Connections {
        target: processModel
        function onTick() { window.refreshCounts() }
    }

    function refreshProfiles() {
        var list = processModel.profiles()
        profileList = list
        if (list.length === 0) { activeProfile = ""; return }
        if (list.indexOf(activeProfile) < 0)
            activeProfile = list[0]
        filteredModel.setActiveProfile(activeProfile)
    }

    onActiveProfileChanged: {
        filteredModel.setActiveProfile(activeProfile)
        refreshCounts()
    }

    Connections {
        target: processModel
        function onProfilesChanged() { refreshProfiles() }
    }

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

    Connections {
        target: processModel
        function onProcessExited(name, profile) {
            if (tray.isAvailable())
                tray.notify(qsTr("Program stopped"),
                            qsTr("%1 (%2) is no longer running.").arg(name).arg(profile))
            window.refreshCounts()
        }
    }

    Connections {
        target: startupModel
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
        implicitHeight: 190
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

                Item {
                    id: autostartItem
                    Layout.preferredWidth: autostartToggle.implicitWidth
                    Layout.preferredHeight: 34
                    visible: (typeof autostart !== 'undefined') && autostart.available

                    property bool on: (typeof autostart !== 'undefined') && autostart.enabled

                    Row {
                        id: autostartToggle
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Theme.spacingS

                        Rectangle {
                            width: 20; height: 20
                            radius: Theme.radiusSmall
                            anchors.verticalCenter: parent.verticalCenter
                            color: autostartItem.on ? Theme.interactive : Theme.surfaceElevated
                            border.width: 1
                            border.color: autostartItem.on ? Theme.interactive : Theme.outline
                            Behavior on color { ColorAnimation { duration: Theme.animFast } }

                            Text {
                                anchors.centerIn: parent
                                text: "✓"
                                visible: autostartItem.on
                                color: Theme.textOnAccent
                                font.pixelSize: 13
                            }
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Start with Windows")
                            color: Theme.textMuted
                            font.pixelSize: TypeScale.caption
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (typeof autostart !== 'undefined')
                                autostart.setEnabled(!autostart.enabled)
                        }
                    }
                }

                AppButton {
                    text: qsTr("Settings")
                    variant: "secondary"
                    onClicked: {
                        settingsDelay.text = processModel.launchDelayMs().toString()
                        settingsDialog.open()
                    }
                }
                AppButton {
                    text: qsTr("Startup")
                    variant: "secondary"
                    visible: startupModel.available()
                    onClicked: {
                        startupModel.refresh()
                        startupDialog.open()
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

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingS

                Flickable {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    contentWidth: tabsRow.width
                    clip: true
                    flickableDirection: Flickable.HorizontalFlick

                    Row {
                        id: tabsRow
                        height: 34
                        spacing: Theme.spacingS

                        Repeater {
                            model: window.profileList

                            Rectangle {
                                required property string modelData
                                height: 34
                                width: tabLabel.width + 2 * Theme.spacing
                                radius: Theme.radiusSmall
                                color: modelData === window.activeProfile
                                       ? Theme.interactive : Theme.surfaceElevated
                                border.width: 1
                                border.color: modelData === window.activeProfile
                                              ? Theme.interactive : Theme.outline

                                Text {
                                    id: tabLabel
                                    anchors.centerIn: parent
                                    text: parent.modelData
                                    color: parent.modelData === window.activeProfile
                                           ? Theme.textOnAccent : Theme.textPrimary
                                    font.pixelSize: TypeScale.base
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: window.activeProfile = parent.modelData
                                }
                            }
                        }
                    }
                }

                AppButton {
                    text: "+"
                    variant: "secondary"
                    Layout.preferredWidth: 34
                    onClicked: {
                        newProfileField.text = ""
                        newProfileDialog.open()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingS

                AppButton {
                    text: qsTr("Start all")
                    variant: "primary"
                    Layout.preferredWidth: 90
                    onClicked: { processModel.startProfile(window.activeProfile); window.refreshCounts() }
                }
                AppButton {
                    text: qsTr("Stop all")
                    variant: "secondary"
                    Layout.preferredWidth: 90
                    onClicked: { processModel.stopProfile(window.activeProfile); window.refreshCounts() }
                }
                Text {
                    Layout.leftMargin: Theme.spacingS
                    text: qsTr("%1 / %2 running").arg(window.profileRunning).arg(window.profileTotal)
                    color: window.profileRunning > 0 ? Theme.running : Theme.textMuted
                    font.pixelSize: TypeScale.caption
                }
                Item { Layout.fillWidth: true }
                AppButton {
                    text: qsTr("Hotkey")
                    variant: "secondary"
                    Layout.preferredWidth: 74
                    onClicked: {
                        profileBindDialog.targetProfile = window.activeProfile
                        profileBindDialog.captured = processModel.profileBind(window.activeProfile)
                        profileBindDialog.open()
                    }
                }
                AppButton {
                    text: qsTr("Delete")
                    variant: "danger"
                    Layout.preferredWidth: 74
                    enabled: window.profileList.length > 1
                    onClicked: processModel.removeProfile(window.activeProfile)
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
            onStartRequested: (id) => { processModel.start(id); window.refreshCounts() }
            onStopRequested: (id) => { processModel.stop(id); window.refreshCounts() }
            onRemoveRequested: (id) => {
                confirmDeleteDialog.targetId = id
                confirmDeleteDialog.open()
            }
            onActivated: (id, running) => {
                if (!running) { processModel.start(id); window.refreshCounts() }
            }
            onRestartRequested: (id) => { processModel.restart(id); window.refreshCounts() }
            onToggleWatchRequested: (id, on) => processModel.setWatch(id, on)
            onOpenFolderRequested: (path) => processModel.openFileLocation(path)
            onCopyPathRequested: (path) => processModel.copyPath(path)
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
            onMoveRequested: (id, currentProfile) => {
                moveDialog.targetId = id
                moveDialog.currentProfile = currentProfile
                moveDialog.open()
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

    DropArea {
        id: dropArea
        anchors.fill: parent
        keys: ["text/uri-list"]

        onDropped: (drop) => {
            if (drop.hasUrls) {
                for (var i = 0; i < drop.urls.length; i++)
                    processModel.addProgramFromUrl(drop.urls[i], window.activeProfile)
                drop.accept()
            }
        }

        Rectangle {
            anchors.fill: parent
            visible: dropArea.containsDrag
            color: Qt.rgba(0.36, 0.55, 1.0, 0.12)
            border.width: 2
            border.color: Theme.interactive
            radius: Theme.radius
            z: 200

            Rectangle {
                anchors.centerIn: parent
                width: dropLabel.width + 2 * Theme.spacingL
                height: dropLabel.height + 2 * Theme.spacing
                radius: Theme.radius
                color: Theme.surfaceElevated
                border.width: 1
                border.color: Theme.interactive

                Text {
                    id: dropLabel
                    anchors.centerIn: parent
                    text: qsTr("Drop programs to add them to \"%1\"").arg(window.activeProfile)
                    color: Theme.textPrimary
                    font.pixelSize: TypeScale.h2
                }
            }
        }
    }

    FileDialog {
        id: exportDialog
        title: qsTr("Export configuration")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Config files (*.json)")]
        defaultSuffix: "json"
        onAccepted: {
            if (!processModel.exportConfig(selectedFile))
                errorBanner.text = qsTr("Export failed")
            else
                errorBanner.text = qsTr("Configuration exported")
            errorBanner.show()
        }
    }

    FileDialog {
        id: importDialog
        title: qsTr("Import configuration")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Config files (*.json)")]
        onAccepted: {
            if (!processModel.importConfig(selectedFile)) {
                errorBanner.text = qsTr("Import failed — invalid file")
                errorBanner.show()
            } else {
                settingsDialog.close()
                window.refreshProfiles()
                window.refreshCounts()
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Choose a program")
        nameFilters: Qt.platform.os === "windows"
                     ? [qsTr("Programs and shortcuts (*.exe *.lnk *.url)"),
                        qsTr("Executables (*.exe)"),
                        qsTr("Shortcuts (*.lnk *.url)"),
                        qsTr("All files (*)")]
                     : [qsTr("All files (*)")]
        onAccepted: processModel.addProgramFromUrl(selectedFile, window.activeProfile)
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

    Dialog {
        id: newProfileDialog
        title: qsTr("New profile")
        anchors.centerIn: parent
        width: 340
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        onOpened: newProfileField.forceActiveFocus()
        onAccepted: {
            var name = newProfileField.text.trim()
            if (name.length > 0) {
                processModel.addProfile(name)
                window.activeProfile = name
            }
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
                text: qsTr("Name for the new profile.")
                color: Theme.textMuted
                font.pixelSize: TypeScale.caption
                Layout.fillWidth: true
            }
            TextField {
                id: newProfileField
                Layout.fillWidth: true
                placeholderText: qsTr("e.g. Games, Work, Streaming")
                color: Theme.textPrimary
                placeholderTextColor: Theme.textMuted
                font.pixelSize: TypeScale.base
                selectByMouse: true
                onAccepted: newProfileDialog.accept()
                background: Rectangle {
                    radius: Theme.radiusSmall
                    color: Theme.surface
                    border.width: 1
                    border.color: newProfileField.activeFocus ? Theme.interactive
                                                              : Theme.outline
                    Behavior on border.color {
                        ColorAnimation { duration: Theme.animFast }
                    }
                }
            }
        }
    }

    Dialog {
        id: profileBindDialog
        property string targetProfile: ""
        property string captured: ""

        title: qsTr("Profile hotkey")
        anchors.centerIn: parent
        width: 360
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel | Dialog.Reset

        onOpened: profileCapture.forceActiveFocus()
        onAccepted: processModel.setProfileBind(targetProfile, captured)
        onReset: {
            captured = ""
            processModel.setProfileBind(targetProfile, "")
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
                text: qsTr("Press a key or combination to launch this whole profile.")
                color: Theme.textMuted
                font.pixelSize: TypeScale.caption
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
            Rectangle {
                id: profileCapture
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                radius: Theme.radiusSmall
                color: Theme.surface
                border.width: 1
                border.color: activeFocus ? Theme.interactive : Theme.outline
                focus: true
                activeFocusOnTab: true

                MouseArea {
                    anchors.fill: parent
                    onClicked: profileCapture.forceActiveFocus()
                }
                Keys.onShortcutOverride: (event) => { event.accepted = true }
                Keys.onPressed: (event) => {
                    if (profileBindDialog.isModifierKey(event.key)) {
                        event.accepted = true
                        return
                    }
                    var s = profileBindDialog.toBindString(event.key, event.modifiers)
                    if (s.length > 0)
                        profileBindDialog.captured = s
                    event.accepted = true
                }

                Text {
                    anchors.centerIn: parent
                    text: profileBindDialog.captured.length > 0
                          ? profileBindDialog.captured : qsTr("Press keys…")
                    color: profileBindDialog.captured.length > 0
                           ? Theme.textPrimary : Theme.textMuted
                    font.pixelSize: TypeScale.h2
                }
            }
        }
    }

    Dialog {
        id: moveDialog
        property string targetId: ""
        property string currentProfile: ""

        title: qsTr("Move to profile")
        anchors.centerIn: parent
        width: 340
        modal: true
        standardButtons: Dialog.Cancel

        background: Rectangle {
            color: Theme.surfaceElevated
            radius: Theme.radius
            border.width: 1
            border.color: Theme.outline
        }

        contentItem: ColumnLayout {
            spacing: Theme.spacingS
            Text {
                text: qsTr("Pick a profile for this program.")
                color: Theme.textMuted
                font.pixelSize: TypeScale.caption
                Layout.fillWidth: true
            }
            Repeater {
                model: window.profileList
                AppButton {
                    required property string modelData
                    Layout.fillWidth: true
                    text: modelData
                    variant: modelData === moveDialog.currentProfile
                             ? "primary" : "secondary"
                    onClicked: {
                        processModel.setProgramProfile(moveDialog.targetId, modelData)
                        moveDialog.close()
                    }
                }
            }
        }
    }

    Dialog {
        id: confirmDeleteDialog
        property string targetId: ""
        title: qsTr("Remove program")
        anchors.centerIn: parent
        width: 340
        modal: true
        standardButtons: Dialog.Yes | Dialog.No

        onAccepted: processModel.removeProgram(targetId)

        background: Rectangle {
            color: Theme.surfaceElevated
            radius: Theme.radius
            border.width: 1
            border.color: Theme.outline
        }

        contentItem: Text {
            text: qsTr("Remove this program from the list? This does not delete the program itself.")
            color: Theme.textPrimary
            font.pixelSize: TypeScale.base
            wrapMode: Text.WordWrap
        }
    }

    Dialog {
        id: settingsDialog
        title: qsTr("Settings")
        anchors.centerIn: parent
        width: 380
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: {
            var ms = parseInt(settingsDelay.text)
            if (isNaN(ms) || ms < 0) ms = 0
            processModel.setLaunchDelayMs(ms)
        }

        background: Rectangle {
            color: Theme.surfaceElevated
            radius: Theme.radius
            border.width: 1
            border.color: Theme.outline
        }

        contentItem: ColumnLayout {
            spacing: Theme.spacing

            Text {
                text: qsTr("Delay between launches in a profile (ms)")
                color: Theme.textPrimary
                font.pixelSize: TypeScale.base
                Layout.fillWidth: true
            }
            Text {
                text: qsTr("0 = launch all at once. E.g. 1000 = one program per second.")
                color: Theme.textMuted
                font.pixelSize: TypeScale.caption
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
            TextField {
                id: settingsDelay
                Layout.fillWidth: true
                inputMethodHints: Qt.ImhDigitsOnly
                color: Theme.textPrimary
                placeholderText: "0"
                placeholderTextColor: Theme.textMuted
                font.pixelSize: TypeScale.base
                background: Rectangle {
                    radius: Theme.radiusSmall
                    color: Theme.surface
                    border.width: 1
                    border.color: settingsDelay.activeFocus ? Theme.interactive : Theme.outline
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.outline }

            Text {
                text: qsTr("Auto-start a profile when the launcher opens")
                color: Theme.textPrimary
                font.pixelSize: TypeScale.base
                Layout.fillWidth: true
            }
            Flow {
                Layout.fillWidth: true
                spacing: Theme.spacingS

                AppButton {
                    text: qsTr("None")
                    variant: processModel.autoStartProfile() === "" ? "primary" : "secondary"
                    onClicked: processModel.setAutoStartProfile("")
                }
                Repeater {
                    model: window.profileList
                    AppButton {
                        required property string modelData
                        text: modelData
                        variant: processModel.autoStartProfile() === modelData
                                 ? "primary" : "secondary"
                        onClicked: processModel.setAutoStartProfile(modelData)
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.outline }

            Text {
                text: qsTr("Backup")
                color: Theme.textPrimary
                font.pixelSize: TypeScale.base
                Layout.fillWidth: true
            }
            Text {
                text: qsTr("Move your settings to another PC. Program paths may differ there.")
                color: Theme.textMuted
                font.pixelSize: TypeScale.caption
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingS
                AppButton {
                    text: qsTr("Export…")
                    variant: "secondary"
                    Layout.fillWidth: true
                    onClicked: exportDialog.open()
                }
                AppButton {
                    text: qsTr("Import…")
                    variant: "secondary"
                    Layout.fillWidth: true
                    onClicked: importDialog.open()
                }
            }
        }
    }

    Dialog {
        id: startupDialog
        title: qsTr("Windows startup")
        anchors.centerIn: parent
        width: Math.min(window.width - 2 * Theme.spacingL, 560)
        height: Math.min(window.height - 2 * Theme.spacingL, 520)
        modal: true
        standardButtons: Dialog.Close

        background: Rectangle {
            color: Theme.surfaceElevated
            radius: Theme.radius
            border.width: 1
            border.color: Theme.outline
        }

        contentItem: ColumnLayout {
            spacing: Theme.spacingS

            Text {
                text: qsTr("Programs that launch when Windows starts.")
                color: Theme.textMuted
                font.pixelSize: TypeScale.caption
                Layout.fillWidth: true
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: startupModel
                spacing: Theme.spacingS / 2

                delegate: Rectangle {
                    required property int index
                    required property string name
                    required property string command
                    required property string source
                    required property bool isEnabled

                    width: ListView.view ? ListView.view.width : 0
                    height: 58
                    radius: Theme.radiusSmall
                    color: Theme.surface
                    border.width: 1
                    border.color: Theme.outline

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacing
                        anchors.rightMargin: Theme.spacing
                        spacing: Theme.spacing

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                Layout.fillWidth: true
                                text: name
                                color: isEnabled ? Theme.textPrimary : Theme.textMuted
                                font.pixelSize: TypeScale.base
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.fillWidth: true
                                text: source + "  ·  " + command
                                color: Theme.textMuted
                                font.pixelSize: TypeScale.caption
                                elide: Text.ElideMiddle
                            }
                        }

                        AppButton {
                            text: isEnabled ? qsTr("On") : qsTr("Off")
                            variant: isEnabled ? "primary" : "secondary"
                            Layout.preferredWidth: 60
                            onClicked: startupModel.setEnabled(index, !isEnabled)
                        }
                        AppButton {
                            text: "✕"
                            variant: "secondary"
                            Layout.preferredWidth: 40
                            onClicked: startupModel.removeEntry(index)
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: parent.count === 0
                    text: qsTr("No startup programs found.")
                    color: Theme.textMuted
                    font.pixelSize: TypeScale.base
                }
            }
        }
    }
}
