#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <QObject>
#include <QHash>
#include <QSet>
#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
class QProcess;
class QTimer;
QT_END_NAMESPACE

class JobController;
class HotkeyManager;

struct ProcessEntry
{
    QString id;
    QString path;
    QString name;
    QString bind;
};

class ProcessManager : public QObject
{
    Q_OBJECT

public:
    explicit ProcessManager(QObject* parent = nullptr);
    ~ProcessManager() override;

    QString addProgram(const QString& path, const QString& bind = QString());
    void    removeProgram(const QString& id);
    void    setBind(const QString& id, const QString& bind);

    void start(const QString& id);
    void stop(const QString& id);
    void stopAll();

    bool isRunning(const QString& id) const;

    QList<ProcessEntry> entries() const;

    QString configFilePath() const;

signals:
    void runStateChanged(const QString& id, bool running);
    void listChanged();
    void errorOccurred(const QString& id, const QString& message);

private slots:
    void pollJobs();

private:
    void save() const;
    void load();
    void syncHotkey(const QString& id);

    QHash<QString, ProcessEntry>    m_entries;
    QHash<QString, JobController*>  m_jobs;
    QSet<QString>                   m_untracked;
    QTimer*                         m_pollTimer = nullptr;
    HotkeyManager*                  m_hotkeys = nullptr;
};

#endif
