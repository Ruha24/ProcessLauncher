#ifndef SCHEDULEMODEL_H
#define SCHEDULEMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QDate>

class ProcessManager;
class QTimer;

struct ScheduleRule
{
    QString id;
    QString targetType;
    QString targetId;
    QString targetLabel;
    QString action = QStringLiteral("start");   // "start" | "stop"
    int     hour = 9;
    int     minute = 0;
    int     days = 0x7F;    // бит 0 — понедельник … бит 6 — воскресенье
    bool    once = false;   // сработать один раз и выключиться
    bool    enabled = true;
    QDate   lastRun;
};

class ScheduleModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        TargetTypeRole,
        TargetIdRole,
        TargetLabelRole,
        HourRole,
        MinuteRole,
        DaysRole,
        OnceRole,
        ActionRole,
        EnabledRole,
        TimeTextRole,
        DescriptionRole,
    };
    Q_ENUM(Role)

    explicit ScheduleModel(ProcessManager* manager, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addRule(const QString& action,
                             const QString& targetType, const QString& targetId,
                             const QString& targetLabel,
                             int hour, int minute, int days, bool once);
    Q_INVOKABLE void setEnabled(int row, bool on);
    // Описания правил собираются из tr() — после смены языка их нужно обновить.
    void retranslate();
    Q_INVOKABLE void removeRule(int row);

private slots:
    void tick();
    void onProfileRenamed(const QString& oldName, const QString& newName);
    void onProfileRemoved(const QString& name);

private:
    QString configFilePath() const;
    void load();
    void save() const;
    QString describe(const ScheduleRule& r) const;
    static QString daysText(int days);

    ProcessManager* m_manager = nullptr;
    QTimer*         m_timer = nullptr;
    QList<ScheduleRule> m_rules;
};

#endif
