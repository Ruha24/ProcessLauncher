#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QWindow>
#include <QtQml/QQmlExtensionPlugin>

#include "processmanager.h"
#include "processlistmodel.h"
#include "traycontroller.h"

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

    TrayController tray(nullptr);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("processModel"), &model);
    engine.rootContext()->setContextProperty(QStringLiteral("tray"), &tray);

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
