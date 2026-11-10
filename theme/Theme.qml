pragma Singleton
import QtQuick

// Палитра и размеры интерфейса. Зелёный — только «запущено», красный — только
// опасные действия; всё интерактивное окрашено акцентом.
QtObject {
    property bool dark: true

    // Поверхности: боковая панель темнее рабочей области, «raised» — поля,
    // кнопки и диалоги поверх неё.
    readonly property color sidebar:    dark ? "#191B1F" : "#ECEEF2"
    readonly property color surface:    dark ? "#1F2226" : "#F8F9FB"
    readonly property color raised:     dark ? "#272B30" : "#FFFFFF"
    readonly property color hover:      dark ? "#2C3137" : "#E9ECF1"
    readonly property color pressed:    dark ? "#343A42" : "#DEE2E9"
    readonly property color line:       dark ? "#30353C" : "#DDE1E7"
    readonly property color lineStrong: dark ? "#454C57" : "#BFC6D0"

    readonly property color text:       dark ? "#E9EBEE" : "#1B1E23"
    readonly property color textMuted:  dark ? "#9AA1AC" : "#5F6876"
    readonly property color textFaint:  dark ? "#6C737E" : "#8D95A1"

    readonly property color accent:      dark ? "#8B8AF7" : "#5653D4"
    readonly property color accentHover: dark ? "#A09FFA" : "#4643C2"
    readonly property color accentSoft:  dark ? "#2C2C4D" : "#E4E4FA"
    readonly property color onAccent:    dark ? "#14142B" : "#FFFFFF"

    readonly property color running:     dark ? "#4ADE80" : "#17914A"
    readonly property color runningSoft: dark ? "#1E3B2B" : "#D9F2E3"
    readonly property color danger:      dark ? "#F87171" : "#D03535"
    readonly property color dangerSoft:  dark ? "#3D2224" : "#FBE3E3"

    readonly property color scrim: dark ? "#99000000" : "#59222630"

    readonly property int radiusSmall: 6
    readonly property int radius:      8
    readonly property int radiusLarge: 12

    readonly property int spaceXS: 4
    readonly property int spaceS:  8
    readonly property int spaceM:  12
    readonly property int spaceL:  16
    readonly property int spaceXL: 24

    readonly property int controlHeight: 32

    readonly property int animFast: 120
    readonly property int animBase: 180
}
