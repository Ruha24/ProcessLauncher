pragma Singleton
import QtQuick

QtObject {
    readonly property color surface:        "#16181d"   // фон окна
    readonly property color surfaceElevated: "#1e2128"  // карточки/строки
    readonly property color surfaceHover:    "#262a33"
    readonly property color outline:         "#2f3440"   // границы/разделители

    readonly property color textPrimary:     "#e6e8ec"   // основной текст
    readonly property color textMuted:        "#8b909b"   // вторичный текст

    readonly property color interactive:     "#5b8cff"   // акцент/кнопки
    readonly property color interactiveHover: "#6f9bff"
    readonly property color textOnAccent:    "#ffffff"

    readonly property color running:         "#3ddc84"   // «запущен» (успех)
    readonly property color runningMuted:    "#1c3a2b"
    readonly property color danger:          "#ff6b6b"   // остановка/удаление
    readonly property color dangerMuted:     "#3a1f22"

    // Метрики
    readonly property int radius:      10
    readonly property int radiusSmall: 6
    readonly property int spacing:     12
    readonly property int spacingS:    8
    readonly property int spacingL:    18

    // Длительности анимаций (в бюджете 100–300 мс для мелких/средних элементов)
    readonly property int animFast: 120
    readonly property int animBase: 180
}
