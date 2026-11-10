import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Theme

// Настройки применяются сразу, без кнопки «Сохранить».
Item {
    id: page

    required property var app
    readonly property bool isMac: Qt.platform.os === "osx"
    property bool manualUpdateCheck: false

    function loadNumbers() {
        pollField.text = processModel.pollInterval().toString()
        delayField.text = processModel.launchDelayMs().toString()
    }

    Component.onCompleted: loadNumbers()
    onVisibleChanged: if (visible) loadNumbers()

    Connections {
        target: updateChecker
        function onUpToDate() {
            if (page.manualUpdateCheck)
                page.app.toast(qsTr("You have the latest version"), false)
            page.manualUpdateCheck = false
        }
        function onCheckFailed(reason) {
            if (page.manualUpdateCheck)
                page.app.toast(qsTr("Could not check for updates: %1").arg(reason), true)
            page.manualUpdateCheck = false
        }
        function onUpdateAvailable(version, url, notes) {
            page.manualUpdateCheck = false
        }
    }

    component SectionTitle: Text {
        Layout.fillWidth: true
        Layout.topMargin: Theme.spaceXL
        Layout.bottomMargin: Theme.spaceXS
        color: Theme.text
        font.pixelSize: TypeScale.title
        font.weight: Font.DemiBold
    }

    component Divider: Rectangle {
        Layout.fillWidth: true
        implicitHeight: 1
        color: Theme.line
    }

    Flickable {
        id: flick
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: column.implicitHeight + 2 * Theme.spaceXL
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { }

        ColumnLayout {
            id: column
            x: Theme.spaceXL
            y: Theme.spaceXL
            width: Math.min(flick.width - 2 * Theme.spaceXL, 620)
            spacing: 0

            PageHeader {
                Layout.fillWidth: true
                title: qsTr("Settings")
            }

            SectionTitle { text: qsTr("Appearance") }
            SettingRow {
                Layout.fillWidth: true
                title: qsTr("Dark theme")
                ToggleSwitch {
                    checked: Theme.dark
                    onToggled: {
                        Theme.dark = checked
                        windowState.setValue("theme_dark", Theme.dark)
                    }
                }
            }

            SectionTitle { text: qsTr("Behavior") }
            SettingRow {
                Layout.fillWidth: true
                visible: autostart.available
                title: page.isMac ? qsTr("Open the launcher at login")
                                  : qsTr("Open the launcher when Windows starts")
                ToggleSwitch {
                    checked: autostart.enabled
                    onToggled: autostart.setEnabled(checked)
                }
            }
            Divider { visible: autostart.available }
            SettingRow {
                Layout.fillWidth: true
                title: qsTr("Start a profile when the launcher opens")
                description: qsTr("Every program in the chosen profile starts right after the launcher does.")
                AppComboBox {
                    implicitWidth: 180
                    model: [qsTr("None")].concat(page.app.profileList)
                    currentIndex: {
                        var i = page.app.profileList.indexOf(
                                    (page.app.styleVersion, processModel.autoStartProfile()))
                        return i < 0 ? 0 : i + 1
                    }
                    onActivated: (i) => processModel.setAutoStartProfile(
                                     i <= 0 ? "" : page.app.profileList[i - 1])
                }
            }
            Divider { }
            SettingRow {
                Layout.fillWidth: true
                title: qsTr("Keep running in the tray when the window is closed")
                description: qsTr("Hotkeys, the schedule and auto-restart only work while the launcher is running.")
                ToggleSwitch {
                    checked: page.app.minimizeToTray
                    onToggled: {
                        page.app.minimizeToTray = checked
                        windowState.setValue("minimize_to_tray", checked)
                    }
                }
            }
            Divider { }
            SettingRow {
                Layout.fillWidth: true
                title: qsTr("Close launched programs when the launcher quits")
                description: qsTr("Turn this off to leave your programs running after you quit or update the launcher.")
                ToggleSwitch {
                    checked: page.app.killOnExit
                    onToggled: {
                        page.app.killOnExit = checked
                        windowState.setValue("kill_on_exit", checked)
                        processModel.setKillOnExit(checked)
                    }
                }
            }

            SectionTitle { text: qsTr("Timing") }
            SettingRow {
                Layout.fillWidth: true
                title: qsTr("Pause between programs when starting a profile")
                description: qsTr("In milliseconds. 0 starts everything at once; 1000 starts one program per second.")
                AppTextField {
                    id: delayField
                    implicitWidth: 96
                    horizontalAlignment: TextInput.AlignRight
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator { bottom: 0; top: 600000 }
                    onEditingFinished: {
                        var ms = parseInt(text)
                        if (isNaN(ms) || ms < 0) ms = 0
                        processModel.setLaunchDelayMs(ms)
                        text = processModel.launchDelayMs().toString()
                    }
                }
            }
            Divider { }
            SettingRow {
                Layout.fillWidth: true
                title: qsTr("How often to check whether programs are still running")
                description: qsTr("In milliseconds, from 200 to 10000. Lower values notice a stopped program sooner.")
                AppTextField {
                    id: pollField
                    implicitWidth: 96
                    horizontalAlignment: TextInput.AlignRight
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator { bottom: 0; top: 10000 }
                    onEditingFinished: {
                        var poll = parseInt(text)
                        if (isNaN(poll)) poll = 1200
                        processModel.setPollInterval(poll)
                        text = processModel.pollInterval().toString()
                        windowState.setValue("poll_interval", processModel.pollInterval())
                    }
                }
            }

            SectionTitle { text: qsTr("Backup") }
            SettingRow {
                Layout.fillWidth: true
                title: qsTr("Programs, profiles and hotkeys")
                description: qsTr("Export them to a file to move to another computer. Importing replaces the current list and stops running programs.")
                AppButton {
                    text: qsTr("Export…")
                    onClicked: page.app.exportConfig()
                }
                AppButton {
                    text: qsTr("Import…")
                    onClicked: page.app.importConfig()
                }
            }

            SectionTitle { text: qsTr("About") }
            SettingRow {
                Layout.fillWidth: true
                title: qsTr("Process Launcher %1").arg(updateChecker.currentVersion)
                description: qsTr("Updates are checked every time the launcher starts.")
                AppButton {
                    text: qsTr("Check for updates")
                    enabled: !page.manualUpdateCheck
                    onClicked: {
                        page.manualUpdateCheck = true
                        updateChecker.checkForUpdates()
                    }
                }
            }
        }
    }
}
