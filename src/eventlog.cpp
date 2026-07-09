#include "eventlog.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <algorithm>

namespace {
constexpr int kMaxEntries = 500;
}

EventLogModel::EventLogModel(QObject* parent)
    : QAbstractListModel(parent)
{
    loadFromFile();
}

QString EventLogModel::logFilePath() const
{
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/events.log");
}

int EventLogModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

QVariant EventLogModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return QVariant();

    const LogEntry& e = m_rows.at(index.row());
    switch (role) {
    case TimeRole: return e.time.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    case KindRole: return e.kind;
    case TextRole: return e.text;
    }
    return QVariant();
}

QHash<int, QByteArray> EventLogModel::roleNames() const
{
    return {
        {TimeRole, QByteArrayLiteral("time")},
        {KindRole, QByteArrayLiteral("kind")},
        {TextRole, QByteArrayLiteral("text")},
    };
}

void EventLogModel::loadFromFile()
{
    QFile f(logFilePath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QList<LogEntry> loaded;
    QTextStream in(&f);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        const QStringList parts = line.split(QLatin1Char('\t'));
        if (parts.size() < 3)
            continue;
        LogEntry e;
        e.time = QDateTime::fromString(parts.at(0), Qt::ISODate);
        e.kind = parts.at(1);
        e.text = parts.mid(2).join(QLatin1Char('\t'));
        loaded.append(e);
    }
    f.close();

    std::reverse(loaded.begin(), loaded.end());
    if (loaded.size() > kMaxEntries)
        loaded = loaded.mid(0, kMaxEntries);

    beginResetModel();
    m_rows = loaded;
    endResetModel();
}

void EventLogModel::appendToFile(const LogEntry& e)
{
    QFile f(logFilePath());
    if (!f.open(QIODevice::Append | QIODevice::Text))
        return;
    QTextStream out(&f);
    out << e.time.toString(Qt::ISODate) << '\t'
        << e.kind << '\t'
        << e.text << '\n';
    f.close();
}

void EventLogModel::addEntry(const LogEntry& e)
{
    beginInsertRows(QModelIndex(), 0, 0);
    m_rows.prepend(e);
    endInsertRows();

    if (m_rows.size() > kMaxEntries) {
        beginRemoveRows(QModelIndex(), m_rows.size() - 1, m_rows.size() - 1);
        m_rows.removeLast();
        endRemoveRows();
    }

    appendToFile(e);
}

void EventLogModel::logCrash(const QString& name, const QString& detail)
{
    LogEntry e;
    e.time = QDateTime::currentDateTime();
    e.kind = QStringLiteral("Crash");

    const QString n = name.trimmed().isEmpty()
                          ? QStringLiteral("(unknown program)")
                          : name.trimmed();
    e.text = detail.trimmed().isEmpty()
                 ? n
                 : (n + QStringLiteral(" — ") + detail.trimmed());
    addEntry(e);
}

void EventLogModel::logError(const QString& message)
{
    if (message.trimmed().isEmpty())
        return;
    LogEntry e;
    e.time = QDateTime::currentDateTime();
    e.kind = QStringLiteral("Error");
    e.text = message;
    addEntry(e);
}

void EventLogModel::clear()
{
    beginResetModel();
    m_rows.clear();
    endResetModel();
    QFile::remove(logFilePath());
}
