pragma Singleton
import QtQuick

QtObject {
    property bool dark: true

    readonly property color surface:         dark ? "#16181d" : "#f4f5f7"
    readonly property color surfaceElevated:  dark ? "#1e2128" : "#ffffff"
    readonly property color surfaceHover:     dark ? "#262a33" : "#e9ebef"
    readonly property color outline:          dark ? "#2f3440" : "#d3d7de"

    readonly property color textPrimary:      dark ? "#e6e8ec" : "#1a1c20"
    readonly property color textMuted:        dark ? "#8b909b" : "#6b7280"

    readonly property color interactive:      dark ? "#5b8cff" : "#3a6ff0"
    readonly property color interactiveHover: dark ? "#6f9bff" : "#5583ff"
    readonly property color textOnAccent:     "#ffffff"

    readonly property color running:          dark ? "#3ddc84" : "#1f9d57"
    readonly property color runningMuted:     dark ? "#1c3a2b" : "#d4f3e0"
    readonly property color danger:           dark ? "#ff6b6b" : "#e03131"
    readonly property color dangerMuted:      dark ? "#3a1f22" : "#ffe3e3"

    readonly property int radius:      10
    readonly property int radiusSmall: 6
    readonly property int spacing:     12
    readonly property int spacingS:    8
    readonly property int spacingL:    18

    readonly property int animFast: 120
    readonly property int animBase: 180
}
