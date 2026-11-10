import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Theme

// Что запускается вместе с системой.
Item {
    id: page

    required property var app
    readonly property bool isMac: Qt.platform.os === "osx"

    onVisibleChanged: if (visible) startupModel.refresh()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PageHeader {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spaceXL
            Layout.rightMargin: Theme.spaceXL
            Layout.topMargin: Theme.spaceXL
            title: qsTr("Startup")
            subtitle: page.isMac
                      ? qsTr("Launch agents that start when you log in. Login Items from System Settings are not listed here.")
                      : qsTr("Programs that start together with Windows. Turning one off keeps it in the list.")

            AppButton {
                text: qsTr("Refresh")
                onClicked: startupModel.refresh()
            }
        }

        ListView {
            id: entries
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: Theme.spaceM
            clip: true
            model: startupModel
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { }

            delegate: Item {
                id: entry
                required property int index
                required property string name
                required property string command
                required property string source
                required property bool isEnabled

                width: ListView.view ? ListView.view.width : 0
                height: 56

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spaceXL
                    anchors.rightMargin: Theme.spaceXL
                    spacing: Theme.spaceL

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text {
                            Layout.fillWidth: true
                            text: entry.name
                            color: entry.isEnabled ? Theme.text : Theme.textMuted
                            font.pixelSize: TypeScale.body
                            font.weight: Font.Medium
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            text: entry.command
                            color: Theme.textFaint
                            font.pixelSize: TypeScale.caption
                            elide: Text.ElideMiddle
                        }
                    }
                    Text {
                        text: entry.source
                        color: Theme.textFaint
                        font.pixelSize: TypeScale.caption
                    }
                    ToggleSwitch {
                        checked: entry.isEnabled
                        onToggled: startupModel.setEnabled(entry.index, checked)
                    }
                    IconButton {
                        kind: "close"
                        danger: true
                        hint: qsTr("Remove from startup")
                        onClicked: {
                            var row = entry.index
                            page.app.confirm(
                                qsTr("Remove “%1” from startup?").arg(entry.name),
                                page.isMac
                                    ? qsTr("Its launch agent file goes to the Trash. The program itself stays installed.")
                                    : qsTr("It will no longer start with Windows. The program itself stays installed."),
                                qsTr("Remove"), true,
                                function() { startupModel.removeEntry(row) })
                        }
                    }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: Theme.spaceXL
                    anchors.rightMargin: Theme.spaceXL
                    height: 1
                    color: Theme.line
                }
            }

            Text {
                anchors.centerIn: parent
                visible: entries.count === 0
                text: qsTr("Nothing starts with the system.")
                color: Theme.textMuted
                font.pixelSize: TypeScale.body
            }
        }
    }
}
