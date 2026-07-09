#include "schedulemodel.h"
#include "processmanager.h"

#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>
#include <QTime>
#include <QDateTime>

namespace {
constexpr int kCheckIntervalMs = 30000;
}

ScheduleModel::ScheduleModel(ProcessManager* manager, QObject* parent)
    : QAbstractListModel(parent)
    , m_manager(manager)
    , m_timer(new QTimer(this))
{
    load();
    connect(m_timer, &QTimer::timeout, this, &ScheduleModel::tick);
    m_timer->start(kCheckIntervalMs);
    tick();
}

QString ScheduleModel::configFilePath() const
{
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/schedule.json");
}

int ScheduleModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_rules.size();
}

QVariant ScheduleModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rules.size())
        return QVariant();

    const ScheduleRule& r = m_rules.at(index.row());
    switch (role) {
    case IdRole:          return r.id;
    case TargetTypeRole:  return r.targetType;
    case TargetIdRole:    return r.targetId;
    case TargetLabelRole: return r.targetLabel;
    case HourRole:        return r.hour;
    case MinuteRole:      return r.minute;
    case DailyRole:       return r.daily;
    case EnabledRole:     return r.enabled;
    case TimeTextRole:
        return QString::asprintf("%02d:%02d", r.hour, r.minute);
    }
    return QVariant();
}

QHash<int, QByteArray> ScheduleModel::roleNames() const
{
    return {
        {IdRole,          QByteArrayLiteral("ruleId")},
        {TargetTypeRole,  QByteArrayLiteral("targetType")},
        {TargetIdRole,    QByteArrayLiteral("targetId")},
        {TargetLabelRole, QByteArrayLiteral("targetLabel")},
        {HourRole,        QByteArrayLiteral("hour")},
        {MinuteRole,      QByteArrayLiteral("minute")},
        {DailyRole,       QByteArrayLiteral("daily")},
        {EnabledRole,     QByteArrayLiteral("isEnabled")},
        {TimeTextRole,    QByteArrayLiteral("timeText")},
    };
}

void ScheduleModel::addRule(const QString& targetType, const QString& targetId,
                            const QString& targetLabel,
                            int hour, int minute, bool daily)
{
    ScheduleRule r;
    r.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    r.targetType = targetType;
    r.targetId = targetId;
    r.targetLabel = targetLabel;
    r.hour = qBound(0, hour, 23);
    r.minute = qBound(0, minute, 59);
    r.daily = daily;
    r.enabled = true;

    beginInsertRows(QModelIndex(), m_rules.size(), m_rules.size());
    m_rules.append(r);
    endInsertRows();
    save();
}

void ScheduleModel::setEnabled(int row, bool on)
{
    if (row < 0 || row >= m_rules.size())
        return;
    m_rules[row].enabled = on;
    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {EnabledRole});
    save();
}

void ScheduleModel::removeRule(int row)
{
    if (row < 0 || row >= m_rules.size())
        return;
    beginRemoveRows(QModelIndex(), row, row);
    m_rules.removeAt(row);
    endRemoveRows();
    save();
}

void ScheduleModel::tick()
{
    if (!m_manager)
        return;

    const QDateTime now = QDateTime::currentDateTime();
    const QDate today = now.date();
    const QTime nowTime = now.time();

    bool changed = false;
    for (int i = 0; i < m_rules.size(); ++i) {
        ScheduleRule& r = m_rules[i];
        if (!r.enabled)
            continue;
        if (r.lastRun == today)
            continue;
        if (nowTime.hour() != r.hour || nowTime.minute() != r.minute)
            continue;

        if (r.targetType == QLatin1String("profile"))
            m_manager->startProfile(r.targetId);
        else
            m_manager->start(r.targetId);

        r.lastRun = today;
        if (!r.daily) {
            r.enabled = false;
            const QModelIndex idx = index(i, 0);
            emit dataChanged(idx, idx, {EnabledRole});
        }
        changed = true;
    }
    if (changed)
        save();
}

void ScheduleModel::load()
{
    QFile f(configFilePath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isArray())
        return;

    QList<ScheduleRule> loaded;
    const QJsonArray arr = doc.array();
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        ScheduleRule r;
        r.id = o.value(QStringLiteral("id")).toString();
        if (r.id.isEmpty())
            r.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        r.targetType = o.value(QStringLiteral("targetType")).toString();
        r.targetId = o.value(QStringLiteral("targetId")).toString();
        r.targetLabel = o.value(QStringLiteral("targetLabel")).toString();
        r.hour = o.value(QStringLiteral("hour")).toInt(9);
        r.minute = o.value(QStringLiteral("minute")).toInt(0);
        r.daily = o.value(QStringLiteral("daily")).toBool(true);
        r.enabled = o.value(QStringLiteral("enabled")).toBool(true);
        const QString lr = o.value(QStringLiteral("lastRun")).toString();
        if (!lr.isEmpty())
            r.lastRun = QDate::fromString(lr, Qt::ISODate);
        loaded.append(r);
    }

    beginResetModel();
    m_rules = loaded;
    endResetModel();
}

void ScheduleModel::save() const
{
    QJsonArray arr;
    for (const ScheduleRule& r : m_rules) {
        arr.append(QJsonObject{
            {QStringLiteral("id"),          r.id},
            {QStringLiteral("targetType"),  r.targetType},
            {QStringLiteral("targetId"),    r.targetId},
            {QStringLiteral("targetLabel"), r.targetLabel},
            {QStringLiteral("hour"),        r.hour},
            {QStringLiteral("minute"),      r.minute},
            {QStringLiteral("daily"),       r.daily},
            {QStringLiteral("enabled"),     r.enabled},
            {QStringLiteral("lastRun"),
             r.lastRun.isValid() ? r.lastRun.toString(Qt::ISODate) : QString()},
        });
    }
    QFile f(configFilePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    f.close();
}
