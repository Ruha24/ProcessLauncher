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
#include "windowstate.h"
#include "eventlog.h"
#include "updatechecker.h"

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
    WindowState windowState;
    EventLogModel eventLog;
    UpdateChecker updateChecker;

    QObject::connect(&manager, &ProcessManager::processExited,
                     &eventLog, &EventLogModel::logCrash);
    QObject::connect(&manager, &ProcessManager::errorOccurred,
                     &eventLog, [&eventLog](const QString&, const QString& msg) {
                         eventLog.logError(msg);
                     });

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("exeicons"), new IconProvider());
    engine.rootContext()->setContextProperty(QStringLiteral("processModel"), &model);
    engine.rootContext()->setContextProperty(QStringLiteral("filteredModel"), &filterModel);
    engine.rootContext()->setContextProperty(QStringLiteral("tray"), &tray);
    engine.rootContext()->setContextProperty(QStringLiteral("autostart"), &autostart);
    engine.rootContext()->setContextProperty(QStringLiteral("startupModel"), &startupModel);
    engine.rootContext()->setContextProperty(QStringLiteral("windowState"), &windowState);
    engine.rootContext()->setContextProperty(QStringLiteral("eventLog"), &eventLog);
    engine.rootContext()->setContextProperty(QStringLiteral("updateChecker"), &updateChecker);

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
