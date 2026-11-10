cmake_minimum_required(VERSION 3.21)

# Единственное место, где задаётся версия приложения: отсюда её берут сам
# exe (проверка обновлений), установщик Windows и бандл macOS.
# Тег релиза должен совпадать с этим числом — CI это проверяет.
project(ProcessLauncher VERSION 1.5 LANGUAGES CXX)

file(WRITE "${CMAKE_BINARY_DIR}/version.txt" "${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Gui Qml Widgets Network)

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory(theme)

qt_add_executable(appProcessLauncher
    src/main.cpp
    src/processmanager.cpp
    src/processmanager.h
    src/processlistmodel.cpp
    src/processlistmodel.h
    src/jobcontroller.cpp
    src/jobcontroller.h
    src/hotkeymanager.cpp
    src/hotkeymanager.h
    src/traycontroller.cpp
    src/traycontroller.h
    src/iconprovider.cpp
    src/iconprovider.h
    src/processfiltermodel.cpp
    src/processfiltermodel.h
    src/autostartmanager.cpp
    src/autostartmanager.h
    src/shortcutresolver.cpp
    src/shortcutresolver.h
    src/startupmanager.cpp
    src/startupmanager.h
    src/windowstate.cpp
    src/windowstate.h
    src/eventlog.cpp
    src/eventlog.h
    src/updatechecker.cpp
    src/updatechecker.h
    src/schedulemodel.cpp
    src/schedulemodel.h
    src/apppaths.cpp
    src/apppaths.h
)

qt_add_qml_module(appProcessLauncher
    URI ProcessLauncher
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/ProgramsPage.qml
        qml/SchedulePage.qml
        qml/StartupPage.qml
        qml/LogPage.qml
        qml/SettingsPage.qml
        qml/ProcessRow.qml
        qml/SidebarItem.qml
        qml/PageHeader.qml
        qml/SettingRow.qml
        qml/HotkeyDialog.qml
        qml/Keycap.qml
        qml/AppDialog.qml
        qml/AppButton.qml
        qml/IconButton.qml
        qml/ToggleSwitch.qml
        qml/AppTextField.qml
        qml/AppComboBox.qml
        qml/SegmentedControl.qml
        qml/AppMenu.qml
        qml/AppMenuItem.qml
        qml/AppMenuSeparator.qml
)

qt_add_resources(appProcessLauncher "app_icons"
    PREFIX "/"
    FILES
        icons/app.png
)

target_compile_definitions(appProcessLauncher
    PRIVATE APP_VERSION="${PROJECT_VERSION}")

target_link_libraries(appProcessLauncher
    PRIVATE
        Qt6::Quick
        Qt6::Gui
        Qt6::Qml
        Qt6::Widgets
        Qt6::Network
        themeplugin
)

if(WIN32)
    target_sources(appProcessLauncher PRIVATE app.rc)
    set_target_properties(appProcessLauncher PROPERTIES
        WIN32_EXECUTABLE TRUE)
    target_link_libraries(appProcessLauncher PRIVATE shell32 ole32 uuid)
endif()

if(APPLE)
    # Глобальные горячие клавиши (RegisterEventHotKey).
    target_link_libraries(appProcessLauncher PRIVATE "-framework Carbon")
    set_target_properties(appProcessLauncher PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Process Launcher"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.processlauncher.app"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
endif()

install(TARGETS appProcessLauncher
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION bin
)
