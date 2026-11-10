import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Theme

// Программы выбранного профиля.
Item {
    id: page

    required property var app

    readonly property string profile: app.activeProfile
    readonly property int running: (app.countsVersion, processModel.profileRunningCount(profile))
    readonly property int total: (app.countsVersion, processModel.profileTotalCount(profile))

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PageHeader {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spaceXL
            Layout.rightMargin: Theme.spaceL
            Layout.topMargin: Theme.spaceXL

            title: page.profile
            emoji: (page.app.styleVersion, processModel.profileIcon(page.profile))
            subtitle: page.total === 0 ? qsTr("No programs yet")
                    : page.running > 0 ? qsTr("%1 of %2 running").arg(page.running).arg(page.total)
                    : page.total === 1 ? qsTr("1 program") : qsTr("%1 programs").arg(page.total)
            subtitleColor: page.running > 0 ? Theme.running : Theme.textMuted

            AppButton {
                text: qsTr("Start all")
                variant: "primary"
                enabled: page.total > 0 && page.running < page.total
                onClicked: { processModel.startProfile(page.profile); page.app.refreshCounts() }
            }
            AppButton {
                text: qsTr("Stop all")
                enabled: page.running > 0
                onClicked: { processModel.stopProfile(page.profile); page.app.refreshCounts() }
            }
            IconButton {
                id: profileMore
                kind: "more"
                hint: qsTr("Profile actions")
                onClicked: page.app.showProfileMenu(page.profile, profileMore, true)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spaceXL
            Layout.rightMargin: Theme.spaceL
            Layout.topMargin: Theme.spaceL
            Layout.bottomMargin: Theme.spaceS
            spacing: Theme.spaceS

            AppTextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search in this profile")
                onTextChanged: filteredModel.setSearchText(text)
            }
            AppButton {
                text: qsTr("Add program")
                onClicked: page.app.addProgram()
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: filteredModel
            boundsBehavior: Flickable.StopAtBounds
            bottomMargin: Theme.spaceM
            ScrollBar.vertical: ScrollBar { }

            delegate: ProcessRow {
                onStartRequested: (id) => { processModel.start(id); page.app.refreshCounts() }
                onStopRequested: (id) => { processModel.stop(id); page.app.refreshCounts() }
                onRestartRequested: (id) => { processModel.restart(id); page.app.refreshCounts() }
                onToggleWatchRequested: (id, on) => processModel.setWatch(id, on)
                onEditNoteRequested: (id, note) => page.app.editNote(id, note)
                onEditBindRequested: (id, bind, name) => page.app.editBind(id, bind, name)
                onEditArgsRequested: (id, args) => page.app.editArgs(id, args)
                onMoveRequested: (id, profile) => page.app.moveProgram(id, profile)
                onRemoveRequested: (id, name) => page.app.confirmRemoveProgram(id, name)
                onOpenFolderRequested: (path) => processModel.openFileLocation(path)
                onCopyPathRequested: (path) => {
                    processModel.copyPath(path)
                    page.app.toast(qsTr("Path copied"), false)
                }
            }

            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(parent.width - 2 * Theme.spaceXL, 360)
                visible: list.count === 0
                spacing: Theme.spaceS

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: searchField.text.length > 0
                          ? qsTr("Nothing matches “%1”").arg(searchField.text)
                          : qsTr("“%1” is empty").arg(page.profile)
                    color: Theme.text
                    font.pixelSize: TypeScale.title
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    visible: searchField.text.length === 0
                    text: qsTr("Add a program, or drop a file or shortcut onto this window. Then start it with one click or a hotkey.")
                    color: Theme.textMuted
                    font.pixelSize: TypeScale.body
                }
                AppButton {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: Theme.spaceS
                    visible: searchField.text.length === 0
                    text: qsTr("Add program")
                    variant: "primary"
                    onClicked: page.app.addProgram()
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
                    processModel.addProgramFromUrl(drop.urls[i], page.profile)
                drop.accept()
                page.app.refreshCounts()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: Theme.spaceM
        visible: dropArea.containsDrag
        radius: Theme.radiusLarge
        color: Theme.accentSoft
        opacity: 0.95
        border.width: 2
        border.color: Theme.accent

        Text {
            anchors.centerIn: parent
            text: qsTr("Drop to add to “%1”").arg(page.profile)
            color: Theme.text
            font.pixelSize: TypeScale.title
            font.weight: Font.DemiBold
        }
    }
}
