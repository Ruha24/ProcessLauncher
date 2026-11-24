#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <QObject>
#include <QHash>
#include <QSet>
#include <QList>
#include <QString>
#include <QStringList>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QProcess;
class QTimer;
QT_END_NAMESPACE

class JobController;
class HotkeyManager;
class LinkIconFetcher;

struct ProcessEntry
{
    QString id;
    QString path;        // файл программы/скрипта, папка или ссылка (URL)
    QString kind = QStringLiteral("program");   // "program" | "folder" | "link"
    QString name;        // то, что видит пользователь: customName или имя файла без расширения
    QString customName;  // заданное пользователем имя; пусто — имя по умолчанию
    QString workDir;     // пусто — папка, где лежит программа
    bool    runAsAdmin = false;
    int     startDelaySec = 0;   // задержка от старта профиля
    QString iconPath;            // своя иконка (картинка или .exe); пусто — по самому файлу
    QString bind;
    QString args;
    QString profile;
    bool    watch = false;
    QString note;
};

class ProcessManager : public QObject
{
    Q_OBJECT

public:
    // Ссылка вида scheme://… (https, steam и т. п.), а не путь к файлу.
    static bool isLink(const QString& path);
    // Папки и ссылки только открываются: процесса, за которым следить, нет.
    static bool isOpenOnly(const ProcessEntry& e);

    explicit ProcessManager(QObject* parent = nullptr);
    ~ProcessManager() override;

    QString addProgram(const QString& path, const QString& bind = QString());
    void    removeProgram(const QString& id);
    void    setBind(const QString& id, const QString& bind);
    void    setArgs(const QString& id, const QString& args);
    void    setNote(const QString& id, const QString& note);
    void    setCustomName(const QString& id, const QString& name);
    void    setWorkDir(const QString& id, const QString& dir);
    void    setRunAsAdmin(const QString& id, bool on);
    void    setStartDelay(const QString& id, int seconds);
    void    setIcon(const QString& id, const QString& iconPath);
    // Найти иконку ссылки (сайт или игра Steam). manual — по просьбе
    // пользователя: заменить текущую иконку и сообщить, если не нашлась.
    void    fetchLinkIcon(const QString& id, bool manual);
    void    setProgramProfile(const QString& id, const QString& profile);

    void start(const QString& id);
    void stop(const QString& id);
    void restart(const QString& id);
    void stopAll();

    void setWatch(const QString& id, bool on);
    bool watch(const QString& id) const;

    bool killOnExit() const;
    void setKillOnExit(bool on);

    int  launchDelayMs() const;
    void setLaunchDelayMs(int ms);
    int  pollInterval() const;
    void setPollInterval(int ms);
    QString autoStartProfile() const;
    void setAutoStartProfile(const QString& name);

    QStringList profiles() const;
    void        addProfile(const QString& name);
    void        removeProfile(const QString& name);
    void        renameProfile(const QString& oldName, const QString& newName);
    void        startProfile(const QString& name);
    void        stopProfile(const QString& name);
    QString     profileBind(const QString& name) const;
    void        setProfileBind(const QString& name, const QString& bind);
    QString     profileColor(const QString& name) const;
    void        setProfileColor(const QString& name, const QString& color);
    QString     profileIcon(const QString& name) const;
    void        setProfileIcon(const QString& name, const QString& icon);

    bool isRunning(const QString& id) const;
    int  uptimeSeconds(const QString& id) const;
    int  profileRunningCount(const QString& name) const;
    int  profileTotalCount(const QString& name) const;

    QList<ProcessEntry> entries() const;
    QString programName(const QString& id) const;   // пусто, если программы нет

    QString configFilePath() const;

    bool exportConfig(const QString& path) const;
    bool importConfig(const QString& path);

signals:
    void runStateChanged(const QString& id, bool running);
    void listChanged();
    void profilesChanged();
    void tick();
    void processExited(const QString& name, const QString& profile);
    void profileRenamed(const QString& oldName, const QString& newName);
    void profileRemoved(const QString& name);
    void errorOccurred(const QString& id, const QString& message);

private slots:
    void pollJobs();
    void onHotkeyActivated(const QString& hotkeyId);
    void processLaunchQueue();

private:
    void save() const;
    void load();
    void resetState();
    void updatePollTimer();
    void handleExit(const QString& id);
    bool cancelDelayedStart(const QString& id);
#ifdef Q_OS_WIN
    void startElevated(const QString& id, const QString& workDir);
#endif
    void syncHotkey(const QString& id);
    void syncProfileHotkey(const QString& name);
    QString profileHotkeyId(const QString& name) const;

    QHash<QString, ProcessEntry>    m_entries;
    QHash<QString, JobController*>  m_jobs;
    QHash<QString, QDateTime>       m_startTimes;
    QHash<QString, qint64>          m_untracked;   // id -> pid
    QStringList                     m_profiles;
    QHash<QString, QString>         m_profileBinds;
    QHash<QString, QString>         m_profileColors;
    QHash<QString, QString>         m_profileIcons;
    int                             m_launchDelayMs = 0;
    bool                            m_killOnExit = true;
    QString                         m_autoStartProfile;
    QStringList                     m_launchQueue;
    QHash<QString, QTimer*>         m_delayedStarts;   // id -> таймер отложенного запуска
    QTimer*                         m_pollTimer = nullptr;
    QTimer*                         m_launchTimer = nullptr;
    HotkeyManager*                  m_hotkeys = nullptr;
    LinkIconFetcher*                m_iconFetcher = nullptr;
    QSet<QString>                   m_manualIconFetch;
};

#endif
