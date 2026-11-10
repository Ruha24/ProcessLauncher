import QtQuick
import QtQuick.Controls.Basic
import Theme

// Диалог в стиле приложения. Кнопки задаются текстом: пустая строка прячет
// кнопку. clearText добавляет слева кнопку «очистить» (сигнал cleared).
Dialog {
    id: dialog

    property int preferredWidth: 400
    property string acceptText: qsTr("Save")
    property string rejectText: qsTr("Cancel")
    property string clearText: ""
    property bool acceptEnabled: true
    property bool destructive: false

    signal cleared()

    anchors.centerIn: Overlay.overlay
    width: Math.min(preferredWidth,
                    (Overlay.overlay ? Overlay.overlay.width : preferredWidth) - 2 * Theme.spaceXL)
    modal: true
    padding: Theme.spaceXL
    topPadding: Theme.spaceXS
    bottomPadding: Theme.spaceL

    Overlay.modal: Rectangle { color: Theme.scrim }

    background: Rectangle {
        radius: Theme.radiusLarge
        color: Theme.raised
        border.width: 1
        border.color: Theme.lineStrong
    }

    header: Item {
        implicitHeight: 56
        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: Theme.spaceXL
            anchors.rightMargin: Theme.spaceXL
            anchors.verticalCenter: parent.verticalCenter
            text: dialog.title
            color: Theme.text
            font.pixelSize: TypeScale.title
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }
    }

    footer: Item {
        implicitHeight: 60

        AppButton {
            anchors.left: parent.left
            anchors.leftMargin: Theme.spaceXL
            anchors.top: parent.top
            visible: dialog.clearText.length > 0
            text: dialog.clearText
            variant: "ghost"
            onClicked: dialog.cleared()
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: Theme.spaceXL
            anchors.top: parent.top
            spacing: Theme.spaceS

            AppButton {
                visible: dialog.rejectText.length > 0
                text: dialog.rejectText
                minWidth: 80
                onClicked: dialog.reject()
            }
            AppButton {
                visible: dialog.acceptText.length > 0
                text: dialog.acceptText
                variant: dialog.destructive ? "danger" : "primary"
                minWidth: 80
                enabled: dialog.acceptEnabled
                onClicked: dialog.accept()
            }
        }
    }
}
