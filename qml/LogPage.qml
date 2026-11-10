import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Theme

Item {
    id: page

    required property var app

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PageHeader {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spaceXL
            Layout.rightMargin: Theme.spaceXL
            Layout.topMargin: Theme.spaceXL
            title: qsTr("Log")
            subtitle: qsTr("Programs that stopped and errors, newest first.")

            AppButton {
                text: qsTr("Clear log")
                enabled: events.count > 0
                onClicked: page.app.confirm(
                               qsTr("Clear the log?"),
                               qsTr("All recorded events are deleted. This cannot be undone."),
                               qsTr("Clear log"), true,
                               function() { eventLog.clear() })
            }
        }

        ListView {
            id: events
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: Theme.spaceM
            clip: true
            model: eventLog
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { }

            delegate: Item {
                id: event
                required property string time
                required property string kind
                required property string text
                readonly property bool isError: kind === "Error"

                width: ListView.view ? ListView.view.width : 0
                height: 44

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spaceXL
                    anchors.rightMargin: Theme.spaceXL
                    spacing: Theme.spaceM

                    Rectangle {
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 8
                        radius: 4
                        color: event.isError ? Theme.danger : Theme.lineStrong
                    }
                    Text {
                        Layout.preferredWidth: 56
                        text: event.isError ? qsTr("Error") : qsTr("Stopped")
                        color: event.isError ? Theme.danger : Theme.textMuted
                        font.pixelSize: TypeScale.caption
                    }
                    Text {
                        id: eventText
                        Layout.fillWidth: true
                        text: event.text
                        color: Theme.text
                        font.pixelSize: TypeScale.body
                        elide: Text.ElideRight

                        HoverHandler { id: eventHover }
                        ToolTip.visible: eventHover.hovered && eventText.truncated
                        ToolTip.delay: 500
                        ToolTip.text: event.text
                    }
                    Text {
                        text: event.time
                        color: Theme.textFaint
                        font.pixelSize: TypeScale.caption
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
                visible: events.count === 0
                text: qsTr("Nothing has stopped or failed yet.")
                color: Theme.textMuted
                font.pixelSize: TypeScale.body
            }
        }
    }
}
