qt_add_library(theme STATIC)

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
