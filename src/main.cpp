#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QWindow>
#include <QtQml/QQmlExtensionPlugin>

#include "processmanager.h"
#include "processlistmodel.h"
#include "processfiltermodel.h"
#include "traycontroller.h"
#include "iconprovider.h"
#include "autostartmanager.h"
#include "startupmanager.h"

Q_IMPORT_QML_PLUGIN(ThemePlugin)

int main(int argc, char* argv[])
{

    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("YourName"));
    QCoreApplication::setApplicationName(QStringLiteral("ProcessLauncher"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/app.png")));

    app.setQuitOnLastWindowClosed(false);

    ProcessManager  manager;
    ProcessListModel model(&manager);

    ProcessFilterModel filterModel;
    filterModel.setSourceModel(&model);

    TrayController tray(nullptr);
    AutostartManager autostart;
    StartupModel startupModel;

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("exeicons"), new IconProvider());
    engine.rootContext()->setContextProperty(QStringLiteral("processModel"), &model);
    engine.rootContext()->setContextProperty(QStringLiteral("filteredModel"), &filterModel);
    engine.rootContext()->setContextProperty(QStringLiteral("tray"), &tray);
    engine.rootContext()->setContextProperty(QStringLiteral("autostart"), &autostart);
    engine.rootContext()->setContextProperty(QStringLiteral("startupModel"), &startupModel);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("ProcessLauncher", "Main");

    const auto roots = engine.rootObjects();
    if (!roots.isEmpty()) {
        if (auto* window = qobject_cast<QWindow*>(roots.first()))
            tray.setWindow(window);
    }

    return app.exec();
}
