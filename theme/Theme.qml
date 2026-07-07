pragma Singleton
import QtQuick

QtObject {

    readonly property color surface:        "#16181d"
    readonly property color surfaceElevated: "#1e2128"
    readonly property color surfaceHover:    "#262a33"
    readonly property color outline:         "#2f3440"

    readonly property color textPrimary:     "#e6e8ec"
    readonly property color textMuted:        "#8b909b"

    readonly property color interactive:     "#5b8cff"
    readonly property color interactiveHover: "#6f9bff"
    readonly property color textOnAccent:    "#ffffff"

    readonly property color running:         "#3ddc84"
    readonly property color runningMuted:    "#1c3a2b"
    readonly property color danger:          "#ff6b6b"
    readonly property color dangerMuted:     "#3a1f22"

    readonly property int radius:      10
    readonly property int radiusSmall: 6
    readonly property int spacing:     12
    readonly property int spacingS:    8
    readonly property int spacingL:    18

    readonly property int animFast: 120
    readonly property int animBase: 180
}
