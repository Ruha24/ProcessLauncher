#include "processmanager.h"
#include "jobcontroller.h"
#include "hotkeymanager.h"

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
}

ProcessManager::ProcessManager(QObject* parent)
    : QObject(parent)
    , m_pollTimer(new QTimer(this))
    , m_hotkeys(new HotkeyManager(this))
{
    m_pollTimer->setInterval(kPollIntervalMs);
    connect(m_pollTimer, &QTimer::timeout, this, &ProcessManager::pollJobs);

    connect(m_hotkeys, &HotkeyManager::activated,
            this, &ProcessManager::start);

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

QString ProcessManager::addProgram(const QString& path, const QString& bind)
{
    const QFileInfo info(path);
    if (path.isEmpty() || !info.exists() || !info.isFile())
        return QString();

    ProcessEntry e;
    e.id   = QUuid::createUuid().toString(QUuid::WithoutBraces);
    e.path = info.absoluteFilePath();
    e.name = info.fileName();
    e.bind = bind;

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
        emit runStateChanged(id, false);
    }

    if (m_jobs.isEmpty() && m_pollTimer->isActive())
        m_pollTimer->stop();
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
    QJsonArray arr;

    QList<QString> ids = m_entries.keys();
    std::sort(ids.begin(), ids.end());
    for (const QString& id : ids) {
        const ProcessEntry& e = m_entries.value(id);
        arr.append(QJsonObject{
            {QStringLiteral("id"),   e.id},
            {QStringLiteral("path"), e.path},
            {QStringLiteral("bind"), e.bind},
        });
    }

    QFile file(configFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning("ProcessManager: cannot write config: %s",
                 qPrintable(file.errorString()));
        return;
    }
    file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
}

void ProcessManager::load()
{
    QFile file(configFilePath());
    if (!file.exists())
        return;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("ProcessManager: cannot read config: %s",
                 qPrintable(file.errorString()));
        return;
    }

    const QByteArray raw = file.readAll();
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);
    if (doc.isNull() || !doc.isArray()) {
        qWarning("ProcessManager: invalid config JSON: %s",
                 qPrintable(parseError.errorString()));
        return;
    }

    const QJsonArray arr = doc.array();
    for (const QJsonValue& v : arr) {
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
        if (!e.bind.isEmpty() && !HotkeyManager::isValidBind(e.bind))
            e.bind.clear();

        m_entries.insert(e.id, e);
        syncHotkey(e.id);
    }

    save();
}
