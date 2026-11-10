import QtQuick
import QtQuick.Controls.Basic
import Theme

ComboBox {
    id: combo

    property string emptyText: ""

    implicitHeight: Theme.controlHeight
    implicitWidth: 180
    leftPadding: 10
    rightPadding: 28
    font.pixelSize: TypeScale.body
    opacity: enabled ? 1 : 0.45

    contentItem: Text {
        text: combo.currentIndex >= 0 ? combo.displayText : combo.emptyText
        color: combo.currentIndex >= 0 ? Theme.text : Theme.textFaint
        font: combo.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    // Стрелка — два штриха, без шрифтовых значков.
    indicator: Item {
        x: combo.width - width - 10
        y: (combo.height - height) / 2
        width: 10; height: 6
        Rectangle {
            width: 7; height: 1.5; radius: 1; color: Theme.textMuted
            x: -0.5; y: 2; rotation: 40
        }
        Rectangle {
            width: 7; height: 1.5; radius: 1; color: Theme.textMuted
            x: 3.5; y: 2; rotation: -40
        }
    }

    background: Rectangle {
        radius: Theme.radiusSmall
        color: combo.hovered ? Theme.hover : Theme.raised
        border.width: combo.visualFocus || combo.popup.visible ? 2 : 1
        border.color: combo.visualFocus || combo.popup.visible ? Theme.accent : Theme.line
    }

    delegate: ItemDelegate {
        id: option
        width: ListView.view ? ListView.view.width : combo.width
        height: 30
        padding: 0
        highlighted: combo.highlightedIndex === index

        contentItem: Text {
            leftPadding: 8
            rightPadding: 8
            text: combo.textRole.length > 0 ? model[combo.textRole] : modelData
            color: Theme.text
            font.pixelSize: TypeScale.body
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: 5
            color: option.highlighted ? Theme.hover : "transparent"
        }
    }

    popup: Popup {
        y: combo.height + 4
        width: Math.max(combo.width, 160)
        padding: 4
        implicitHeight: Math.min(contentItem.implicitHeight + 8, 264)

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: combo.delegateModel
            currentIndex: combo.highlightedIndex
            ScrollBar.vertical: ScrollBar { }
        }

        background: Rectangle {
            radius: Theme.radius
            color: Theme.raised
            border.width: 1
            border.color: Theme.lineStrong
        }
    }
}
