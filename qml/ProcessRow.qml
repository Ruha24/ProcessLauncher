import QtQuick
import QtQuick.Layouts
import Theme

Item {
    id: row

    required property string procId
    required property string name
    required property string bind
    required property string path
    required property bool    running

    signal startRequested(string id)
    signal stopRequested(string id)
    signal editBindRequested(string id, string currentBind)
    signal removeRequested(string id)

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
                    visible: row.bind.length > 0 || row.running
                    text: row.running ? (row.bind.length > 0
                                            ? qsTr("Running · %1").arg(row.bind)
                                            : qsTr("Running"))
                                      : row.bind
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
                Layout.preferredWidth: 72
                onClicked: row.editBindRequested(row.procId, row.bind)
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
