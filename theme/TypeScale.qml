pragma Singleton
import QtQuick

QtObject {
    readonly property int caption: 12   // ms(-1)  метаданные/бинд
    readonly property int base:    15   // ms(0)   основной текст
    readonly property int h2:      19   // ms(1)   имя процесса
    readonly property int h1:      23   // ms(2)   заголовок
    readonly property int display: 29   // ms(3)   резерв
}
