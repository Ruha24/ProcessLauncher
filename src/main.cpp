#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QWindow>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QLocalServer>
#include <QLocalSocket>
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
#include "schedulemodel.h"
#include "apppaths.h"

Q_IMPORT_QML_PLUGIN(ThemePlugin)

#ifndef APP_VERSION
#define APP_VERSION "0.0"
#endif

namespace {

// Две копии лаунчера перетирали бы друг другу конфиг и делили горячие клавиши,
// поэтому запускаться может только одна. Имя канала своё у каждого
// пользователя, чтобы разные учётные записи на одном ПК друг другу не мешали.
QString instanceServerName()
{
    const QByteArray hash = QCryptographicHash::hash(
        QDir::homePath().toUtf8(), QCryptographicHash::Sha1).toHex().left(12);
    return QStringLiteral("ProcessLauncher-") + QString::fromLatin1(hash);
}

// true — лаунчер уже работает; заодно просим его показать окно.
const QString kOrganization = QStringLiteral("ruha42");
const QString kOldOrganization = QStringLiteral("YourName");

// До версии 1.5 настройки лежали под именем-заглушкой "YourName". При первом
// запуске новой версии копируем их на новое место, чтобы у тех, кто уже
// пользуется приложением, ничего не пропало. Старые файлы не трогаем.
void migrateFromOldOrganization()
{
    QCoreApplication::setOrganizationName(kOldOrganization);
    const QString oldDir =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QCoreApplication::setOrganizationName(kOrganization);

    if (AppPaths::isPortable())
        return;   // портативная копия хранит всё рядом с exe

    const QString newDir =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    const QString marker = QStringLiteral("/programs.json");
    if (oldDir != newDir && QFile::exists(oldDir + marker)
        && !QFile::exists(newDir + marker)) {
        QDir().mkpath(newDir);
        const QStringList files = QDir(oldDir).entryList(QDir::Files);
        for (const QString& name : files)
            QFile::copy(oldDir + QLatin1Char('/') + name,
                        newDir + QLatin1Char('/') + name);
    }

    QSettings fresh;
    if (fresh.allKeys().isEmpty()) {
        const QSettings old(kOldOrganization, QCoreApplication::applicationName());
        const QStringList keys = old.allKeys();
        for (const QString& key : keys)
            fresh.setValue(key, old.value(key));
    }
}

bool wakeRunningInstance(const QString& name)
{
    QLocalSocket socket;
    socket.connectToServer(name);
    if (!socket.waitForConnected(500))
        return false;
    socket.write("show");
    socket.waitForBytesWritten(500);
    return true;
}

}

int main(int argc, char* argv[])
{

    QApplication app(argc, argv);

    // От имени организации зависит, где лежат настройки пользователей (папка
    // конфигов и ветка реестра) — менять его можно только вместе с переносом,
    // как в migrateFromOldOrganization().
    QCoreApplication::setOrganizationName(kOrganization);
    QCoreApplication::setApplicationName(QStringLiteral("ProcessLauncher"));
    QCoreApplication::setApplicationVersion(QStringLiteral(APP_VERSION));

    const QString instanceName = instanceServerName();
    if (wakeRunningInstance(instanceName))
        return 0;

    QLocalServer instanceServer;
    instanceServer.setSocketOptions(QLocalServer::UserAccessOption);
    QLocalServer::removeServer(instanceName);
    instanceServer.listen(instanceName);

    migrateFromOldOrganization();
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
    ScheduleModel scheduleModel(&manager);

    QObject::connect(&instanceServer, &QLocalServer::newConnection,
                     &tray, [&instanceServer, &tray]() {
                         while (QLocalSocket* c = instanceServer.nextPendingConnection())
                             c->deleteLater();
                         tray.showWindow();
                     });

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
    engine.rootContext()->setContextProperty(QStringLiteral("scheduleModel"), &scheduleModel);

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
