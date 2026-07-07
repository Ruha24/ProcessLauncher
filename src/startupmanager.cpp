#include "startupmanager.h"

#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QByteArray>
#include <algorithm>

namespace {

const QString kRunKey =
    QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");
const QString kApprovedRunKey =
    QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run");
const QString kApprovedFolderKey =
    QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\StartupFolder");

const QString kSourceRegistry = QStringLiteral("Registry (HKCU)");
const QString kSourceFolder   = QStringLiteral("Startup folder");

bool approvedEnabled(const QByteArray& v)
{
    if (v.isEmpty())
        return true;
    return static_cast<unsigned char>(v.at(0)) != 0x03;
}

QByteArray approvedValue(bool enabled)
{
    QByteArray v(12, '\0');
    v[0] = enabled ? 0x02 : 0x03;
    return v;
}

QString startupFolderPath()
{
    const QString base = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);

    QDir d(base);
    d.cdUp();
    return d.absoluteFilePath(
        QStringLiteral("Microsoft/Windows/Start Menu/Programs/Startup"));
}

}

StartupModel::StartupModel(QObject* parent)
    : QAbstractListModel(parent)
{
    refresh();
}

bool StartupModel::available() const
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

int StartupModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

QVariant StartupModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return QVariant();

    const StartupEntry& e = m_rows.at(index.row());
    switch (role) {
    case NameRole:    return e.name;
    case CommandRole: return e.command;
    case SourceRole:  return e.source;
    case EnabledRole: return e.enabled;
    }
    return QVariant();
}

QHash<int, QByteArray> StartupModel::roleNames() const
{
    return {
        {NameRole,    QByteArrayLiteral("name")},
        {CommandRole, QByteArrayLiteral("command")},
        {SourceRole,  QByteArrayLiteral("source")},
        {EnabledRole, QByteArrayLiteral("enabled")},
    };
}

QList<StartupEntry> StartupModel::collect() const
{
    QList<StartupEntry> rows;

#ifdef Q_OS_WIN

    {
        QSettings run(kRunKey, QSettings::NativeFormat);
        QSettings approved(kApprovedRunKey, QSettings::NativeFormat);
        const QStringList keys = run.allKeys();
        for (const QString& k : keys) {
            StartupEntry e;
            e.name    = k;
            e.command = run.value(k).toString();
            e.source  = kSourceRegistry;
            e.enabled = approvedEnabled(approved.value(k).toByteArray());
            rows.append(e);
        }
    }

    {
        QSettings approved(kApprovedFolderKey, QSettings::NativeFormat);
        QDir dir(startupFolderPath());
        const QFileInfoList files =
            dir.entryInfoList(QDir::Files, QDir::Name);
        for (const QFileInfo& fi : files) {
            StartupEntry e;
            e.name    = fi.completeBaseName();
            e.command = fi.absoluteFilePath();
            e.source  = kSourceFolder;
            e.enabled = approvedEnabled(
                approved.value(fi.fileName()).toByteArray());
            rows.append(e);
        }
    }
#endif

    std::sort(rows.begin(), rows.end(),
              [](const StartupEntry& a, const StartupEntry& b) {
                  return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
              });
    return rows;
}

void StartupModel::refresh()
{
    beginResetModel();
    m_rows = collect();
    endResetModel();
}

void StartupModel::setEnabled(int row, bool on)
{
#ifdef Q_OS_WIN
    if (row < 0 || row >= m_rows.size())
        return;

    StartupEntry& e = m_rows[row];
    const QString approvedKey =
        e.source == kSourceRegistry ? kApprovedRunKey : kApprovedFolderKey;
    const QString valueName =
        e.source == kSourceRegistry ? e.name : QFileInfo(e.command).fileName();

    QSettings approved(approvedKey, QSettings::NativeFormat);
    approved.setValue(valueName, approvedValue(on));
    approved.sync();

    if (approved.status() != QSettings::NoError) {
        emit errorMessage(tr("Could not change startup state for %1").arg(e.name));
        return;
    }

    e.enabled = on;
    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {EnabledRole});
#else
    Q_UNUSED(row) Q_UNUSED(on)
#endif
}

void StartupModel::removeEntry(int row)
{
#ifdef Q_OS_WIN
    if (row < 0 || row >= m_rows.size())
        return;

    const StartupEntry e = m_rows.at(row);
    bool ok = false;

    if (e.source == kSourceRegistry) {
        QSettings run(kRunKey, QSettings::NativeFormat);
        run.remove(e.name);
        run.sync();
        ok = (run.status() == QSettings::NoError);
    } else {
        ok = QFile::remove(e.command);
    }

    if (!ok) {
        emit errorMessage(tr("Could not remove %1").arg(e.name));
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    m_rows.removeAt(row);
    endRemoveRows();
#else
    Q_UNUSED(row)
#endif
}
