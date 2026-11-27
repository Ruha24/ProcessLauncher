import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window
import Theme

// Окно быстрого поиска: открывается горячей клавишей поверх всех окон,
// Enter запускает выбранную программу или профиль.
Window {
    id: popup

    required property var app
    property var results: []
    property int current: 0

    readonly property int rowHeight: 48
    readonly property int maxRows: 7

    transientParent: null
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"
    width: 560
    height: card.implicitHeight

    function refresh() {
        results = processModel.quickSearch(field.text)
        current = 0
    }

    function toggle() {
        if (visible) {
            hide()
            return
        }
        field.text = ""
        refresh()
        var s = Screen
        // Окно на том экране, где сейчас главное окно лаунчера, на трети высоты.
        x = s.virtualX + (s.width - width) / 2
        y = s.virtualY + s.height / 4
        tray.bringToFront(popup)
        field.forceActiveFocus()
    }

    function activate(i, stop) {
        if (i < 0 || i >= results.length)
            return
        var r = results[i]
        if (r.kind === "profile") {
            if (stop) processModel.stopProfile(r.id)
            else processModel.startProfile(r.id)
        } else {
            if (stop) processModel.stop(r.id)
            else processModel.start(r.id)
        }
        app.refreshCounts()
        hide()
    }

    onActiveChanged: if (!active && visible) hide()

    Rectangle {
        id: card
        width: parent.width
        implicitHeight: column.implicitHeight + 2
        radius: Theme.radiusLarge
        color: Theme.raised
        border.width: 1
        border.color: Theme.lineStrong

        ColumnLayout {
            id: column
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 1
            spacing: 0

            TextField {
                id: field
                Layout.fillWidth: true
                implicitHeight: 52
                leftPadding: Theme.spaceL
                rightPadding: Theme.spaceL
                color: Theme.text
                placeholderText: qsTr("Start a program or profile…")
                placeholderTextColor: Theme.textFaint
                selectionColor: Theme.accent
                selectedTextColor: Theme.onAccent
                font.pixelSize: TypeScale.title
                background: null
                onTextChanged: popup.refresh()

                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Escape) {
                        popup.hide(); event.accepted = true
                    } else if (event.key === Qt.Key_Down) {
                        popup.current = Math.min(popup.current + 1, popup.results.length - 1)
                        list.positionViewAtIndex(popup.current, ListView.Contain)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Up) {
                        popup.current = Math.max(popup.current - 1, 0)
                        list.positionViewAtIndex(popup.current, ListView.Contain)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        popup.activate(popup.current, (event.modifiers & Qt.ControlModifier) !== 0)
                        event.accepted = true
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: Theme.line
            }

            ListView {
                id: list
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(count, popup.maxRows) * popup.rowHeight
                Layout.topMargin: count > 0 ? 4 : 0
                Layout.bottomMargin: count > 0 ? 4 : 0
                visible: count > 0
                clip: true
                model: popup.results
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { }

                delegate: Rectangle {
                    id: hit
                    required property var modelData
                    required property int index
                    width: ListView.view ? ListView.view.width : 0
                    height: popup.rowHeight
                    color: index === popup.current ? Theme.hover : "transparent"

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: popup.current = hit.index
                        onClicked: popup.activate(hit.index, false)
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spaceL
                        anchors.rightMargin: Theme.spaceL
                        spacing: Theme.spaceM

                        Item {
                            Layout.preferredWidth: 28
                            Layout.preferredHeight: 28
                            Image {
                                anchors.fill: parent
                                visible: hit.modelData.kind === "program"
                                sourceSize.width: 28
                                sourceSize.height: 28
                                fillMode: Image.PreserveAspectFit
                                asynchronous: true
                                source: visible ? "image://exeicons/" + encodeURIComponent(hit.modelData.iconSource) : ""
                            }
                            Text {
                                anchors.centerIn: parent
                                visible: hit.modelData.kind === "profile"
                                text: hit.modelData.emoji.length > 0 ? hit.modelData.emoji : "📂"
                                font.pixelSize: 20
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                Layout.fillWidth: true
                                text: hit.modelData.name
                                color: Theme.text
                                font.pixelSize: TypeScale.body
                                font.weight: Font.Medium
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.fillWidth: true
                                text: hit.modelData.subtitle
                                color: Theme.textFaint
                                font.pixelSize: TypeScale.caption
                                elide: Text.ElideRight
                            }
                        }

                        Text {
                            visible: hit.modelData.running
                            text: qsTr("Running")
                            color: Theme.running
                            font.pixelSize: TypeScale.caption
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.margins: Theme.spaceL
                visible: list.count === 0
                text: qsTr("Nothing found")
                color: Theme.textMuted
                font.pixelSize: TypeScale.body
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: Theme.line
            }

            Text {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.spaceL
                Layout.rightMargin: Theme.spaceL
                Layout.topMargin: Theme.spaceS
                Layout.bottomMargin: Theme.spaceS
                text: qsTr("Enter — start · %1+Enter — stop · Esc — close")
                      .arg(Qt.platform.os === "osx" ? "Cmd" : "Ctrl")
                color: Theme.textFaint
                font.pixelSize: TypeScale.caption
                elide: Text.ElideRight
            }
        }
    }
}
