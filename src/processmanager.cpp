#include "processmanager.h"
#include "jobcontroller.h"
#include "hotkeymanager.h"
#include "apppaths.h"
#include "shortcutresolver.h"
#include "linkiconfetcher.h"

#include <QProcess>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>
#include <QDesktopServices>
#include <QRegularExpression>
#include <QThread>
#include <algorithm>
#include <utility>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#include <string>
#endif

namespace {
constexpr int kPollIntervalMs = 1200;
const QString kProfilePrefix = QStringLiteral("profile:");
const QString kSearchHotkeyId = QStringLiteral("search:");
const QString kDefaultSearchBind = QStringLiteral("Ctrl+Alt+Space");

QString defaultName(const QString& path)
{
    if (ProcessManager::isLink(path)) {
        const QUrl url(path);
        QString host = url.host();
        if (host.startsWith(QLatin1String("www.")))
            host = host.mid(4);
        return host.isEmpty() ? path : host;
    }
    const QFileInfo info(path);
    if (info.isDir())
        return info.fileName().isEmpty() ? QDir::toNativeSeparators(path) : info.fileName();
    const QString base = info.completeBaseName();
    return base.isEmpty() ? info.fileName() : base;
}

QString suffixOf(const QString& path)
{
    return QFileInfo(path).suffix().toLower();
}

QString effectiveName(const ProcessEntry& e)
{
    return e.customName.isEmpty() ? defaultName(e.path) : e.customName;
}

// Окружение для запуска: системное плюс строки ИМЯ=значение из настроек
// программы. %ИМЯ% и ${ИМЯ} в значении заменяются текущими значениями
// (PATH=C:\tools;%PATH%), пустое значение убирает переменную.
QProcessEnvironment buildEnvironment(const QString& text)
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    static const QRegularExpression ref(
        QStringLiteral("%([A-Za-z_][A-Za-z0-9_()]*)%|\\$\\{([A-Za-z_][A-Za-z0-9_]*)\\}"));
    const QStringList lines = text.split(QLatin1Char('\n'));
    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        const qsizetype eq = line.indexOf(QLatin1Char('='));
        if (eq <= 0)
            continue;
        const QString name = line.left(eq).trimmed();
        const QString raw = line.mid(eq + 1).trimmed();
        QString value;
        qsizetype last = 0;
        auto m = ref.globalMatch(raw);
        while (m.hasNext()) {
            const auto match = m.next();
            value += raw.mid(last, match.capturedStart() - last);
            const QString var = match.captured(1).isEmpty() ? match.captured(2)
                                                            : match.captured(1);
            value += env.value(var);
            last = match.capturedEnd();
        }
        value += raw.mid(last);
        if (value.isEmpty())
            env.remove(name);
        else
            env.insert(name, value);
    }
    return env;
}

bool isValidWindowMode(const QString& mode)
{
    return mode.isEmpty() || mode == QLatin1String("minimized")
           || mode == QLatin1String("maximized");
}

#ifdef Q_OS_WIN
WORD showCommand(const QString& mode)
{
    if (mode == QLatin1String("minimized"))
        return SW_SHOWMINNOACTIVE;   // свернуть и не отнимать фокус
    if (mode == QLatin1String("maximized"))
        return SW_SHOWMAXIMIZED;
    return SW_SHOWNORMAL;
}
#endif
}

ProcessManager::ProcessManager(QObject* parent)
    : QObject(parent)
    , m_pollTimer(new QTimer(this))
    , m_launchTimer(new QTimer(this))
    , m_hotkeys(new HotkeyManager(this))
    , m_iconFetcher(new LinkIconFetcher(this))
{
    connect(m_iconFetcher, &LinkIconFetcher::finished, this,
            [this](const QString& id, const QString& iconPath) {
        const bool manual = m_manualIconFetch.remove(id);
        const auto it = m_entries.constFind(id);
        if (it == m_entries.constEnd())
            return;
        if (iconPath.isEmpty()) {
            if (manual)
                emit errorOccurred(id, tr("Couldn't find an icon for %1").arg(it->name));
            return;
        }
        // Автоматически не перетираем иконку, которую пользователь уже выбрал сам.
        if (manual || it->iconPath.isEmpty())
            setIcon(id, iconPath);
    });

    m_clock.start();

    // Раз в несколько секунд ищем программы из списка, запущенные в обход
    // лаунчера: тогда они показываются как работающие.
    m_scanTimer = new QTimer(this);
    m_scanTimer->setInterval(3000);
    connect(m_scanTimer, &QTimer::timeout, this, &ProcessManager::scanExternal);
    m_scanTimer->start();
    QTimer::singleShot(0, this, &ProcessManager::scanExternal);

    m_pollTimer->setInterval(kPollIntervalMs);
    connect(m_pollTimer, &QTimer::timeout, this, &ProcessManager::pollJobs);

    m_launchTimer->setSingleShot(true);
    connect(m_launchTimer, &QTimer::timeout,
            this, &ProcessManager::processLaunchQueue);

    connect(m_hotkeys, &HotkeyManager::activated,
            this, &ProcessManager::onHotkeyActivated);

    m_searchBind = kDefaultSearchBind;
    load();
    loadStats();
    if (!m_searchBind.isEmpty())
        m_hotkeys->setHotkey(kSearchHotkeyId, m_searchBind);

    if (!m_autoStartProfile.isEmpty() && m_profiles.contains(m_autoStartProfile))
        startProfile(m_autoStartProfile);
}

ProcessManager::~ProcessManager()
{
    accountTime();
    saveStats();
    // Выход из лаунчера: в зависимости от настройки либо закрываем всё, что
    // запускали, либо оставляем программы работать дальше.
    if (m_killOnExit) {
        for (JobController* job : std::as_const(m_jobs))
            job->terminate();
    }
    qDeleteAll(m_jobs);
    m_jobs.clear();
}

bool ProcessManager::killOnExit() const
{
    return m_killOnExit;
}

void ProcessManager::setKillOnExit(bool on)
{
    if (m_killOnExit == on)
        return;
    m_killOnExit = on;
    for (JobController* job : std::as_const(m_jobs))
        job->setKillOnClose(on);
}

QString ProcessManager::configFilePath() const
{
    return AppPaths::configFile(QStringLiteral("programs.json"));
}

bool ProcessManager::exportConfig(const QString& path) const
{
    save();
    QString target = path;
    if (QUrl(path).isLocalFile())
        target = QUrl(path).toLocalFile();

    QFile::remove(target);
    return QFile::copy(configFilePath(), target);
}

bool ProcessManager::importConfig(const QString& path)
{
    QString source = path;
    if (QUrl(path).isLocalFile())
        source = QUrl(path).toLocalFile();

    QFile in(source);
    if (!in.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    const QByteArray raw = in.readAll();
    in.close();

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (doc.isNull())
        return false;

    // Принимаем только то, что похоже на наш конфиг: иначе любой посторонний
    // .json молча стёр бы все программы и профили.
    if (doc.isObject()) {
        if (!doc.object().value(QStringLiteral("programs")).isArray())
            return false;
    } else if (!doc.isArray()) {
        return false;
    }

    // Сначала надёжно пишем файл — и только после успеха трогаем текущее
    // состояние. При ошибке записи всё остаётся как было.
    QSaveFile out(configFilePath());
    if (!out.open(QIODevice::WriteOnly))
        return false;
    out.write(raw);
    if (!out.commit())
        return false;

    stopAll();
    resetState();
    load();
    if (!m_searchBind.isEmpty())
        m_hotkeys->setHotkey(kSearchHotkeyId, m_searchBind);
    emit listChanged();
    emit profilesChanged();
    return true;
}

void ProcessManager::onHotkeyActivated(const QString& hotkeyId)
{
    if (hotkeyId == kSearchHotkeyId)
        emit searchRequested();
    else if (hotkeyId.startsWith(kProfilePrefix))
        startProfile(hotkeyId.mid(kProfilePrefix.size()));
    else
        start(hotkeyId);
}

QStringList ProcessManager::profiles() const
{
    return m_profiles;
}

void ProcessManager::addProfile(const QString& name)
{
    const QString n = name.trimmed();
    if (n.isEmpty() || m_profiles.contains(n))
        return;
    m_profiles.append(n);
    save();
    emit profilesChanged();
}

void ProcessManager::removeProfile(const QString& name)
{
    if (!m_profiles.contains(name) || m_profiles.size() <= 1)
        return;

    const QString fallback = m_profiles.value(0) == name
                                 ? m_profiles.value(1)
                                 : m_profiles.value(0);

    for (ProcessEntry& e : m_entries) {
        if (e.profile == name)
            e.profile = fallback;
    }

    m_hotkeys->clearHotkey(profileHotkeyId(name));
    m_profileBinds.remove(name);
    m_profileColors.remove(name);
    m_profileIcons.remove(name);
    m_profiles.removeAll(name);
    if (m_autoStartProfile == name)
        m_autoStartProfile.clear();

    save();
    emit profileRemoved(name);
    emit profilesChanged();
    emit listChanged();
}

void ProcessManager::renameProfile(const QString& oldName, const QString& newName)
{
    const QString n = newName.trimmed();
    const int idx = m_profiles.indexOf(oldName);
    if (idx < 0 || n.isEmpty() || m_profiles.contains(n))
        return;

    m_profiles[idx] = n;
    for (ProcessEntry& e : m_entries) {
        if (e.profile == oldName)
            e.profile = n;
    }
    if (m_profileBinds.contains(oldName)) {
        m_profileBinds.insert(n, m_profileBinds.take(oldName));
        m_hotkeys->clearHotkey(profileHotkeyId(oldName));
        syncProfileHotkey(n);
    }
    if (m_profileColors.contains(oldName))
        m_profileColors.insert(n, m_profileColors.take(oldName));
    if (m_profileIcons.contains(oldName))
        m_profileIcons.insert(n, m_profileIcons.take(oldName));
    if (m_autoStartProfile == oldName)
        m_autoStartProfile = n;

    save();
    emit profileRenamed(oldName, n);
    emit profilesChanged();
    emit listChanged();
}

void ProcessManager::setProgramProfile(const QString& id, const QString& profile)
{
    auto it = m_entries.find(id);
    if (it == m_entries.end() || !m_profiles.contains(profile)
        || it->profile == profile)
        return;

    it->profile = profile;
    save();
    emit listChanged();
}

void ProcessManager::startProfile(const QString& name)
{
    // entries() идёт в порядке списка — программы стартуют сверху вниз.
    QList<QString> ids;
    for (const ProcessEntry& e : entries()) {
        if (e.profile != name || isRunning(e.id) || m_delayedStarts.contains(e.id))
            continue;
        if (e.startDelaySec <= 0) {
            ids.append(e.id);
            continue;
        }

        // Своя задержка: запускаем по таймеру, независимо от остальных.
        auto* timer = new QTimer(this);
        timer->setSingleShot(true);
        const QString id = e.id;
        connect(timer, &QTimer::timeout, this, [this, id]() {
            cancelDelayedStart(id);
            if (m_entries.contains(id) && !isRunning(id))
                start(id);
        });
        m_delayedStarts.insert(id, timer);
        timer->start(e.startDelaySec * 1000);
    }

    if (m_launchDelayMs <= 0) {
        for (const QString& id : ids)
            start(id);
        return;
    }

    m_launchQueue.append(ids);
    if (!m_launchTimer->isActive() && !m_launchQueue.isEmpty())
        processLaunchQueue();
}

void ProcessManager::processLaunchQueue()
{
    if (m_launchQueue.isEmpty())
        return;

    const QString id = m_launchQueue.takeFirst();
    if (m_entries.contains(id) && !isRunning(id))
        start(id);

    if (!m_launchQueue.isEmpty())
        m_launchTimer->start(m_launchDelayMs);
}

bool ProcessManager::cancelDelayedStart(const QString& id)
{
    QTimer* timer = m_delayedStarts.take(id);
    if (!timer)
        return false;
    timer->stop();
    timer->deleteLater();
    return true;
}

void ProcessManager::stopProfile(const QString& name)
{
    const QList<QString> ids = m_entries.keys();
    for (const QString& id : ids) {
        if (m_entries.value(id).profile == name)
            stop(id);
    }
}

QString ProcessManager::profileBind(const QString& name) const
{
    return m_profileBinds.value(name);
}

void ProcessManager::setProfileBind(const QString& name, const QString& bind)
{
    QString normalized = bind.trimmed();
    if (!normalized.isEmpty() && !HotkeyManager::isValidBind(normalized)) {
        emit errorOccurred(QString(), tr("Invalid hotkey: %1").arg(normalized));
        normalized.clear();
    }
    if (m_profileBinds.value(name) == normalized)
        return;

    if (normalized.isEmpty())
        m_profileBinds.remove(name);
    else
        m_profileBinds.insert(name, normalized);

    syncProfileHotkey(name);
    save();
    emit profilesChanged();
}

QString ProcessManager::searchBind() const
{
    return m_searchBind;
}

void ProcessManager::setSearchBind(const QString& bind)
{
    QString normalized = bind.trimmed();
    if (!normalized.isEmpty() && !HotkeyManager::isValidBind(normalized)) {
        emit errorOccurred(QString(), tr("Invalid hotkey: %1").arg(normalized));
        return;
    }
    if (m_searchBind == normalized)
        return;
    m_searchBind = normalized;
    m_hotkeys->clearHotkey(kSearchHotkeyId);
    if (!m_searchBind.isEmpty())
        m_hotkeys->setHotkey(kSearchHotkeyId, m_searchBind);
    save();
    emit profilesChanged();
}

QString ProcessManager::profileColor(const QString& name) const
{
    return m_profileColors.value(name);
}

void ProcessManager::setProfileColor(const QString& name, const QString& color)
{
    if (m_profileColors.value(name) == color)
        return;
    if (color.isEmpty())
        m_profileColors.remove(name);
    else
        m_profileColors.insert(name, color);
    save();
    emit profilesChanged();
}

QString ProcessManager::profileIcon(const QString& name) const
{
    return m_profileIcons.value(name);
}

void ProcessManager::setProfileIcon(const QString& name, const QString& icon)
{
    if (m_profileIcons.value(name) == icon)
        return;
    if (icon.isEmpty())
        m_profileIcons.remove(name);
    else
        m_profileIcons.insert(name, icon);
    save();
    emit profilesChanged();
}

QString ProcessManager::profileHotkeyId(const QString& name) const
{
    return kProfilePrefix + name;
}

void ProcessManager::syncProfileHotkey(const QString& name)
{
    const QString bind = m_profileBinds.value(name);
    if (!m_hotkeys->setHotkey(profileHotkeyId(name), bind)
        && !bind.isEmpty()) {
        emit errorOccurred(QString(),
                           tr("Could not register profile hotkey \"%1\"").arg(bind));
    }
}

bool ProcessManager::isLink(const QString& path)
{
    // Схема минимум из двух букв — чтобы "C:/…" не считалось ссылкой.
    static const QRegularExpression re(QStringLiteral("^[A-Za-z][A-Za-z0-9+.-]+://"));
    return re.match(path).hasMatch();
}

bool ProcessManager::isOpenOnly(const ProcessEntry& e)
{
    return e.kind == QLatin1String("folder") || e.kind == QLatin1String("link");
}

QString ProcessManager::addProgram(const QString& path, const QString& bind)
{
    QString realPath = path;

    // Если перетащили/выбрали ярлык — тихо подменяем на настоящий путь к .exe.
    QString shortcutName, shortcutIcon;
    if (ShortcutResolver::isShortcut(path)) {
        shortcutName = QFileInfo(path).completeBaseName();
        shortcutIcon = ShortcutResolver::iconFile(path);
        bool ok = false;
        const QString resolved = ShortcutResolver::resolve(path, &ok);
        if (ok && !resolved.isEmpty()) {
            realPath = resolved;
        } else {
            emit errorOccurred(QString(),
                tr("Could not resolve shortcut: %1").arg(QFileInfo(path).fileName()));
            return QString();
        }
    }

    ProcessEntry e;
    if (isLink(realPath.trimmed())) {
        e.path = realPath.trimmed();
        e.kind = QStringLiteral("link");
    } else {
        const QFileInfo info(realPath);
        if (realPath.isEmpty() || !info.exists())
            return QString();
        e.path = info.absoluteFilePath();
        if (info.isDir())
            e.kind = QStringLiteral("folder");
    }

    e.id   = QUuid::createUuid().toString(QUuid::WithoutBraces);
    // У ссылки (steam://rungameid/730) осмысленного имени нет — берём имя
    // ярлыка («Counter-Strike») и его иконку.
    if (e.kind == QLatin1String("link") && !shortcutName.isEmpty())
        e.customName = shortcutName;
    e.iconPath = shortcutIcon;
    e.name = effectiveName(e);
    e.bind = bind;
    e.profile = m_profiles.isEmpty() ? QStringLiteral("Default")
                                     : m_profiles.first();
    e.order = 0;
    for (const ProcessEntry& other : std::as_const(m_entries))
        e.order = qMax(e.order, other.order + 1);

    m_entries.insert(e.id, e);
    syncHotkey(e.id);
    save();
    emit listChanged();
    if (e.kind == QLatin1String("link") && e.iconPath.isEmpty())
        fetchLinkIcon(e.id, false);
    return e.id;
}

void ProcessManager::removeProgram(const QString& id)
{
    auto it = m_entries.find(id);
    if (it == m_entries.end())
        return;

    if (m_jobs.contains(id))
        stop(id);
    m_hotkeys->clearHotkey(id);
    m_entries.erase(it);
    if (m_stats.remove(id)) {
        m_statsDirty = true;
        saveStats();
    }
    save();
    emit listChanged();
}

void ProcessManager::setBind(const QString& id, const QString& bind)
{
    auto it = m_entries.find(id);
    if (it == m_entries.end())
        return;

    QString normalized = bind.trimmed();
    if (!normalized.isEmpty() && !HotkeyManager::isValidBind(normalized)) {
        emit errorOccurred(id, tr("Invalid hotkey: %1").arg(normalized));
        normalized.clear();
    }
    if (it->bind == normalized)
        return;

    it->bind = normalized;
    syncHotkey(id);
    save();
    emit listChanged();
}

void ProcessManager::setCustomName(const QString& id, const QString& name)
{
    auto it = m_entries.find(id);
    if (it == m_entries.end())
        return;

    QString n = name.trimmed();
    if (n == defaultName(it->path))
        n.clear();
    if (it->customName == n)
        return;

    it->customName = n;
    it->name = effectiveName(*it);
    save();
    emit listChanged();
}

void ProcessManager::setWorkDir(const QString& id, const QString& dir)
{
    auto it = m_entries.find(id);
    if (it == m_entries.end())
        return;

    QString d = dir.trimmed();
    if (d.startsWith(QStringLiteral("file:")))
        d = QUrl(d).toLocalFile();
    if (!d.isEmpty()) {
        d = QDir::cleanPath(d);
        if (d == QDir::cleanPath(QFileInfo(it->path).absolutePath()))
            d.clear();
        else
            d = QDir::toNativeSeparators(d);
    }
    if (it->workDir == d)
        return;

    it->workDir = d;
    save();
    emit listChanged();
}

void ProcessManager::setRunAsAdmin(const QString& id, bool on)
{
    auto it = m_entries.find(id);
    if (it == m_entries.end() || it->runAsAdmin == on)
        return;

    it->runAsAdmin = on;
    save();
    emit listChanged();
}

void ProcessManager::setIcon(const QString& id, const QString& iconPath)
{
    auto it = m_entries.find(id);
    if (it == m_entries.end())
        return;

    QString p = iconPath.trimmed();
    if (p.startsWith(QStringLiteral("file:")))
        p = QUrl(p).toLocalFile();
    if (!p.isEmpty())
        p = QFileInfo(p).absoluteFilePath();
    if (it->iconPath == p)
        return;

    it->iconPath = p;
    save();
    emit listChanged();
}

void ProcessManager::fetchLinkIcon(const QString& id, bool manual)
{
    const auto it = m_entries.constFind(id);
    if (it == m_entries.constEnd() || it->kind != QLatin1String("link"))
        return;
    if (!LinkIconFetcher::canFetch(it->path)) {
        if (manual)
            emit errorOccurred(id, tr("Icons can only be found for websites and Steam games"));
        return;
    }
    if (manual)
        m_manualIconFetch.insert(id);
    m_iconFetcher->fetch(id, it->path);
}

void ProcessManager::setStartDelay(const QString& id, int seconds)
{
    auto it = m_entries.find(id);
    const int s = qBound(0, seconds, 3600);
    if (it == m_entries.end() || it->startDelaySec == s)
        return;

    it->startDelaySec = s;
    save();
    emit listChanged();
}

void ProcessManager::setArgs(const QString& id, const QString& args)
{
    auto it = m_entries.find(id);
    if (it == m_entries.end() || it->args == args)
        return;

    it->args = args;
    save();
    emit listChanged();
}

void ProcessManager::setWindowMode(const QString& id, const QString& mode)
{
    auto it = m_entries.find(id);
    if (it == m_entries.end() || !isValidWindowMode(mode) || it->windowMode == mode)
        return;

    it->windowMode = mode;
    save();
    emit listChanged();
}

void ProcessManager::setEnv(const QString& id, const QString& env)
{
    auto it = m_entries.find(id);
    const QString text = env.trimmed();
    if (it == m_entries.end() || it->env == text)
        return;

    it->env = text;
    save();
    emit listChanged();
}

void ProcessManager::setNote(const QString& id, const QString& note)
{
    auto it = m_entries.find(id);
    if (it == m_entries.end() || it->note == note)
        return;

    it->note = note;
    save();
    emit listChanged();
}

void ProcessManager::start(const QString& id)
{
    auto it = m_entries.find(id);
    if (it == m_entries.end())
        return;
    if (m_jobs.contains(id))
        return;
    if (const auto ut = m_untracked.constFind(id); ut != m_untracked.constEnd()) {
        if (JobController::isPidAlive(ut.value()))
            return;
        m_untracked.remove(id);
    }

    // Программа уже открыта (сама или не через лаунчер) — вторую копию не
    // запускаем, а начинаем следить за этой.
    if (adoptRunning({id}))
        return;

    if (it->kind == QLatin1String("link")) {
        if (!QDesktopServices::openUrl(QUrl(it->path)))
            emit errorOccurred(id, tr("%1: could not open link — %2").arg(it->name, it->path));
        return;
    }

    if (!QFileInfo::exists(it->path)) {
        emit errorOccurred(id, tr("%1: not found — %2").arg(it->name, it->path));
        return;
    }

    if (it->kind == QLatin1String("folder")) {
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(it->path)))
            emit errorOccurred(id, tr("%1: could not open folder").arg(it->name));
        return;
    }

    QString workDir = QFileInfo(it->path).absolutePath();
    if (!it->workDir.isEmpty()) {
        if (!QFileInfo(it->workDir).isDir()) {
            emit errorOccurred(id, tr("%1: working folder not found — %2")
                                       .arg(it->name, it->workDir));
            return;
        }
        workDir = it->workDir;
    }

#ifdef Q_OS_WIN
    if (it->runAsAdmin) {
        startElevated(id, workDir);
        return;
    }
#endif

    QProcess proc;
    proc.setWorkingDirectory(workDir);
    if (!it->env.isEmpty())
        proc.setProcessEnvironment(buildEnvironment(it->env));
#ifdef Q_OS_WIN
    if (!it->windowMode.isEmpty()) {
        const WORD show = showCommand(it->windowMode);
        proc.setCreateProcessArgumentsModifier(
            [show](QProcess::CreateProcessArguments* a) {
                a->startupInfo->dwFlags |= STARTF_USESHOWWINDOW;
                a->startupInfo->wShowWindow = show;
            });
    }
#endif
    const QString ext = suffixOf(it->path);
    const QString nativePath = QDir::toNativeSeparators(it->path);
    const QStringList userArgs = it->args.trimmed().isEmpty()
                                     ? QStringList() : QProcess::splitCommand(it->args);
#ifdef Q_OS_WIN
    if (ext == QLatin1String("bat") || ext == QLatin1String("cmd")) {
        // cmd /c ""путь" аргументы" — внешние кавычки cmd снимает сам,
        // так путь с пробелами и аргументы доходят как есть.
        proc.setProgram(QStringLiteral("cmd.exe"));
        proc.setNativeArguments(QStringLiteral("/c \"\"%1\" %2\"")
                                    .arg(nativePath, it->args.trimmed()));
    } else if (ext == QLatin1String("ps1")) {
        proc.setProgram(QStringLiteral("powershell.exe"));
        proc.setArguments(QStringList{QStringLiteral("-NoProfile"),
                                      QStringLiteral("-ExecutionPolicy"),
                                      QStringLiteral("Bypass"),
                                      QStringLiteral("-File"), nativePath} + userArgs);
    } else
#else
    if (ext == QLatin1String("sh")) {
        proc.setProgram(QStringLiteral("/bin/sh"));
        proc.setArguments(QStringList{it->path} + userArgs);
    } else
#endif
    {
        proc.setProgram(it->path);
        proc.setArguments(userArgs);
    }

    qint64 pid = 0;
    if (!proc.startDetached(&pid) || pid <= 0) {
        emit errorOccurred(id, proc.errorString().isEmpty()
                                   ? tr("Failed to start")
                                   : proc.errorString());
        return;
    }

    auto* job = new JobController();
    job->setKillOnClose(m_killOnExit);
    if (job->assign(pid)) {
        m_jobs.insert(id, job);
    } else {
        // В job поместить не вышло (чаще всего — программа с повышением прав).
        // Остановить её мы не сможем, но за тем, жива ли она, следим по PID.
        delete job;
        m_untracked.insert(id, pid);
    }

    m_startTimes.insert(id, QDateTime::currentDateTime());
    updatePollTimer();
    emit runStateChanged(id, true);
}

void ProcessManager::updatePollTimer()
{
    const bool needed = !m_jobs.isEmpty() || !m_untracked.isEmpty();
    if (needed && !m_pollTimer->isActive())
        m_pollTimer->start();
    else if (!needed && m_pollTimer->isActive())
        m_pollTimer->stop();
}

#ifdef Q_OS_WIN
void ProcessManager::startElevated(const QString& id, const QString& workDir)
{
    const ProcessEntry e = m_entries.value(id);
    const QString nativePath = QDir::toNativeSeparators(e.path);
    const QString ext = suffixOf(e.path);
    QString program = nativePath;
    QString paramStr = e.args.trimmed();
    if (ext == QLatin1String("bat") || ext == QLatin1String("cmd")) {
        program = QStringLiteral("cmd.exe");
        paramStr = QStringLiteral("/c \"\"%1\" %2\"").arg(nativePath, e.args.trimmed());
    } else if (ext == QLatin1String("ps1")) {
        program = QStringLiteral("powershell.exe");
        paramStr = QStringLiteral("-NoProfile -ExecutionPolicy Bypass -File \"%1\" %2")
                       .arg(nativePath, e.args.trimmed()).trimmed();
    }
    const std::wstring file = program.toStdWString();
    const std::wstring params = paramStr.toStdWString();
    const std::wstring dir = QDir::toNativeSeparators(workDir).toStdWString();

    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    sei.lpVerb = L"runas";
    sei.lpFile = file.c_str();
    sei.lpParameters = params.empty() ? nullptr : params.c_str();
    sei.lpDirectory = dir.c_str();
    sei.nShow = showCommand(e.windowMode);

    // Вызов ждёт, пока пользователь ответит в окне UAC.
    if (!ShellExecuteExW(&sei) || !sei.hProcess) {
        const DWORD err = GetLastError();
        emit errorOccurred(id, err == ERROR_CANCELLED
            ? tr("%1: launch as administrator was cancelled").arg(e.name)
            : tr("%1: failed to start as administrator").arg(e.name));
        return;
    }

    auto* job = new JobController();
    job->setKillOnClose(m_killOnExit);
    job->adoptProcessHandle(sei.hProcess);
    m_jobs.insert(id, job);

    m_startTimes.insert(id, QDateTime::currentDateTime());
    updatePollTimer();
    emit runStateChanged(id, true);
}
#endif

void ProcessManager::stop(const QString& id)
{
    // Остановка отменяет и ещё не наступивший отложенный запуск.
    cancelDelayedStart(id);
    m_launchQueue.removeAll(id);

    bool wasRunning = false;

    if (auto jt = m_jobs.find(id); jt != m_jobs.end()) {
        jt.value()->terminate();
        delete jt.value();
        m_jobs.erase(jt);
        wasRunning = true;
    }
    if (const auto ut = m_untracked.constFind(id); ut != m_untracked.constEnd()) {
        if (JobController::isPidAlive(ut.value())
            && !JobController::terminatePid(ut.value())) {
            // Процесс с правами администратора, к которому у нас нет доступа:
            // честно говорим об этом и продолжаем следить за ним.
            emit errorOccurred(id, tr("Can't stop %1: it runs with administrator "
                                      "rights. Close it yourself.")
                                       .arg(m_entries.value(id).name));
            return;
        }
        m_untracked.remove(id);
        wasRunning = true;
    }

    accountTime();
    m_startTimes.remove(id);
    m_countedAt.remove(id);
    updatePollTimer();

    if (wasRunning)
        emit runStateChanged(id, false);
}

void ProcessManager::stopAll()
{
    const QList<QString> ids = m_entries.keys();
    for (const QString& id : ids)
        stop(id);
}

void ProcessManager::restart(const QString& id)
{
    if (!m_entries.contains(id))
        return;
    stop(id);
    QTimer::singleShot(400, this, [this, id]() { start(id); });
}

void ProcessManager::setWatch(const QString& id, bool on)
{
    auto it = m_entries.find(id);
    if (it == m_entries.end() || it->watch == on)
        return;
    it->watch = on;
    save();
    emit listChanged();
}

bool ProcessManager::watch(const QString& id) const
{
    const auto it = m_entries.constFind(id);
    return it != m_entries.constEnd() && it->watch;
}

int ProcessManager::launchDelayMs() const
{
    return m_launchDelayMs;
}

void ProcessManager::setLaunchDelayMs(int ms)
{
    if (ms < 0) ms = 0;
    if (m_launchDelayMs == ms)
        return;
    m_launchDelayMs = ms;
    save();
    emit profilesChanged();
}

int ProcessManager::pollInterval() const
{
    return m_pollTimer->interval();
}

void ProcessManager::setPollInterval(int ms)
{
    if (ms < 200) ms = 200;
    if (ms > 10000) ms = 10000;
    m_pollTimer->setInterval(ms);
}

QString ProcessManager::autoStartProfile() const
{
    return m_autoStartProfile;
}

void ProcessManager::setAutoStartProfile(const QString& name)
{
    if (m_autoStartProfile == name)
        return;
    m_autoStartProfile = name;
    save();
    emit profilesChanged();
}

bool ProcessManager::isRunning(const QString& id) const
{
    if (const auto ut = m_untracked.constFind(id); ut != m_untracked.constEnd())
        return JobController::isPidAlive(ut.value());
    const auto it = m_jobs.constFind(id);
    return it != m_jobs.constEnd() && it.value()->isAlive();
}

int ProcessManager::uptimeSeconds(const QString& id) const
{
    const auto it = m_startTimes.constFind(id);
    if (it == m_startTimes.constEnd() || !isRunning(id))
        return 0;
    const qint64 secs = it->secsTo(QDateTime::currentDateTime());
    return secs > 0 ? static_cast<int>(secs) : 0;
}

int ProcessManager::profileRunningCount(const QString& name) const
{
    int n = 0;
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        if (it->profile == name && isRunning(it->id))
            ++n;
    }
    return n;
}

int ProcessManager::profileTotalCount(const QString& name) const
{
    int n = 0;
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        if (it->profile == name)
            ++n;
    }
    return n;
}

QString ProcessManager::programName(const QString& id) const
{
    return m_entries.value(id).name;
}

void ProcessManager::renumber(const QStringList& ids)
{
    int n = 0;
    for (const QString& id : ids) {
        auto it = m_entries.find(id);
        if (it != m_entries.end())
            it->order = n++;
    }
}

void ProcessManager::moveProgram(const QString& id, const QString& targetId, bool after)
{
    if (id == targetId || !m_entries.contains(id) || !m_entries.contains(targetId))
        return;

    QStringList ids;
    for (const ProcessEntry& e : entries())
        ids.append(e.id);
    ids.removeOne(id);
    const int at = ids.indexOf(targetId) + (after ? 1 : 0);
    ids.insert(at, id);

    renumber(ids);
    save();
    emit listChanged();
}

QString ProcessManager::duplicateProgram(const QString& id)
{
    if (!m_entries.contains(id))
        return QString();

    ProcessEntry copy = m_entries.value(id);
    copy.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    copy.customName = tr("%1 (copy)").arg(copy.name);
    copy.name = copy.customName;
    copy.bind.clear();   // одна клавиша не может запускать две программы

    QStringList ids;
    for (const ProcessEntry& e : entries())
        ids.append(e.id);
    ids.insert(ids.indexOf(id) + 1, copy.id);

    m_entries.insert(copy.id, copy);
    renumber(ids);
    save();
    emit listChanged();
    return copy.id;
}

QList<ProcessEntry> ProcessManager::entries() const
{
    QList<ProcessEntry> result = m_entries.values();
    std::sort(result.begin(), result.end(),
              [](const ProcessEntry& a, const ProcessEntry& b) {
                  if (a.order != b.order)
                      return a.order < b.order;
                  if (a.name != b.name)
                      return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
                  return a.id < b.id;
              });
    return result;
}

void ProcessManager::pollJobs()
{
    accountTime();

    QList<QString> finished;
    for (auto it = m_jobs.constBegin(); it != m_jobs.constEnd(); ++it) {
        if (!it.value()->isAlive())
            finished.append(it.key());
    }
    for (auto it = m_untracked.constBegin(); it != m_untracked.constEnd(); ++it) {
        if (!JobController::isPidAlive(it.value()))
            finished.append(it.key());
    }

    for (const QString& id : finished)
        handleExit(id);

    sampleUsage();
    updatePollTimer();
    emit tick();
}

namespace {
// Скрипты работают внутри cmd/powershell/sh — по пути их процесс не найти.
bool canDetect(const ProcessEntry& e)
{
    if (e.kind != QLatin1String("program"))
        return false;
    const QString ext = QFileInfo(e.path).suffix().toLower();
    return ext != QLatin1String("bat") && ext != QLatin1String("cmd")
        && ext != QLatin1String("ps1") && ext != QLatin1String("sh");
}

// Тот ли это процесс, что у программы в списке.
bool sameProgram(const QString& entryPath, const QString& procPath)
{
#ifdef Q_OS_WIN
    constexpr Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#else
    constexpr Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif
    const QString entry = QDir::cleanPath(entryPath);
    const QString proc = QDir::cleanPath(procPath);
    if (proc.compare(entry, cs) == 0)
        return true;
    // macOS: в списке бандл Foo.app, а работает Foo.app/Contents/MacOS/Foo.
    if (proc.startsWith(entry + QLatin1Char('/'), cs))
        return true;
    // Программы, которые запускают себя из подпапки версии (Discord:
    // Discord/app-1.0.9/Discord.exe) — тот же файл где-то внутри её папки.
    const QFileInfo e(entry);
    return QFileInfo(proc).fileName().compare(e.fileName(), cs) == 0
        && proc.startsWith(e.absolutePath() + QLatin1Char('/'), cs);
}
}

bool ProcessManager::adoptRunning(const QStringList& ids)
{
    QStringList candidates;
    QSet<QString> names;
    for (const QString& id : ids) {
        const auto it = m_entries.constFind(id);
        if (it == m_entries.constEnd() || !canDetect(*it) || isRunning(id))
            continue;
        candidates.append(id);
        names.insert(QFileInfo(it->path).fileName().toLower());
    }
    if (candidates.isEmpty())
        return false;

    const QList<JobController::ProcessInfo> procs = JobController::runningProcesses(names);
    if (procs.isEmpty())
        return false;

    // Процессы, за которыми уже следим, второй раз не берём: иначе копия
    // программы с другими аргументами «подхватила» бы уже запущенную.
    QSet<qint64> claimed;
    for (const JobController* job : std::as_const(m_jobs))
        for (qint64 pid : job->pids())
            claimed.insert(pid);
    for (qint64 pid : std::as_const(m_untracked))
        claimed.insert(pid);

    bool adopted = false;
    for (const QString& id : std::as_const(candidates)) {
        const QString path = m_entries.value(id).path;
        for (const JobController::ProcessInfo& p : procs) {
            if (claimed.contains(p.pid) || !sameProgram(path, p.path))
                continue;
            claimed.insert(p.pid);
            m_untracked.insert(id, p.pid);
            const QDateTime started = JobController::startTime(p.pid);
            m_startTimes.insert(id, started.isValid() ? started : QDateTime::currentDateTime());
            updatePollTimer();
            emit runStateChanged(id, true);
            adopted = true;
            break;
        }
    }
    return adopted;
}

void ProcessManager::scanExternal()
{
    QStringList ids;
    for (const ProcessEntry& e : entries()) {
        // Ждущие отложенного запуска сейчас стартуют сами.
        if (!m_delayedStarts.contains(e.id) && !m_launchQueue.contains(e.id))
            ids.append(e.id);
    }
    adoptRunning(ids);
}

void ProcessManager::sampleUsage()
{
    const qint64 now = m_clock.elapsed();
    const int cores = qMax(1, QThread::idealThreadCount());

    auto update = [&](const QString& id, const JobController::Usage& u) {
        UsageSample& s = m_usage[id];
        // Чаще раза в секунду не меряем: на коротком отрезке процент скачет.
        if (s.atMs >= 0 && now - s.atMs < 900)
            return;
        if (u.cpuNs >= 0 && s.cpuNs >= 0 && u.cpuNs >= s.cpuNs && now > s.atMs) {
            // Как в Диспетчере задач: 100 % — все ядра заняты полностью.
            const double busy = double(u.cpuNs - s.cpuNs) / (double(now - s.atMs) * 1e6);
            s.cpuPercent = qBound(0.0, busy / cores * 100.0, 100.0);
        }
        s.cpuNs = u.cpuNs;
        s.atMs = now;
        s.memBytes = u.memBytes;
    };

    for (auto it = m_jobs.constBegin(); it != m_jobs.constEnd(); ++it)
        update(it.key(), it.value()->usage());
    for (auto it = m_untracked.constBegin(); it != m_untracked.constEnd(); ++it)
        if (!m_jobs.contains(it.key()))
            update(it.key(), JobController::pidUsage(it.value()));
}

double ProcessManager::cpuPercent(const QString& id) const
{
    return m_usage.value(id).cpuPercent;
}

qint64 ProcessManager::memoryBytes(const QString& id) const
{
    return m_usage.value(id).memBytes;
}

namespace {
constexpr int kKeepDays = 400;             // столько дней храним по отдельности
constexpr qint64 kMaxGapMs = 5 * 60 * 1000; // больший разрыв — компьютер спал
}

void ProcessManager::accountTime()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (auto it = m_startTimes.constBegin(); it != m_startTimes.constEnd(); ++it) {
        const QString& id = it.key();
        auto c = m_countedAt.find(id);
        if (c == m_countedAt.end()) {
            // Запущена лаунчером — считаем с запуска; подхваченную со стороны —
            // с момента, когда её увидели (раньше лаунчер мог быть закрыт).
            const qint64 started = it.value().toMSecsSinceEpoch();
            const bool fresh = m_jobs.contains(id) && now - started < kMaxGapMs;
            m_countedAt.insert(id, now);
            if (fresh)
                credit(id, started, now);
            continue;
        }
        const qint64 from = c.value();
        c.value() = now;
        if (now > from && now - from <= kMaxGapMs)
            credit(id, from, now);
    }

    if (m_statsDirty && now - m_statsSavedAt >= 60 * 1000)
        saveStats();
}

void ProcessManager::credit(const QString& id, qint64 fromMs, qint64 toMs)
{
    RunStats& s = m_stats[id];
    QDateTime from = QDateTime::fromMSecsSinceEpoch(fromMs);
    const QDateTime to = QDateTime::fromMSecsSinceEpoch(toMs);
    // Время через полночь делим между днями.
    while (from.date() < to.date()) {
        const QDateTime midnight(from.date().addDays(1), QTime(0, 0));
        s.days[from.date().toString(Qt::ISODate)] += from.msecsTo(midnight);
        from = midnight;
    }
    s.days[from.date().toString(Qt::ISODate)] += from.msecsTo(to);
    m_statsDirty = true;
}

qint64 ProcessManager::runSeconds(const QString& id, const QString& period) const
{
    const auto it = m_stats.constFind(id);
    qint64 ms = 0;
    if (it != m_stats.constEnd()) {
        const QDate today = QDate::currentDate();
        if (period == QLatin1String("today")) {
            ms = it->days.value(today.toString(Qt::ISODate));
        } else if (period == QLatin1String("week")) {
            for (int i = 0; i < 7; ++i)
                ms += it->days.value(today.addDays(-i).toString(Qt::ISODate));
        } else {
            ms = it->older;
            for (qint64 v : it->days)
                ms += v;
        }
    }
    return ms / 1000;
}

void ProcessManager::clearStatistics()
{
    m_stats.clear();
    // Работающие сейчас продолжают считаться с этого момента.
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (auto it = m_countedAt.begin(); it != m_countedAt.end(); ++it)
        it.value() = now;
    m_statsDirty = true;
    saveStats();
}

QString ProcessManager::statsFilePath() const
{
    return AppPaths::configFile(QStringLiteral("stats.json"));
}

void ProcessManager::loadStats()
{
    QFile file(statsFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    const QDate oldest = QDate::currentDate().addDays(-kKeepDays);
    bool pruned = false;
    for (auto it = root.constBegin(); it != root.constEnd(); ++it) {
        const QJsonObject obj = it.value().toObject();
        RunStats s;
        s.older = qint64(obj.value(QStringLiteral("older")).toDouble() * 1000);
        const QJsonObject days = obj.value(QStringLiteral("days")).toObject();
        for (auto d = days.constBegin(); d != days.constEnd(); ++d) {
            const QDate date = QDate::fromString(d.key(), Qt::ISODate);
            const qint64 ms = qint64(d.value().toDouble() * 1000);
            if (!date.isValid() || ms <= 0)
                continue;
            if (date < oldest) {
                s.older += ms;
                pruned = true;
            } else {
                s.days.insert(d.key(), ms);
            }
        }
        if (s.older > 0 || !s.days.isEmpty())
            m_stats.insert(it.key(), s);
    }
    m_statsDirty = pruned;
}

void ProcessManager::saveStats()
{
    if (!m_statsDirty)
        return;
    QJsonObject root;
    for (auto it = m_stats.constBegin(); it != m_stats.constEnd(); ++it) {
        if (!m_entries.contains(it.key()))
            continue;
        QJsonObject days;
        for (auto d = it->days.constBegin(); d != it->days.constEnd(); ++d)
            days.insert(d.key(), double(d.value() / 1000));
        QJsonObject obj;
        obj.insert(QStringLiteral("days"), days);
        if (it->older > 0)
            obj.insert(QStringLiteral("older"), double(it->older / 1000));
        root.insert(it.key(), obj);
    }
    QSaveFile file(statsFilePath());
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    if (file.commit()) {
        m_statsDirty = false;
        m_statsSavedAt = QDateTime::currentMSecsSinceEpoch();
    }
}

void ProcessManager::handleExit(const QString& id)
{
    if (auto jt = m_jobs.find(id); jt != m_jobs.end()) {
        delete jt.value();
        m_jobs.erase(jt);
    }
    m_untracked.remove(id);
    m_startTimes.remove(id);
    m_countedAt.remove(id);
    m_usage.remove(id);

    const auto e = m_entries.constFind(id);
    const bool shouldWatch = (e != m_entries.constEnd() && e->watch);
    QString exitedName, exitedProfile;
    if (e != m_entries.constEnd()) {
        exitedName = e->name;
        exitedProfile = e->profile;
    }

    emit runStateChanged(id, false);

    if (shouldWatch) {
        emit processExited(exitedName,
                           tr("%1 — restarting").arg(exitedProfile));
        QTimer::singleShot(500, this, [this, id]() {
            if (m_entries.contains(id) && watch(id) && !isRunning(id))
                start(id);
        });
    } else {
        emit processExited(exitedName, exitedProfile);
    }
}

void ProcessManager::resetState()
{
    m_hotkeys->clearAll();
    m_launchQueue.clear();
    for (QTimer* t : std::as_const(m_delayedStarts))
        t->deleteLater();
    m_delayedStarts.clear();
    m_launchTimer->stop();

    m_entries.clear();
    m_profiles.clear();
    m_profileBinds.clear();
    m_profileColors.clear();
    m_profileIcons.clear();
    m_extraRoot = QJsonObject();
    m_launchDelayMs = 0;
    m_autoStartProfile.clear();
    m_searchBind = kDefaultSearchBind;
    m_hotkeys->clearHotkey(kSearchHotkeyId);
}

void ProcessManager::syncHotkey(const QString& id)
{
    const auto it = m_entries.constFind(id);
    if (it == m_entries.constEnd())
        return;

    if (!m_hotkeys->setHotkey(id, it->bind) && !it->bind.trimmed().isEmpty()) {
        emit errorOccurred(id, tr("Could not register hotkey \"%1\"")
                                   .arg(it->bind));
    }
}

void ProcessManager::save() const
{
    QJsonArray programs;
    QList<QString> ids = m_entries.keys();
    std::sort(ids.begin(), ids.end());
    for (const QString& id : ids) {
        const ProcessEntry& e = m_entries.value(id);
        QJsonObject obj = e.extra;
        obj.insert(QStringLiteral("id"),      e.id);
        obj.insert(QStringLiteral("path"),    e.path);
        obj.insert(QStringLiteral("bind"),    e.bind);
        obj.insert(QStringLiteral("args"),    e.args);
        obj.insert(QStringLiteral("note"),    e.note);
        obj.insert(QStringLiteral("profile"), e.profile);
        obj.insert(QStringLiteral("watch"),   e.watch);
        obj.insert(QStringLiteral("order"),   e.order);
        // Необязательные поля пишем только когда они заданы — конфиг
        // остаётся читаемым и совместимым со старыми версиями.
        if (e.kind != QLatin1String("program"))
            obj.insert(QStringLiteral("kind"), e.kind);
        if (!e.customName.isEmpty())
            obj.insert(QStringLiteral("name"), e.customName);
        if (!e.workDir.isEmpty())
            obj.insert(QStringLiteral("workDir"), e.workDir);
        if (e.runAsAdmin)
            obj.insert(QStringLiteral("admin"), true);
        if (e.startDelaySec > 0)
            obj.insert(QStringLiteral("delay"), e.startDelaySec);
        if (!e.iconPath.isEmpty())
            obj.insert(QStringLiteral("icon"), e.iconPath);
        if (!e.windowMode.isEmpty())
            obj.insert(QStringLiteral("window"), e.windowMode);
        if (!e.env.isEmpty())
            obj.insert(QStringLiteral("env"), e.env);
        programs.append(obj);
    }

    QJsonArray profilesArr;
    for (const QString& p : m_profiles)
        profilesArr.append(p);

    QJsonObject profileBindsObj;
    for (auto it = m_profileBinds.constBegin(); it != m_profileBinds.constEnd(); ++it)
        profileBindsObj.insert(it.key(), it.value());

    QJsonObject profileColorsObj;
    for (auto it = m_profileColors.constBegin(); it != m_profileColors.constEnd(); ++it)
        profileColorsObj.insert(it.key(), it.value());

    QJsonObject profileIconsObj;
    for (auto it = m_profileIcons.constBegin(); it != m_profileIcons.constEnd(); ++it)
        profileIconsObj.insert(it.key(), it.value());

    QJsonObject root = m_extraRoot;
    root.insert(QStringLiteral("programs"),      programs);
    root.insert(QStringLiteral("profiles"),      profilesArr);
    root.insert(QStringLiteral("profileBinds"),  profileBindsObj);
    root.insert(QStringLiteral("profileColors"), profileColorsObj);
    root.insert(QStringLiteral("profileIcons"),  profileIconsObj);
    root.insert(QStringLiteral("launchDelayMs"), m_launchDelayMs);
    root.insert(QStringLiteral("autoStartProfile"), m_autoStartProfile);
    root.insert(QStringLiteral("searchBind"), m_searchBind);

    // QSaveFile пишет во временный файл и подменяет им настоящий только при
    // успехе: сбой или выключение посреди записи не оставит обрезанный конфиг.
    QSaveFile file(configFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("ProcessManager: cannot write config: %s",
                 qPrintable(file.errorString()));
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit())
        qWarning("ProcessManager: cannot write config: %s",
                 qPrintable(file.errorString()));
}

void ProcessManager::load()
{
    QFile file(configFilePath());
    if (!file.exists()) {
        if (m_profiles.isEmpty())
            m_profiles.append(QStringLiteral("Default"));
        return;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("ProcessManager: cannot read config: %s",
                 qPrintable(file.errorString()));
        if (m_profiles.isEmpty())
            m_profiles.append(QStringLiteral("Default"));
        return;
    }

    const QByteArray raw = file.readAll();
    file.close();
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);
    if (doc.isNull()) {
        qWarning("ProcessManager: invalid config JSON: %s",
                 qPrintable(parseError.errorString()));
        // Повреждённый файл не выбрасываем: откладываем копию, чтобы следующее
        // сохранение не затёрло то, что ещё можно восстановить руками.
        const QString backup = configFilePath() + QStringLiteral(".broken");
        QFile::remove(backup);
        QFile::copy(configFilePath(), backup);
        if (m_profiles.isEmpty())
            m_profiles.append(QStringLiteral("Default"));
        return;
    }

    QJsonArray programs;
    if (doc.isArray()) {
        programs = doc.array();
    } else if (doc.isObject()) {
        const QJsonObject root = doc.object();
        programs = root.value(QStringLiteral("programs")).toArray();

        m_extraRoot = root;
        for (const char* key : {"programs", "profiles", "profileBinds", "profileColors",
                                "profileIcons", "launchDelayMs", "autoStartProfile",
                                "searchBind"})
            m_extraRoot.remove(QLatin1String(key));

        const QJsonArray profilesArr = root.value(QStringLiteral("profiles")).toArray();
        for (const QJsonValue& pv : profilesArr) {
            const QString p = pv.toString().trimmed();
            if (!p.isEmpty() && !m_profiles.contains(p))
                m_profiles.append(p);
        }

        const QJsonObject binds = root.value(QStringLiteral("profileBinds")).toObject();
        for (auto it = binds.constBegin(); it != binds.constEnd(); ++it) {
            const QString b = it.value().toString();
            if (!b.isEmpty() && HotkeyManager::isValidBind(b))
                m_profileBinds.insert(it.key(), b);
        }

        m_launchDelayMs = root.value(QStringLiteral("launchDelayMs")).toInt(0);
        m_autoStartProfile = root.value(QStringLiteral("autoStartProfile")).toString();
        // Ключа нет — конфиг от версии без быстрого поиска: остаётся клавиша по умолчанию.
        if (root.contains(QStringLiteral("searchBind")))
            m_searchBind = root.value(QStringLiteral("searchBind")).toString().trimmed();

        const QJsonObject colors = root.value(QStringLiteral("profileColors")).toObject();
        for (auto it = colors.constBegin(); it != colors.constEnd(); ++it)
            m_profileColors.insert(it.key(), it.value().toString());

        const QJsonObject icons = root.value(QStringLiteral("profileIcons")).toObject();
        for (auto it = icons.constBegin(); it != icons.constEnd(); ++it)
            m_profileIcons.insert(it.key(), it.value().toString());
    }

    if (m_profiles.isEmpty())
        m_profiles.append(QStringLiteral("Default"));
    if (!m_searchBind.isEmpty() && !HotkeyManager::isValidBind(m_searchBind))
        m_searchBind.clear();

    for (const QJsonValue& v : programs) {
        if (!v.isObject())
            continue;
        const QJsonObject obj = v.toObject();
        const QString path = obj.value(QStringLiteral("path")).toString();
        if (path.isEmpty())
            continue;

        ProcessEntry e;
        e.id   = obj.value(QStringLiteral("id")).toString();
        if (e.id.isEmpty())
            e.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        e.path = path;
        const QString kind = obj.value(QStringLiteral("kind")).toString();
        if (kind == QLatin1String("folder") || kind == QLatin1String("link"))
            e.kind = kind;
        e.customName = obj.value(QStringLiteral("name")).toString().trimmed();
        e.name = effectiveName(e);
        e.workDir = obj.value(QStringLiteral("workDir")).toString();
        e.runAsAdmin = obj.value(QStringLiteral("admin")).toBool(false);
        e.startDelaySec = qBound(0, obj.value(QStringLiteral("delay")).toInt(0), 3600);
        e.iconPath = obj.value(QStringLiteral("icon")).toString();
        e.bind = obj.value(QStringLiteral("bind")).toString();
        e.args = obj.value(QStringLiteral("args")).toString();
        e.note = obj.value(QStringLiteral("note")).toString();
        e.profile = obj.value(QStringLiteral("profile")).toString();
        e.watch = obj.value(QStringLiteral("watch")).toBool(false);
        e.order = obj.value(QStringLiteral("order")).toInt(-1);
        e.windowMode = obj.value(QStringLiteral("window")).toString();
        if (!isValidWindowMode(e.windowMode))
            e.windowMode.clear();
        e.env = obj.value(QStringLiteral("env")).toString().trimmed();
        if (e.profile.isEmpty() || !m_profiles.contains(e.profile))
            e.profile = m_profiles.first();
        if (!e.bind.isEmpty() && !HotkeyManager::isValidBind(e.bind))
            e.bind.clear();

        e.extra = obj;
        for (const char* key : {"id", "path", "kind", "name", "workDir", "admin", "delay",
                                "icon", "bind", "args", "note", "profile", "watch",
                                "order", "window", "env"})
            e.extra.remove(QLatin1String(key));

        m_entries.insert(e.id, e);
        syncHotkey(e.id);
    }

    for (const QString& name : m_profiles)
        syncProfileHotkey(name);

    // В конфигах до 1.7 порядка нет: программы без него встают после
    // остальных по алфавиту, затем номера идут подряд.
    {
        QList<ProcessEntry> sorted = m_entries.values();
        std::sort(sorted.begin(), sorted.end(),
                  [](const ProcessEntry& a, const ProcessEntry& b) {
                      const bool aHas = a.order >= 0, bHas = b.order >= 0;
                      if (aHas != bHas)
                          return aHas;
                      if (aHas && a.order != b.order)
                          return a.order < b.order;
                      const int c = a.name.compare(b.name, Qt::CaseInsensitive);
                      return c != 0 ? c < 0 : a.id < b.id;
                  });
        QStringList ids;
        for (const ProcessEntry& e : sorted)
            ids.append(e.id);
        renumber(ids);
    }

    // Ссылки без иконки: игры Steam берут её из локального кеша Steam, сайты —
    // из сети. Так иконки возвращаются, даже если конфиг перезаписала старая
    // версия, ничего о них не знавшая.
    for (const ProcessEntry& e : std::as_const(m_entries))
        if (e.kind == QLatin1String("link") && e.iconPath.isEmpty()
            && LinkIconFetcher::canFetch(e.path))
            fetchLinkIcon(e.id, false);

    save();
}
