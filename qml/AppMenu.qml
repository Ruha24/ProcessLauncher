import QtQuick
import QtQuick.Controls.Basic
import Theme

Menu {
    id: menu

    padding: 4
    overlap: 0

    delegate: AppMenuItem { }

    background: Rectangle {
        implicitWidth: 232
        radius: Theme.radius
        color: Theme.raised
        border.width: 1
        border.color: Theme.lineStrong
    }
}
