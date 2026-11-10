import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Theme

Item {
    id: page

    required property var app

    property string targetType: "profile"
    property bool daily: true

    function clampField(field, max) {
        var v = parseInt(field.text)
        if (isNaN(v) || v < 0) v = 0
        if (v > max) v = max
        field.text = (v < 10 ? "0" : "") + v
        return v
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PageHeader {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spaceXL
            Layout.rightMargin: Theme.spaceXL
            Layout.topMargin: Theme.spaceXL
            title: qsTr("Schedule")
            subtitle: qsTr("Start a profile or a single program at a set time. The launcher has to be running at that moment.")
        }

        // Новое правило.
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spaceXL
            Layout.rightMargin: Theme.spaceXL
            Layout.topMargin: Theme.spaceL
            implicitHeight: composer.implicitHeight + 2 * Theme.spaceL
            radius: Theme.radius
            color: Theme.sidebar
            border.width: 1
            border.color: Theme.line

            ColumnLayout {
                id: composer
                anchors.fill: parent
                anchors.margins: Theme.spaceL
                spacing: Theme.spaceM

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceS

                    Text {
                        text: qsTr("Start")
                        color: Theme.textMuted
                        font.pixelSize: TypeScale.body
                        Layout.preferredWidth: 40
                    }
                    SegmentedControl {
                        options: [{ label: qsTr("Profile"), value: "profile" },
                                  { label: qsTr("Program"), value: "program" }]
                        value: page.targetType
                        onPicked: (v) => page.targetType = v
                    }
                    AppComboBox {
                        id: profileCombo
                        Layout.fillWidth: true
                        visible: page.targetType === "profile"
                        model: page.app.profileList
                        emptyText: qsTr("Choose a profile")
                    }
                    AppComboBox {
                        id: programCombo
                        Layout.fillWidth: true
                        visible: page.targetType === "program"
                        model: processModel
                        textRole: "name"
                        valueRole: "procId"
                        emptyText: qsTr("Choose a program")
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceS

                    Text {
                        text: qsTr("at")
                        color: Theme.textMuted
                        font.pixelSize: TypeScale.body
                        Layout.preferredWidth: 40
                    }
                    AppTextField {
                        id: hourField
                        Layout.preferredWidth: 48
                        horizontalAlignment: TextInput.AlignHCenter
                        text: "09"
                        maximumLength: 2
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: IntValidator { bottom: 0; top: 23 }
                        onEditingFinished: page.clampField(hourField, 23)
                    }
                    Text {
                        text: ":"
                        color: Theme.textMuted
                        font.pixelSize: TypeScale.title
                    }
                    AppTextField {
                        id: minuteField
                        Layout.preferredWidth: 48
                        horizontalAlignment: TextInput.AlignHCenter
                        text: "00"
                        maximumLength: 2
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: IntValidator { bottom: 0; top: 59 }
                        onEditingFinished: page.clampField(minuteField, 59)
                    }
                    SegmentedControl {
                        Layout.leftMargin: Theme.spaceS
                        options: [{ label: qsTr("Every day"), value: true },
                                  { label: qsTr("Once"), value: false }]
                        value: page.daily
                        onPicked: (v) => page.daily = v
                    }
                    Item { Layout.fillWidth: true }
                    AppButton {
                        text: qsTr("Add rule")
                        variant: "primary"
                        enabled: page.targetType === "profile" ? profileCombo.currentIndex >= 0
                                                               : programCombo.currentIndex >= 0
                        onClicked: {
                            var h = page.clampField(hourField, 23)
                            var m = page.clampField(minuteField, 59)
                            if (page.targetType === "profile")
                                scheduleModel.addRule("profile", profileCombo.currentText,
                                                      profileCombo.currentText, h, m, page.daily)
                            else
                                scheduleModel.addRule("program", programCombo.currentValue,
                                                      programCombo.currentText, h, m, page.daily)
                        }
                    }
                }
            }
        }

        ListView {
            id: rules
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: Theme.spaceM
            clip: true
            model: scheduleModel
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { }

            delegate: Item {
                id: rule
                required property int index
                required property string targetType
                required property string targetLabel
                required property string timeText
                required property bool daily
                required property bool isEnabled

                width: ListView.view ? ListView.view.width : 0
                height: 56

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spaceXL
                    anchors.rightMargin: Theme.spaceXL
                    spacing: Theme.spaceL

                    Text {
                        Layout.preferredWidth: 64
                        text: rule.timeText
                        color: rule.isEnabled ? Theme.text : Theme.textFaint
                        font.pixelSize: TypeScale.heading
                        font.weight: Font.DemiBold
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text {
                            Layout.fillWidth: true
                            text: rule.targetLabel
                            color: rule.isEnabled ? Theme.text : Theme.textMuted
                            font.pixelSize: TypeScale.body
                            font.weight: Font.Medium
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            text: rule.targetType === "profile"
                                  ? (rule.daily ? qsTr("Whole profile, every day")
                                                : qsTr("Whole profile, once"))
                                  : (rule.daily ? qsTr("Program, every day")
                                                : qsTr("Program, once"))
                            color: Theme.textFaint
                            font.pixelSize: TypeScale.caption
                            elide: Text.ElideRight
                        }
                    }
                    ToggleSwitch {
                        checked: rule.isEnabled
                        onToggled: scheduleModel.setEnabled(rule.index, checked)
                    }
                    IconButton {
                        kind: "close"
                        danger: true
                        hint: qsTr("Delete rule")
                        onClicked: scheduleModel.removeRule(rule.index)
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
                width: Math.min(parent.width - 2 * Theme.spaceXL, 340)
                visible: rules.count === 0
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: qsTr("No rules yet. Pick what to start and when, then add a rule.")
                color: Theme.textMuted
                font.pixelSize: TypeScale.body
            }
        }
    }
}
