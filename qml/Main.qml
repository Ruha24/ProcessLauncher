import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Theme

ApplicationWindow {
    id: window

    visible: true
    width: 940
    height: 640
    minimumWidth: 760
    minimumHeight: 480
    title: qsTr("Process Launcher")
    color: Theme.surface

    // ------------------------------------------------------------------
    // Состояние
    // ------------------------------------------------------------------
    property string page: "programs"   // programs | schedule | startup | log | settings
    property string activeProfile: ""
    property var profileList: []
    property bool minimizeToTray: true
    property bool killOnExit: true

    // Счётчики и оформление профилей читаются вызовами методов модели, поэтому
    // привязки «подталкиваются» этими номерами версий.
    property int countsVersion: 0
    property int styleVersion: 0

    readonly property var pages: ["programs", "schedule", "startup", "log", "settings"]

    function refreshCounts() { countsVersion++ }

    function refreshProfiles() {
        var list = processModel.profiles()
        profileList = list
        if (list.length === 0) { activeProfile = ""; return }
        if (list.indexOf(activeProfile) < 0)
            activeProfile = list[0]
        filteredModel.setActiveProfile(activeProfile)
    }

    function showProfile(name) {
        activeProfile = name
        page = "programs"
    }

    onActiveProfileChanged: {
        filteredModel.setActiveProfile(activeProfile)
        refreshCounts()
    }

    Component.onCompleted: {
        var g = windowState.load()
        if (g.width > 0 && g.height > 0) {
            window.width = Math.max(g.width, minimumWidth)
            window.height = Math.max(g.height, minimumHeight)
        }
        if (g.x !== -1 && g.y !== -1) {
            window.x = g.x
            window.y = g.y
        }
        Theme.dark = windowState.boolValue("theme_dark", true)
        minimizeToTray = windowState.boolValue("minimize_to_tray", true)
        killOnExit = windowState.boolValue("kill_on_exit", true)
        processModel.setKillOnExit(killOnExit)
        var poll = parseInt(windowState.value("poll_interval", 0))
        if (poll > 0) processModel.setPollInterval(poll)
        refreshProfiles()
        geometryReady = true
        updateChecker.checkForUpdates()
    }

    // Положение окна сохраняем с задержкой, а не на каждый пиксель перетаскивания.
    property bool geometryReady: false
    function geometryChanged() { if (geometryReady) geometryTimer.restart() }
    onXChanged: geometryChanged()
    onYChanged: geometryChanged()
    onWidthChanged: geometryChanged()
    onHeightChanged: geometryChanged()
    Timer {
        id: geometryTimer
        interval: 400
        onTriggered: {
            if (window.visibility === Window.Windowed)
                windowState.save(window.x, window.y, window.width, window.height)
        }
    }

    onClosing: (close) => {
        if (window.minimizeToTray && tray.isAvailable()) {
            close.accepted = false
            tray.hideWindow()
        } else {
            Qt.quit()
        }
    }

    Connections {
        target: processModel
        function onTick() { window.refreshCounts() }
        function onProfilesChanged() {
            window.refreshProfiles()
            window.styleVersion++
            window.refreshCounts()
        }
        function onErrorMessage(text) { window.toast(text, true) }
        function onProcessExited(name, profile) {
            if (tray.isAvailable())
                tray.notify(qsTr("Program stopped"),
                            qsTr("%1 (%2) is no longer running.").arg(name).arg(profile))
            window.refreshCounts()
        }
        function onRowsInserted() { window.refreshCounts() }
        function onRowsRemoved() { window.refreshCounts() }
        function onModelReset() { window.refreshCounts() }
        function onDataChanged() { window.refreshCounts() }
    }

    Connections {
        target: startupModel
        function onErrorMessage(text) { window.toast(text, true) }
    }

    Connections {
        target: updateChecker
        function onUpdateAvailable(version, url, notes) {
            updateDialog.newVersion = version
            updateDialog.releaseUrl = url
            updateDialog.notes = notes
            updateDialog.open()
        }
    }

    // ------------------------------------------------------------------
    // Действия, которые вызывают страницы
    // ------------------------------------------------------------------
    function toast(text, isError) {
        toastItem.text = text
        toastItem.isError = isError
        toastItem.show()
    }

    function addProgram() { fileDialog.open() }
    function exportConfig() { exportDialog.open() }
    function importConfig() { importDialog.open() }

    function confirm(title, text, acceptText, destructive, callback) {
        confirmDialog.title = title
        confirmDialog.message = text
        confirmDialog.acceptText = acceptText
        confirmDialog.destructive = destructive
        confirmDialog.callback = callback
        confirmDialog.open()
    }

    function askText(title, description, placeholder, value, acceptText, callback) {
        textDialog.title = title
        textDialog.description = description
        textDialog.placeholder = placeholder
        textDialog.initialValue = value
        textDialog.acceptText = acceptText
        textDialog.callback = callback
        textDialog.open()
    }

    function editBind(id, bind, name) {
        hotkeyDialog.title = qsTr("Hotkey for %1").arg(name)
        hotkeyDialog.description = qsTr("Pressing it starts this program.")
        hotkeyDialog.captured = bind
        hotkeyDialog.callback = function(b) { processModel.setBind(id, b) }
        hotkeyDialog.open()
    }

    function editProfileBind(name) {
        hotkeyDialog.title = qsTr("Hotkey for “%1”").arg(name)
        hotkeyDialog.description = qsTr("Pressing it starts every program in this profile.")
        hotkeyDialog.captured = processModel.profileBind(name)
        hotkeyDialog.callback = function(b) { processModel.setProfileBind(name, b) }
        hotkeyDialog.open()
    }

    function editArgs(id, args) {
        askText(qsTr("Launch arguments"),
                qsTr("Passed to the program on its command line every time it starts."),
                qsTr("--fullscreen --profile dev"), args, qsTr("Save"),
                function(t) { processModel.setArgs(id, t) })
    }

    function editNote(id, note) {
        askText(qsTr("Note"),
                qsTr("Shown under the program name in the list."),
                qsTr("Start after the VPN is up"), note, qsTr("Save"),
                function(t) { processModel.setNote(id, t) })
    }

    function moveProgram(id, currentProfile) {
        moveDialog.targetId = id
        moveDialog.currentProfile = currentProfile
        moveDialog.open()
    }

    function confirmRemoveProgram(id, name) {
        confirm(qsTr("Remove %1 from the list?").arg(name),
                qsTr("Its hotkey, arguments and note are forgotten. The program itself stays on your computer."),
                qsTr("Remove"), true,
                function() { processModel.removeProgram(id); window.refreshCounts() })
    }

    function newProfile() {
        askText(qsTr("New profile"),
                qsTr("A profile is a group of programs you start together."),
                qsTr("Work, Games, Streaming"), "", qsTr("Create profile"),
                function(t) {
                    var name = t.trim()
                    if (name.length === 0)
                        return
                    if (window.profileList.indexOf(name) >= 0) {
                        window.toast(qsTr("A profile named “%1” already exists").arg(name), true)
                    } else {
                        processModel.addProfile(name)
                    }
                    window.showProfile(name)
                })
    }

    function renameProfile(name) {
        askText(qsTr("Rename profile"), "", "", name, qsTr("Rename"),
                function(t) {
                    var n = t.trim()
                    if (n.length === 0 || n === name)
                        return
                    if (window.profileList.indexOf(n) >= 0) {
                        window.toast(qsTr("A profile named “%1” already exists").arg(n), true)
                        return
                    }
                    var wasActive = window.activeProfile === name
                    // Заранее переключаемся на новое имя, чтобы список не
                    // «прыгнул» на первый профиль, когда старое имя исчезнет.
                    processModel.renameProfile(name, n)
                    if (wasActive)
                        window.activeProfile = n
                })
    }

    function styleProfile(name) {
        styleDialog.targetProfile = name
        styleDialog.selectedColor = processModel.profileColor(name)
        styleDialog.selectedIcon = processModel.profileIcon(name)
        styleDialog.open()
    }

    function confirmDeleteProfile(name) {
        var fallback = profileList[0] === name ? profileList[1] : profileList[0]
        confirm(qsTr("Delete the profile “%1”?").arg(name),
                qsTr("Its programs are not removed — they move to “%1”.").arg(fallback),
                qsTr("Delete profile"), true,
                function() { processModel.removeProfile(name) })
    }

    function showProfileMenu(name, anchorItem, alignRight) {
        profileMenu.target = name
        if (anchorItem && alignRight)
            profileMenu.popup(anchorItem, anchorItem.width - profileMenu.implicitWidth,
                              anchorItem.height + 4)
        else if (anchorItem)
            profileMenu.popup(anchorItem, 0, anchorItem.height + 4)
        else
            profileMenu.popup()
    }

    // ------------------------------------------------------------------
    // Окно: боковая панель и страницы
    // ------------------------------------------------------------------
    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 224
            color: Theme.sidebar

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spaceS
                anchors.topMargin: Theme.spaceL
                spacing: 2

                RowLayout {
                    Layout.fillWidth: true
                    Layout.bottomMargin: Theme.spaceXS
                    Text {
                        Layout.fillWidth: true
                        Layout.leftMargin: Theme.spaceM
                        text: qsTr("Profiles")
                        color: Theme.textFaint
                        font.pixelSize: TypeScale.caption
                        font.weight: Font.DemiBold
                    }
                    IconButton {
                        kind: "plus"
                        hint: qsTr("New profile")
                        implicitWidth: 26
                        implicitHeight: 26
                        onClicked: window.newProfile()
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 2
                    model: window.profileList
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: SidebarItem {
                        id: profileItem
                        required property string modelData
                        readonly property int running:
                            (window.countsVersion, processModel.profileRunningCount(modelData))
                        readonly property int total:
                            (window.countsVersion, processModel.profileTotalCount(modelData))
                        readonly property string customColor:
                            (window.styleVersion, processModel.profileColor(modelData))

                        width: ListView.view ? ListView.view.width : 0
                        text: modelData
                        selected: window.page === "programs" && window.activeProfile === modelData
                        showMarker: true
                        emoji: (window.styleVersion, processModel.profileIcon(modelData))
                        markerColor: customColor.length > 0 ? customColor : Theme.lineStrong
                        badge: running > 0 ? running + "/" + total : (total > 0 ? total : "")
                        badgeActive: running > 0
                        onClicked: window.showProfile(modelData)
                        onContextRequested: window.showProfileMenu(modelData, profileItem)
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.spaceS
                    Layout.rightMargin: Theme.spaceS
                    Layout.topMargin: Theme.spaceS
                    Layout.bottomMargin: Theme.spaceS
                    implicitHeight: 1
                    color: Theme.line
                }

                SidebarItem {
                    Layout.fillWidth: true
                    text: qsTr("Schedule")
                    selected: window.page === "schedule"
                    onClicked: window.page = "schedule"
                }
                SidebarItem {
                    Layout.fillWidth: true
                    visible: startupModel.available()
                    text: qsTr("Startup")
                    selected: window.page === "startup"
                    onClicked: window.page = "startup"
                }
                SidebarItem {
                    Layout.fillWidth: true
                    text: qsTr("Log")
                    selected: window.page === "log"
                    onClicked: window.page = "log"
                }
                SidebarItem {
                    Layout.fillWidth: true
                    text: qsTr("Settings")
                    selected: window.page === "settings"
                    onClicked: window.page = "settings"
                }
            }

            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: Theme.line
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            StackLayout {
                anchors.fill: parent
                currentIndex: window.pages.indexOf(window.page)

                ProgramsPage { app: window }
                SchedulePage { app: window }
                StartupPage  { app: window }
                LogPage      { app: window }
                SettingsPage { app: window }
            }

            // Короткое сообщение внизу: ошибки и подтверждения.
            Rectangle {
                id: toastItem
                property string text: ""
                property bool isError: false
                function show() { opacity = 1; toastTimer.restart() }

                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Theme.spaceL
                width: Math.min(toastLabel.implicitWidth + 2 * Theme.spaceL,
                                parent.width - 2 * Theme.spaceXL)
                height: Math.max(36, toastLabel.height + Theme.spaceL)
                radius: Theme.radius
                color: isError ? Theme.dangerSoft : Theme.raised
                border.width: 1
                border.color: isError ? Theme.danger : Theme.lineStrong
                opacity: 0
                visible: opacity > 0
                Behavior on opacity { NumberAnimation { duration: Theme.animBase } }

                Timer {
                    id: toastTimer
                    interval: 4000
                    onTriggered: toastItem.opacity = 0
                }
                Text {
                    id: toastLabel
                    anchors.centerIn: parent
                    width: Math.min(implicitWidth, toastItem.parent.width - 4 * Theme.spaceXL)
                    text: toastItem.text
                    color: toastItem.isError ? Theme.danger : Theme.text
                    font.pixelSize: TypeScale.body
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                }
                TapHandler { onTapped: toastItem.opacity = 0 }
            }
        }
    }

    // ------------------------------------------------------------------
    // Меню профиля
    // ------------------------------------------------------------------
    AppMenu {
        id: profileMenu
        property string target: ""

        AppMenuItem {
            text: qsTr("Rename…")
            onTriggered: window.renameProfile(profileMenu.target)
        }
        AppMenuItem {
            text: qsTr("Color and icon…")
            onTriggered: window.styleProfile(profileMenu.target)
        }
        AppMenuItem {
            text: qsTr("Hotkey…")
            trailingText: (window.styleVersion, processModel.profileBind(profileMenu.target))
            onTriggered: window.editProfileBind(profileMenu.target)
        }
        AppMenuSeparator { }
        AppMenuItem {
            text: qsTr("Stop programs in all profiles")
            onTriggered: { processModel.stopAll(); window.refreshCounts() }
        }
        AppMenuSeparator { }
        AppMenuItem {
            text: qsTr("Delete profile")
            danger: true
            enabled: window.profileList.length > 1
            onTriggered: window.confirmDeleteProfile(profileMenu.target)
        }
    }

    // ------------------------------------------------------------------
    // Системные диалоги выбора файлов
    // ------------------------------------------------------------------
    FileDialog {
        id: fileDialog
        title: qsTr("Choose a program")
        nameFilters: Qt.platform.os === "windows"
                     ? [qsTr("Programs and shortcuts (*.exe *.lnk *.url)"),
                        qsTr("Executables (*.exe)"),
                        qsTr("Shortcuts (*.lnk *.url)"),
                        qsTr("All files (*)")]
                     : [qsTr("All files (*)")]
        onAccepted: {
            processModel.addProgramFromUrl(selectedFile, window.activeProfile)
            window.refreshCounts()
        }
    }

    FileDialog {
        id: exportDialog
        title: qsTr("Export configuration")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Config files (*.json)")]
        defaultSuffix: "json"
        onAccepted: {
            if (processModel.exportConfig(selectedFile))
                window.toast(qsTr("Configuration exported"), false)
            else
                window.toast(qsTr("Could not write the file. Check that the folder is writable."), true)
        }
    }

    FileDialog {
        id: importDialog
        title: qsTr("Import configuration")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Config files (*.json)")]
        onAccepted: {
            if (processModel.importConfig(selectedFile)) {
                window.refreshProfiles()
                window.refreshCounts()
                window.toast(qsTr("Configuration imported"), false)
            } else {
                window.toast(qsTr("This file is not a Process Launcher configuration. Nothing was changed."), true)
            }
        }
    }

    // ------------------------------------------------------------------
    // Диалоги
    // ------------------------------------------------------------------
    HotkeyDialog {
        id: hotkeyDialog
        property var callback: null
        onSaved: (bind) => { if (callback) callback(bind) }
    }

    AppDialog {
        id: textDialog
        property string description: ""
        property string placeholder: ""
        property string initialValue: ""
        property var callback: null

        onOpened: {
            textDialogField.text = initialValue
            textDialogField.forceActiveFocus()
            textDialogField.selectAll()
        }
        onAccepted: if (callback) callback(textDialogField.text)

        contentItem: ColumnLayout {
            spacing: Theme.spaceM
            Text {
                Layout.fillWidth: true
                visible: textDialog.description.length > 0
                text: textDialog.description
                color: Theme.textMuted
                font.pixelSize: TypeScale.body
                wrapMode: Text.WordWrap
            }
            AppTextField {
                id: textDialogField
                Layout.fillWidth: true
                placeholderText: textDialog.placeholder
                onAccepted: textDialog.accept()
            }
        }
    }

    AppDialog {
        id: confirmDialog
        property string message: ""
        property var callback: null

        preferredWidth: 420
        onAccepted: if (callback) callback()

        contentItem: Text {
            text: confirmDialog.message
            color: Theme.textMuted
            font.pixelSize: TypeScale.body
            wrapMode: Text.WordWrap
        }
    }

    AppDialog {
        id: moveDialog
        property string targetId: ""
        property string currentProfile: ""

        title: qsTr("Move to profile")
        preferredWidth: 340
        acceptText: ""
        rejectText: qsTr("Cancel")

        contentItem: ColumnLayout {
            spacing: 2
            Repeater {
                model: window.profileList
                SidebarItem {
                    required property string modelData
                    Layout.fillWidth: true
                    text: modelData
                    selected: modelData === moveDialog.currentProfile
                    showMarker: true
                    emoji: (window.styleVersion, processModel.profileIcon(modelData))
                    badge: modelData === moveDialog.currentProfile ? qsTr("current") : ""
                    onClicked: {
                        processModel.setProgramProfile(moveDialog.targetId, modelData)
                        window.refreshCounts()
                        moveDialog.close()
                    }
                }
            }
        }
    }

    AppDialog {
        id: styleDialog
        property string targetProfile: ""
        property string selectedColor: ""
        property string selectedIcon: ""

        readonly property var swatches: ["#8B8AF7", "#4ADE80", "#F87171", "#FBBF24",
                                         "#C084FC", "#2DD4BF", "#F472B6", "#94A3B8"]
        readonly property var emojis: ["🎮", "💼", "🎧", "🛠️", "🌐", "🎬",
                                       "📊", "🚀", "🔥", "⭐", "🎯", "📁"]

        title: qsTr("Color and icon for “%1”").arg(targetProfile)
        preferredWidth: 380
        clearText: qsTr("Reset")

        onAccepted: {
            processModel.setProfileColor(targetProfile, selectedColor)
            processModel.setProfileIcon(targetProfile, selectedIcon)
        }
        onCleared: {
            selectedColor = ""
            selectedIcon = ""
        }

        contentItem: ColumnLayout {
            spacing: Theme.spaceM

            Text {
                text: qsTr("Color")
                color: Theme.textMuted
                font.pixelSize: TypeScale.body
            }
            Flow {
                Layout.fillWidth: true
                spacing: Theme.spaceS
                Repeater {
                    model: styleDialog.swatches
                    Rectangle {
                        id: swatch
                        required property string modelData
                        readonly property bool current: styleDialog.selectedColor === modelData
                        width: 30; height: 30; radius: 15
                        color: "transparent"
                        border.width: current ? 2 : 0
                        border.color: Theme.text
                        Rectangle {
                            anchors.centerIn: parent
                            width: 20; height: 20; radius: 10
                            color: swatch.modelData
                        }
                        TapHandler {
                            onTapped: styleDialog.selectedColor = swatch.current ? "" : swatch.modelData
                        }
                    }
                }
            }

            Text {
                Layout.topMargin: Theme.spaceXS
                text: qsTr("Icon")
                color: Theme.textMuted
                font.pixelSize: TypeScale.body
            }
            Flow {
                Layout.fillWidth: true
                spacing: Theme.spaceXS
                Repeater {
                    model: styleDialog.emojis
                    Rectangle {
                        id: emojiCell
                        required property string modelData
                        readonly property bool current: styleDialog.selectedIcon === modelData
                        width: 36; height: 36; radius: Theme.radiusSmall
                        color: current ? Theme.accentSoft
                             : emojiHover.hovered ? Theme.hover : "transparent"
                        border.width: current ? 2 : 0
                        border.color: Theme.accent
                        Text {
                            anchors.centerIn: parent
                            text: emojiCell.modelData
                            font.pixelSize: 18
                        }
                        HoverHandler { id: emojiHover }
                        TapHandler {
                            onTapped: styleDialog.selectedIcon = emojiCell.current ? "" : emojiCell.modelData
                        }
                    }
                }
            }
        }
    }

    AppDialog {
        id: updateDialog
        property string newVersion: ""
        property string releaseUrl: ""
        property string notes: ""
        property string phase: "idle"   // idle | downloading | done
        property real progress: 0
        property string errorText: ""

        title: qsTr("Version %1 is available").arg(newVersion)
        preferredWidth: 460
        closePolicy: Popup.CloseOnEscape

        onOpened: {
            phase = "idle"
            progress = 0
            errorText = ""
        }

        Connections {
            target: updateChecker
            function onDownloadProgress(f) { updateDialog.progress = f }
            function onDownloadFinished(path) { updateDialog.phase = "done" }
            function onDownloadFailed(reason) {
                updateDialog.phase = "idle"
                updateDialog.errorText = qsTr("Download failed: %1").arg(reason)
            }
        }

        contentItem: ColumnLayout {
            spacing: Theme.spaceM

            Text {
                Layout.fillWidth: true
                text: qsTr("You have version %1.").arg(updateChecker.currentVersion)
                color: Theme.textMuted
                font.pixelSize: TypeScale.body
            }

            Rectangle {
                Layout.fillWidth: true
                visible: updateDialog.notes.trim().length > 0
                radius: Theme.radius
                color: Theme.surface
                border.width: 1
                border.color: Theme.line
                implicitHeight: Math.min(notesText.implicitHeight + 2 * Theme.spaceM, 190)

                Flickable {
                    id: notesFlick
                    anchors.fill: parent
                    anchors.margins: Theme.spaceM
                    clip: true
                    contentWidth: width
                    contentHeight: notesText.implicitHeight
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { }

                    Text {
                        id: notesText
                        width: notesFlick.width
                        text: updateDialog.notes
                        color: Theme.text
                        font.pixelSize: TypeScale.caption
                        wrapMode: Text.WordWrap
                        textFormat: Text.PlainText
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceXS
                visible: updateDialog.phase === "downloading"
                Text {
                    text: qsTr("Downloading… %1%").arg(Math.round(updateDialog.progress * 100))
                    color: Theme.textMuted
                    font.pixelSize: TypeScale.caption
                }
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 4
                    radius: 2
                    color: Theme.pressed
                    Rectangle {
                        height: parent.height
                        radius: 2
                        width: parent.width * updateDialog.progress
                        color: Theme.accent
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                visible: updateDialog.phase === "done"
                text: window.killOnExit
                      ? qsTr("Downloaded. Installing closes the launcher and the programs it started, then opens it again.")
                      : qsTr("Downloaded. Installing closes the launcher and opens it again; your programs keep running.")
                color: Theme.textMuted
                font.pixelSize: TypeScale.body
                wrapMode: Text.WordWrap
            }

            Text {
                Layout.fillWidth: true
                visible: updateDialog.errorText.length > 0
                text: updateDialog.errorText
                color: Theme.danger
                font.pixelSize: TypeScale.caption
                wrapMode: Text.WordWrap
            }
        }

        footer: Item {
            implicitHeight: 60
            Row {
                anchors.right: parent.right
                anchors.rightMargin: Theme.spaceXL
                anchors.top: parent.top
                spacing: Theme.spaceS

                AppButton {
                    text: qsTr("Later")
                    minWidth: 80
                    visible: updateDialog.phase !== "downloading"
                    onClicked: updateDialog.close()
                }
                AppButton {
                    text: qsTr("Open download page")
                    variant: "primary"
                    visible: updateDialog.phase === "idle" && !updateChecker.canDownload
                    onClicked: {
                        Qt.openUrlExternally(updateDialog.releaseUrl)
                        updateDialog.close()
                    }
                }
                AppButton {
                    text: qsTr("Download")
                    variant: "primary"
                    minWidth: 80
                    visible: updateDialog.phase === "idle" && updateChecker.canDownload
                    onClicked: {
                        updateDialog.errorText = ""
                        updateDialog.progress = 0
                        updateDialog.phase = "downloading"
                        updateChecker.downloadUpdate()
                    }
                }
                AppButton {
                    text: qsTr("Install")
                    variant: "primary"
                    minWidth: 80
                    visible: updateDialog.phase === "done"
                    onClicked: {
                        updateChecker.runInstaller()
                        updateDialog.close()
                    }
                }
            }
        }
    }
}
