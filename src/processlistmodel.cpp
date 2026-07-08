#include "processlistmodel.h"

#include <QGuiApplication>
#include <QClipboard>
#include <QProcess>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>

ProcessListModel::ProcessListModel(ProcessManager* manager, QObject* parent)
    : QAbstractListModel(parent)
    , m_manager(manager)
{
    Q_ASSERT(m_manager);
    m_rows = m_manager->entries();

    connect(m_manager, &ProcessManager::listChanged,
            this, &ProcessListModel::onListChanged);
    connect(m_manager, &ProcessManager::runStateChanged,
            this, &ProcessListModel::onRunStateChanged);
    connect(m_manager, &ProcessManager::errorOccurred,
            this, &ProcessListModel::onErrorOccurred);
    connect(m_manager, &ProcessManager::profilesChanged,
            this, &ProcessListModel::profilesChanged);
    connect(m_manager, &ProcessManager::tick,
            this, &ProcessListModel::tick);
    connect(m_manager, &ProcessManager::processExited,
            this, &ProcessListModel::processExited);
}

int ProcessListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

QVariant ProcessListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return QVariant();

    const ProcessEntry& e = m_rows.at(index.row());
    switch (role) {
    case IdRole:      return e.id;
    case NameRole:    return e.name;
    case BindRole:    return e.bind;
    case PathRole:    return e.path;
    case RunningRole: return m_manager->isRunning(e.id);
    case ArgsRole:    return e.args;
    case ProfileRole: return e.profile;
    case WatchRole:   return e.watch;
    }
    return QVariant();
}

QHash<int, QByteArray> ProcessListModel::roleNames() const
{
    return {
        {IdRole,      QByteArrayLiteral("procId")},
        {NameRole,    QByteArrayLiteral("name")},
        {BindRole,    QByteArrayLiteral("bind")},
        {PathRole,    QByteArrayLiteral("path")},
        {RunningRole, QByteArrayLiteral("running")},
        {ArgsRole,    QByteArrayLiteral("args")},
        {ProfileRole, QByteArrayLiteral("profile")},
        {WatchRole,   QByteArrayLiteral("watch")},
    };
}

void ProcessListModel::addProgram(const QString& path, const QString& bind)
{
    m_manager->addProgram(path, bind);
}

void ProcessListModel::addProgramFromUrl(const QUrl& url, const QString& profile)
{
    const QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    const QString id = m_manager->addProgram(path, QString());
    if (!id.isEmpty() && !profile.isEmpty())
        m_manager->setProgramProfile(id, profile);
}

void ProcessListModel::openFileLocation(const QString& path) const
{
    const QFileInfo info(path);
    if (!info.exists())
        return;

#ifdef Q_OS_WIN
    QProcess::startDetached(QStringLiteral("explorer.exe"),
                            {QStringLiteral("/select,"),
                             QDir::toNativeSeparators(info.absoluteFilePath())});
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()));
#endif
}

void ProcessListModel::copyPath(const QString& path) const
{
    if (QClipboard* cb = QGuiApplication::clipboard())
        cb->setText(QDir::toNativeSeparators(path));
}

int ProcessListModel::uptimeSeconds(const QString& id) const
{
    return m_manager->uptimeSeconds(id);
}

int ProcessListModel::profileRunningCount(const QString& name) const
{
    return m_manager->profileRunningCount(name);
}

int ProcessListModel::profileTotalCount(const QString& name) const
{
    return m_manager->profileTotalCount(name);
}

void ProcessListModel::removeProgram(const QString& id)
{
    m_manager->removeProgram(id);
}

void ProcessListModel::setBind(const QString& id, const QString& bind)
{
    m_manager->setBind(id, bind);
}

void ProcessListModel::setArgs(const QString& id, const QString& args)
{
    m_manager->setArgs(id, args);
}

void ProcessListModel::setProgramProfile(const QString& id, const QString& profile)
{
    m_manager->setProgramProfile(id, profile);
}

QStringList ProcessListModel::profiles() const
{
    return m_manager->profiles();
}

void ProcessListModel::addProfile(const QString& name)
{
    m_manager->addProfile(name);
}

void ProcessListModel::removeProfile(const QString& name)
{
    m_manager->removeProfile(name);
}

void ProcessListModel::renameProfile(const QString& oldName, const QString& newName)
{
    m_manager->renameProfile(oldName, newName);
}

void ProcessListModel::startProfile(const QString& name)
{
    m_manager->startProfile(name);
}

void ProcessListModel::stopProfile(const QString& name)
{
    m_manager->stopProfile(name);
}

QString ProcessListModel::profileBind(const QString& name) const
{
    return m_manager->profileBind(name);
}

void ProcessListModel::setProfileBind(const QString& name, const QString& bind)
{
    m_manager->setProfileBind(name, bind);
}

void ProcessListModel::start(const QString& id)
{
    m_manager->start(id);
}

void ProcessListModel::stop(const QString& id)
{
    m_manager->stop(id);
}

void ProcessListModel::stopAll()
{
    m_manager->stopAll();
}

void ProcessListModel::restart(const QString& id)
{
    m_manager->restart(id);
}

void ProcessListModel::setWatch(const QString& id, bool on)
{
    m_manager->setWatch(id, on);
}

int ProcessListModel::launchDelayMs() const
{
    return m_manager->launchDelayMs();
}

void ProcessListModel::setLaunchDelayMs(int ms)
{
    m_manager->setLaunchDelayMs(ms);
}

QString ProcessListModel::autoStartProfile() const
{
    return m_manager->autoStartProfile();
}

void ProcessListModel::setAutoStartProfile(const QString& name)
{
    m_manager->setAutoStartProfile(name);
}

bool ProcessListModel::exportConfig(const QUrl& url) const
{
    const QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    return m_manager->exportConfig(path);
}

bool ProcessListModel::importConfig(const QUrl& url)
{
    const QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    const bool ok = m_manager->importConfig(path);
    if (ok) {
        beginResetModel();
        m_rows = m_manager->entries();
        endResetModel();
    }
    return ok;
}

int ProcessListModel::indexOfId(const QString& id) const
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows.at(i).id == id)
            return i;
    }
    return -1;
}

void ProcessListModel::onListChanged()
{
    beginResetModel();
    m_rows = m_manager->entries();
    endResetModel();
}

void ProcessListModel::onRunStateChanged(const QString& id, bool running)
{
    Q_UNUSED(running)

    const int row = indexOfId(id);
    if (row < 0)
        return;

    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {RunningRole});
}

void ProcessListModel::onErrorOccurred(const QString& id, const QString& message)
{
    Q_UNUSED(id)
    emit errorMessage(message);
}
