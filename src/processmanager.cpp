#include "processmanager.h"
#include "jobcontroller.h"
#include "hotkeymanager.h"
#include "shortcutresolver.h"

#include <QProcess>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <algorithm>

namespace {
constexpr int kPollIntervalMs = 1200;
const QString kProfilePrefix = QStringLiteral("profile:");
}

ProcessManager::ProcessManager(QObject* parent)
    : QObject(parent)
    , m_pollTimer(new QTimer(this))
    , m_hotkeys(new HotkeyManager(this))
{
    m_pollTimer->setInterval(kPollIntervalMs);
    connect(m_pollTimer, &QTimer::timeout, this, &ProcessManager::pollJobs);

    connect(m_hotkeys, &HotkeyManager::activated,
            this, &ProcessManager::onHotkeyActivated);

    load();
}

ProcessManager::~ProcessManager()
{

    qDeleteAll(m_jobs);
    m_jobs.clear();
}

QString ProcessManager::configFilePath() const
{
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/programs.json");
}

void ProcessManager::onHotkeyActivated(const QString& hotkeyId)
{
    if (hotkeyId.startsWith(kProfilePrefix))
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
    m_profiles.removeAll(name);

    save();
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

    save();
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
    const QList<QString> ids = m_entries.keys();
    for (const QString& id : ids) {
        if (m_entries.value(id).profile == name)
            start(id);
    }
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

QString ProcessManager::addProgram(const QString& path, const QString& bind)
{
    QString realPath = path;

    // Если перетащили/выбрали ярлык — тихо подменяем на настоящий путь к .exe.
    if (ShortcutResolver::isShortcut(path)) {
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

    const QFileInfo info(realPath);
    if (realPath.isEmpty() || !info.exists() || !info.isFile())
        return QString();

    ProcessEntry e;
    e.id   = QUuid::createUuid().toString(QUuid::WithoutBraces);
    e.path = info.absoluteFilePath();
    e.name = info.fileName();
    e.bind = bind;
    e.profile = m_profiles.isEmpty() ? QStringLiteral("Default")
                                     : m_profiles.first();

    m_entries.insert(e.id, e);
    syncHotkey(e.id);
    save();
    emit listChanged();
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

void ProcessManager::setArgs(const QString& id, const QString& args)
{
    auto it = m_entries.find(id);
    if (it == m_entries.end() || it->args == args)
        return;

    it->args = args;
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

    QProcess proc;
    proc.setProgram(it->path);
    proc.setWorkingDirectory(QFileInfo(it->path).absolutePath());
    if (!it->args.trimmed().isEmpty())
        proc.setArguments(QProcess::splitCommand(it->args));

    qint64 pid = 0;
    if (!proc.startDetached(&pid) || pid <= 0) {
        emit errorOccurred(id, proc.errorString().isEmpty()
                                   ? tr("Failed to start")
                                   : proc.errorString());
        return;
    }

    auto* job = new JobController();
    if (job->assign(pid)) {
        m_jobs.insert(id, job);
        if (!m_pollTimer->isActive())
            m_pollTimer->start();
    } else {

        delete job;
        m_untracked.insert(id);
    }

    m_startTimes.insert(id, QDateTime::currentDateTime());
    emit runStateChanged(id, true);
}

void ProcessManager::stop(const QString& id)
{
    bool wasRunning = false;

    if (auto jt = m_jobs.find(id); jt != m_jobs.end()) {
        jt.value()->terminate();
        delete jt.value();
        m_jobs.erase(jt);
        wasRunning = true;
    }
    if (m_untracked.remove(id))
        wasRunning = true;

    m_startTimes.remove(id);

    if (m_jobs.isEmpty() && m_pollTimer->isActive())
        m_pollTimer->stop();

    if (wasRunning)
        emit runStateChanged(id, false);
}

void ProcessManager::stopAll()
{
    const QList<QString> ids = m_entries.keys();
    for (const QString& id : ids)
        stop(id);
}

bool ProcessManager::isRunning(const QString& id) const
{
    if (m_untracked.contains(id))
        return true;
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

QList<ProcessEntry> ProcessManager::entries() const
{
    QList<ProcessEntry> result = m_entries.values();
    std::sort(result.begin(), result.end(),
              [](const ProcessEntry& a, const ProcessEntry& b) {
                  if (a.name != b.name)
                      return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
                  return a.id < b.id;
              });
    return result;
}

void ProcessManager::pollJobs()
{
    QList<QString> finished;
    for (auto it = m_jobs.constBegin(); it != m_jobs.constEnd(); ++it) {
        if (!it.value()->isAlive())
            finished.append(it.key());
    }

    for (const QString& id : finished) {
        auto jt = m_jobs.find(id);
        if (jt != m_jobs.end()) {
            delete jt.value();
            m_jobs.erase(jt);
        }
        m_startTimes.remove(id);

        const auto e = m_entries.constFind(id);
        if (e != m_entries.constEnd())
            emit processExited(e->name, e->profile);

        emit runStateChanged(id, false);
    }

    if (m_jobs.isEmpty() && m_pollTimer->isActive())
        m_pollTimer->stop();

    emit tick();
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
        programs.append(QJsonObject{
            {QStringLiteral("id"),      e.id},
            {QStringLiteral("path"),    e.path},
            {QStringLiteral("bind"),    e.bind},
            {QStringLiteral("args"),    e.args},
            {QStringLiteral("profile"), e.profile},
        });
    }

    QJsonArray profilesArr;
    for (const QString& p : m_profiles)
        profilesArr.append(p);

    QJsonObject profileBindsObj;
    for (auto it = m_profileBinds.constBegin(); it != m_profileBinds.constEnd(); ++it)
        profileBindsObj.insert(it.key(), it.value());

    const QJsonObject root{
        {QStringLiteral("programs"),     programs},
        {QStringLiteral("profiles"),     profilesArr},
        {QStringLiteral("profileBinds"), profileBindsObj},
    };

    QFile file(configFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning("ProcessManager: cannot write config: %s",
                 qPrintable(file.errorString()));
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
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
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);
    if (doc.isNull()) {
        qWarning("ProcessManager: invalid config JSON: %s",
                 qPrintable(parseError.errorString()));
        return;
    }

    QJsonArray programs;
    if (doc.isArray()) {
        programs = doc.array();
    } else if (doc.isObject()) {
        const QJsonObject root = doc.object();
        programs = root.value(QStringLiteral("programs")).toArray();

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
    }

    if (m_profiles.isEmpty())
        m_profiles.append(QStringLiteral("Default"));

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
        e.name = QFileInfo(path).fileName();
        e.bind = obj.value(QStringLiteral("bind")).toString();
        e.args = obj.value(QStringLiteral("args")).toString();
        e.profile = obj.value(QStringLiteral("profile")).toString();
        if (e.profile.isEmpty() || !m_profiles.contains(e.profile))
            e.profile = m_profiles.first();
        if (!e.bind.isEmpty() && !HotkeyManager::isValidBind(e.bind))
            e.bind.clear();

        m_entries.insert(e.id, e);
        syncHotkey(e.id);
    }

    for (const QString& name : m_profiles)
        syncProfileHotkey(name);

    save();
}
