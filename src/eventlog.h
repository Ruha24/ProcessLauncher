#ifndef EVENTLOG_H
#define EVENTLOG_H

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QDateTime>

struct LogEntry
{
    QDateTime time;
    QString   kind;
    QString   text;
};

class EventLogModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role {
        TimeRole = Qt::UserRole + 1,
        KindRole,
        TextRole,
    };
    Q_ENUM(Role)

    explicit EventLogModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void clear();

public slots:
    void logCrash(const QString& name, const QString& detail);
    void logError(const QString& message);

private:
    QString logFilePath() const;
    void loadFromFile();
    void appendToFile(const LogEntry& e);
    void addEntry(const LogEntry& e);

    QList<LogEntry> m_rows;
};

#endif
