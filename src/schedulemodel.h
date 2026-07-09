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
    int     hour = 9;
    int     minute = 0;
    bool    daily = true;
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
        DailyRole,
        EnabledRole,
        TimeTextRole,
    };
    Q_ENUM(Role)

    explicit ScheduleModel(ProcessManager* manager, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addRule(const QString& targetType, const QString& targetId,
                             const QString& targetLabel,
                             int hour, int minute, bool daily);
    Q_INVOKABLE void setEnabled(int row, bool on);
    Q_INVOKABLE void removeRule(int row);

private slots:
    void tick();

private:
    QString configFilePath() const;
    void load();
    void save() const;

    ProcessManager* m_manager = nullptr;
    QTimer*         m_timer = nullptr;
    QList<ScheduleRule> m_rules;
};

#endif
