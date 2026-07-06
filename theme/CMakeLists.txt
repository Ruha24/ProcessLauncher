# Модуль темы как СТАТИЧЕСКАЯ библиотека — тогда и backing-библиотека,
# и плагин статические, и их можно слинковать в приложение (single binary).
qt_add_library(theme STATIC)

# ВАЖНО: QT_QML_SINGLETON_TYPE ставится ДО qt_add_qml_module, иначе
# singleton-запись не попадёт в qmldir.
set_source_files_properties(Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(TypeScale.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(theme
    URI Theme
    VERSION 1.0
    QML_FILES
        Theme.qml
        TypeScale.qml
)
