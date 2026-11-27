import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Theme

// Сколько времени работала каждая программа: сегодня, за неделю, всего.
Item {
    id: page

    required property var app
    property bool active: false
    property string period: "today"
    property var rows: []
    readonly property real maxSeconds: rows.length > 0 ? rows[0].seconds : 0
    readonly property real totalSeconds: {
        var t = 0
        for (var i = 0; i < rows.length; ++i)
            t += rows[i].seconds
        return t
    }

    function refresh() { rows = processModel.statistics(period) }

    function formatDuration(secs) {
        var h = Math.floor(secs / 3600)
        var m = Math.floor((secs % 3600) / 60)
        if (h > 0) return qsTr("%1 h %2 min").arg(h).arg(m)
        if (m > 0) return qsTr("%1 min").arg(m)
        return qsTr("< 1 min")
    }

    onActiveChanged: if (active) refresh()
    onPeriodChanged: refresh()

    // Пока страница открыта, цифры у работающих программ растут.
    Timer {
        interval: 5000
        repeat: true
        running: page.active
        onTriggered: page.refresh()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PageHeader {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spaceXL
            Layout.rightMargin: Theme.spaceXL
            Layout.topMargin: Theme.spaceXL
            title: qsTr("Statistics")
            subtitle: page.rows.length > 0
                      ? qsTr("Total: %1").arg(page.formatDuration(page.totalSeconds))
                      : qsTr("How long each program has been running.")

            AppButton {
                text: qsTr("Clear")
                enabled: page.rows.length > 0 || page.period !== "all"
                onClicked: page.app.confirm(
                               qsTr("Clear statistics?"),
                               qsTr("Time recorded for all programs is deleted. This cannot be undone."),
                               qsTr("Clear"), true,
                               function() { processModel.clearStatistics(); page.refresh() })
            }
        }

        SegmentedControl {
            Layout.leftMargin: Theme.spaceXL
            Layout.topMargin: Theme.spaceL
            options: [{ label: qsTr("Today"), value: "today" },
                      { label: qsTr("7 days"), value: "week" },
                      { label: qsTr("All time"), value: "all" }]
            value: page.period
            onPicked: (v) => page.period = v
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: Theme.spaceM
            clip: true
            model: page.rows
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { }

            delegate: Item {
                id: entry
                required property var modelData
                width: ListView.view ? ListView.view.width : 0
                height: 52

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spaceXL
                    anchors.rightMargin: Theme.spaceXL
                    spacing: Theme.spaceM

                    Image {
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                        sourceSize.width: 24
                        sourceSize.height: 24
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        source: "image://exeicons/" + encodeURIComponent(entry.modelData.iconSource)
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 5

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.spaceS
                            Text {
                                Layout.fillWidth: true
                                text: entry.modelData.name
                                color: Theme.text
                                font.pixelSize: TypeScale.body
                                font.weight: Font.Medium
                                elide: Text.ElideRight
                            }
                            Rectangle {
                                visible: entry.modelData.running
                                Layout.preferredWidth: 8
                                Layout.preferredHeight: 8
                                radius: 4
                                color: Theme.running
                            }
                            Text {
                                text: page.formatDuration(entry.modelData.seconds)
                                color: Theme.textMuted
                                font.pixelSize: TypeScale.caption
                                font.features: { "tnum": 1 }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 4
                            radius: 2
                            color: Theme.line
                            Rectangle {
                                height: parent.height
                                radius: 2
                                color: Theme.accent
                                width: page.maxSeconds > 0
                                       ? Math.max(4, parent.width * entry.modelData.seconds / page.maxSeconds)
                                       : 0
                            }
                        }
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                width: Math.min(parent.width - 2 * Theme.spaceXL, 360)
                visible: list.count === 0
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: page.period === "today"
                      ? qsTr("Nothing has run today yet. Time is counted while a program is running.")
                      : qsTr("No time recorded yet. Time is counted while a program is running.")
                color: Theme.textMuted
                font.pixelSize: TypeScale.body
            }
        }
    }
}
