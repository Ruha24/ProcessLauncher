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
#include <QCommandLineParser>
#include <QDataStream>
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
#include "languagemanager.h"
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

// Команды из командной строки, например из ярлыка на рабочем столе:
//   --profile <имя>       запустить профиль
//   --stop-profile <имя>  остановить профиль
//   --start <имя>         запустить программу (по имени в списке)
//   --stop <имя>          остановить программу
//   --hidden              не показывать окно (сразу в трей)
//   --show                показать окно даже вместе с командами
struct LaunchCommands
{
    QStringList startProfiles, stopProfiles, startPrograms, stopPrograms;
    bool hidden = false;
    bool show = false;

    bool hasActions() const
    {
        return !startProfiles.isEmpty() || !stopProfiles.isEmpty()
            || !startPrograms.isEmpty() || !stopPrograms.isEmpty();
    }
    // Окно показываем, только если его не просили спрятать и нет команд:
    // ярлык «запустить профиль» не должен каждый раз открывать лаунчер.
    bool showWindow() const { return show || (!hidden && !hasActions()); }
};

LaunchCommands parseCommands(const QStringList& args)
{
    QCommandLineParser parser;
    const QCommandLineOption profile(QStringLiteral("profile"), QString(), QStringLiteral("name"));
    const QCommandLineOption stopProfile(QStringLiteral("stop-profile"), QString(), QStringLiteral("name"));
    const QCommandLineOption start(QStringLiteral("start"), QString(), QStringLiteral("name"));
    const QCommandLineOption stop(QStringLiteral("stop"), QString(), QStringLiteral("name"));
    const QCommandLineOption hidden(QStringLiteral("hidden"));
    const QCommandLineOption show(QStringLiteral("show"));
    parser.addOptions({profile, stopProfile, start, stop, hidden, show});
    parser.parse(args);   // неизвестные ключи просто игнорируем

    LaunchCommands c;
    c.startProfiles = parser.values(profile);
    c.stopProfiles = parser.values(stopProfile);
    c.startPrograms = parser.values(start);
    c.stopPrograms = parser.values(stop);
    c.hidden = parser.isSet(hidden);
    c.show = parser.isSet(show);
    return c;
}

void runCommands(const LaunchCommands& c, ProcessManager& manager, TrayController& tray)
{
    QStringList problems;

    const QStringList profiles = manager.profiles();
    auto findProfile = [&](const QString& name) {
        for (const QString& p : profiles)
            if (p.compare(name, Qt::CaseInsensitive) == 0)
                return p;
        problems << QObject::tr("No profile named “%1”").arg(name);
        return QString();
    };
    const QList<ProcessEntry> entries = manager.entries();
    auto findProgram = [&](const QString& name) {
        for (const ProcessEntry& e : entries)
            if (e.name.compare(name, Qt::CaseInsensitive) == 0)
                return e.id;
        problems << QObject::tr("No program named “%1”").arg(name);
        return QString();
    };

    for (const QString& n : c.stopProfiles)
        if (const QString p = findProfile(n); !p.isEmpty()) manager.stopProfile(p);
    for (const QString& n : c.stopPrograms)
        if (const QString id = findProgram(n); !id.isEmpty()) manager.stop(id);
    for (const QString& n : c.startProfiles)
        if (const QString p = findProfile(n); !p.isEmpty()) manager.startProfile(p);
    for (const QString& n : c.startPrograms)
        if (const QString id = findProgram(n); !id.isEmpty()) manager.start(id);

    if (!problems.isEmpty())
        tray.notify(QStringLiteral("Process Launcher"), problems.join(QLatin1Char('\n')));
}

// true — лаунчер уже работает: передаём ему свои аргументы и выходим.
bool forwardToRunningInstance(const QString& name, const QStringList& args)
{
    QLocalSocket socket;
    socket.connectToServer(name);
    if (!socket.waitForConnected(500))
        return false;

    QByteArray payload;
    QDataStream out(&payload, QIODevice::WriteOnly);
    out << args;
    socket.write(payload);
    socket.waitForBytesWritten(1000);
    socket.disconnectFromServer();
    if (socket.state() != QLocalSocket::UnconnectedState)
        socket.waitForDisconnected(1000);
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
    if (forwardToRunningInstance(instanceName, app.arguments()))
        return 0;

    const LaunchCommands startup = parseCommands(app.arguments());

    QLocalServer instanceServer;
    instanceServer.setSocketOptions(QLocalServer::UserAccessOption);
    QLocalServer::removeServer(instanceName);
    instanceServer.listen(instanceName);

    migrateFromOldOrganization();
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/app.png")));

    app.setQuitOnLastWindowClosed(false);

    WindowState windowState;
    // До остальных объектов: их строки должны сразу выйти на нужном языке.
    LanguageManager language(&windowState);

    ProcessManager  manager;
    ProcessListModel model(&manager);

    ProcessFilterModel filterModel;
    filterModel.setSourceModel(&model);

    TrayController tray(nullptr);
    tray.setProcessManager(&manager);
    AutostartManager autostart;
    StartupModel startupModel;
    EventLogModel eventLog;
    UpdateChecker updateChecker;
    ScheduleModel scheduleModel(&manager);
    QObject::connect(&language, &LanguageManager::languageChanged,
                     &scheduleModel, &ScheduleModel::retranslate);

    // Второй экземпляр присылает свои аргументы и отключается.
    QObject::connect(&instanceServer, &QLocalServer::newConnection,
                     &tray, [&instanceServer, &tray, &manager]() {
        while (QLocalSocket* c = instanceServer.nextPendingConnection()) {
            auto* buffer = new QByteArray;
            QObject::connect(c, &QLocalSocket::readyRead, c, [c, buffer]() {
                buffer->append(c->readAll());
            });
            QObject::connect(c, &QLocalSocket::disconnected, &tray,
                             [c, buffer, &tray, &manager]() {
                buffer->append(c->readAll());
                QStringList args;
                QDataStream in(*buffer);
                in >> args;   // старые версии присылали "show" — тогда список пуст
                delete buffer;
                c->deleteLater();

                const LaunchCommands cmd = parseCommands(args);
                runCommands(cmd, manager, tray);
                if (cmd.showWindow())
                    tray.showWindow();
            });
        }
    });

    QObject::connect(&manager, &ProcessManager::processExited,
                     &eventLog, &EventLogModel::logCrash);
    QObject::connect(&manager, &ProcessManager::errorOccurred,
                     &eventLog, [&eventLog](const QString&, const QString& msg) {
                         eventLog.logError(msg);
                     });

    QQmlApplicationEngine engine;
    language.setEngine(&engine);
    engine.rootContext()->setContextProperty(QStringLiteral("language"), &language);
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
    // Без трея спрятанное окно было бы не вернуть — тогда показываем всегда.
    engine.rootContext()->setContextProperty(QStringLiteral("startHidden"),
                                             !startup.showWindow() && tray.isAvailable());

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

    runCommands(startup, manager, tray);

    return app.exec();
}
