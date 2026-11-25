#include "schedulemodel.h"
#include "processmanager.h"
#include "apppaths.h"

#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>
#include <QTime>
#include <QDateTime>

namespace {
constexpr int kCheckIntervalMs = 30000;
// Если компьютер спал или был занят в нужную минуту, правило ещё
// срабатывает в течение этого окна.
constexpr int kGraceSecs = 5 * 60;
constexpr int kAllDays = 0x7F;
constexpr int kWeekdays = 0x1F;
constexpr int kWeekend = 0x60;

int dayBit(const QDate& d)
{
    return 1 << (d.dayOfWeek() - 1);   // dayOfWeek: 1 = понедельник
}
}

ScheduleModel::ScheduleModel(ProcessManager* manager, QObject* parent)
    : QAbstractListModel(parent)
    , m_manager(manager)
    , m_timer(new QTimer(this))
{
    load();
    connect(m_timer, &QTimer::timeout, this, &ScheduleModel::tick);
    if (m_manager) {
        connect(m_manager, &ProcessManager::profileRenamed,
                this, &ScheduleModel::onProfileRenamed);
        connect(m_manager, &ProcessManager::profileRemoved,
                this, &ScheduleModel::onProfileRemoved);
        connect(m_manager, &ProcessManager::listChanged, this, [this]() {
            if (!m_rules.isEmpty())
                emit dataChanged(index(0, 0), index(m_rules.size() - 1, 0),
                                 {TargetLabelRole});
        });
    }
    m_timer->start(kCheckIntervalMs);
    tick();
}

QString ScheduleModel::configFilePath() const
{
    return AppPaths::configFile(QStringLiteral("schedule.json"));
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
    case TargetLabelRole:
        // Имя программы берём актуальное — её могли переименовать.
        if (r.targetType == QLatin1String("program") && m_manager) {
            const QString name = m_manager->programName(r.targetId);
            if (!name.isEmpty())
                return name;
        }
        return r.targetLabel;
    case HourRole:        return r.hour;
    case MinuteRole:      return r.minute;
    case DaysRole:        return r.days;
    case OnceRole:        return r.once;
    case ActionRole:      return r.action;
    case EnabledRole:     return r.enabled;
    case TimeTextRole:
        return QString::asprintf("%02d:%02d", r.hour, r.minute);
    case DescriptionRole: return describe(r);
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
        {DaysRole,        QByteArrayLiteral("days")},
        {OnceRole,        QByteArrayLiteral("once")},
        {ActionRole,      QByteArrayLiteral("action")},
        {EnabledRole,     QByteArrayLiteral("isEnabled")},
        {TimeTextRole,    QByteArrayLiteral("timeText")},
        {DescriptionRole, QByteArrayLiteral("description")},
    };
}

QString ScheduleModel::daysText(int days)
{
    days &= kAllDays;
    if (days == kAllDays) return tr("every day");
    if (days == kWeekdays) return tr("weekdays");
    if (days == kWeekend)  return tr("weekends");

    static const char* names[] = {
        QT_TR_NOOP("Mon"), QT_TR_NOOP("Tue"), QT_TR_NOOP("Wed"), QT_TR_NOOP("Thu"),
        QT_TR_NOOP("Fri"), QT_TR_NOOP("Sat"), QT_TR_NOOP("Sun"),
    };
    QStringList parts;
    for (int i = 0; i < 7; ++i)
        if (days & (1 << i))
            parts << tr(names[i]);
    return parts.join(QStringLiteral(", "));
}

QString ScheduleModel::describe(const ScheduleRule& r) const
{
    const bool stop = (r.action == QLatin1String("stop"));
    const bool profile = (r.targetType == QLatin1String("profile"));
    const QString what = profile
        ? (stop ? tr("Stop whole profile") : tr("Start whole profile"))
        : (stop ? tr("Stop program") : tr("Start program"));
    if (r.once)
        return tr("%1, once").arg(what);
    return tr("%1, %2").arg(what, daysText(r.days));
}

void ScheduleModel::addRule(const QString& action,
                            const QString& targetType, const QString& targetId,
                            const QString& targetLabel,
                            int hour, int minute, int days, bool once)
{
    ScheduleRule r;
    r.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    r.action = (action == QLatin1String("stop")) ? action : QStringLiteral("start");
    r.targetType = targetType;
    r.targetId = targetId;
    r.targetLabel = targetLabel;
    r.hour = qBound(0, hour, 23);
    r.minute = qBound(0, minute, 59);
    r.days = once ? kAllDays : (days & kAllDays);
    r.once = once;
    r.enabled = true;
    if (r.days == 0)
        return;

    // Время сегодня уже прошло — правило ждёт следующего раза, а не
    // срабатывает сразу из-за окна на случай сна.
    if (QTime(r.hour, r.minute) < QTime::currentTime())
        r.lastRun = QDate::currentDate();

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
    // То же, что при добавлении: включённое правило не догоняет
    // уже прошедшее сегодня время.
    if (on && QTime(m_rules[row].hour, m_rules[row].minute) < QTime::currentTime())
        m_rules[row].lastRun = QDate::currentDate();
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

void ScheduleModel::onProfileRenamed(const QString& oldName, const QString& newName)
{
    bool changed = false;
    for (int i = 0; i < m_rules.size(); ++i) {
        ScheduleRule& r = m_rules[i];
        if (r.targetType != QLatin1String("profile") || r.targetId != oldName)
            continue;
        const bool labelWasName = (r.targetLabel == oldName);
        r.targetId = newName;
        if (labelWasName)
            r.targetLabel = newName;
        const QModelIndex idx = index(i, 0);
        emit dataChanged(idx, idx, {TargetIdRole, TargetLabelRole});
        changed = true;
    }
    if (changed)
        save();
}

void ScheduleModel::onProfileRemoved(const QString& name)
{
    // Правило без цели всё равно ничего не запустит — убираем его.
    bool changed = false;
    for (int i = m_rules.size() - 1; i >= 0; --i) {
        const ScheduleRule& r = m_rules.at(i);
        if (r.targetType != QLatin1String("profile") || r.targetId != name)
            continue;
        beginRemoveRows(QModelIndex(), i, i);
        m_rules.removeAt(i);
        endRemoveRows();
        changed = true;
    }
    if (changed)
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
        if (!(r.days & dayBit(today)))
            continue;
        const int late = QTime(r.hour, r.minute).secsTo(nowTime);
        if (late < 0 || late > kGraceSecs)
            continue;

        const bool profile = (r.targetType == QLatin1String("profile"));
        if (r.action == QLatin1String("stop")) {
            if (profile) m_manager->stopProfile(r.targetId);
            else         m_manager->stop(r.targetId);
        } else {
            if (profile) m_manager->startProfile(r.targetId);
            else         m_manager->start(r.targetId);
        }

        r.lastRun = today;
        if (r.once) {
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
        r.action = o.value(QStringLiteral("action")).toString() == QLatin1String("stop")
                       ? QStringLiteral("stop") : QStringLiteral("start");
        // Старый формат: только "daily" (true — каждый день, false — один раз).
        if (o.contains(QStringLiteral("once")))
            r.once = o.value(QStringLiteral("once")).toBool(false);
        else
            r.once = !o.value(QStringLiteral("daily")).toBool(true);
        r.days = o.value(QStringLiteral("days")).toInt(kAllDays) & kAllDays;
        if (r.days == 0)
            r.days = kAllDays;
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
            {QStringLiteral("action"),      r.action},
            {QStringLiteral("days"),        r.days},
            {QStringLiteral("once"),        r.once},
            // Для старых версий: им понятно только "daily".
            {QStringLiteral("daily"),       !r.once},
            {QStringLiteral("enabled"),     r.enabled},
            {QStringLiteral("lastRun"),
             r.lastRun.isValid() ? r.lastRun.toString(Qt::ISODate) : QString()},
        });
    }
    QSaveFile f(configFilePath());
    if (!f.open(QIODevice::WriteOnly))
        return;
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    f.commit();
}

void ScheduleModel::retranslate()
{
    if (!m_rules.isEmpty())
        emit dataChanged(index(0), index(int(m_rules.size()) - 1));
}
