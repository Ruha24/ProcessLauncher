#ifndef PROCESSLISTMODEL_H
#define PROCESSLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>

#include "processmanager.h"

class ProcessListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        NameRole,
        BindRole,
        PathRole,
        RunningRole,
        ArgsRole,
        ProfileRole,
        WatchRole,
        NoteRole,
    };
    Q_ENUM(Role)

    explicit ProcessListModel(ProcessManager* manager, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addProgram(const QString& path, const QString& bind);
    Q_INVOKABLE void removeProgram(const QString& id);
    Q_INVOKABLE void setBind(const QString& id, const QString& bind);
    Q_INVOKABLE bool isValidBind(const QString& bind) const;
    Q_INVOKABLE void setArgs(const QString& id, const QString& args);
    Q_INVOKABLE void setNote(const QString& id, const QString& note);
    Q_INVOKABLE void setProgramProfile(const QString& id, const QString& profile);
    Q_INVOKABLE void start(const QString& id);
    Q_INVOKABLE void stop(const QString& id);
    Q_INVOKABLE void restart(const QString& id);
    Q_INVOKABLE void stopAll();
    Q_INVOKABLE void setWatch(const QString& id, bool on);

    Q_INVOKABLE int  launchDelayMs() const;
    Q_INVOKABLE void setLaunchDelayMs(int ms);
    Q_INVOKABLE int  pollInterval() const;
    Q_INVOKABLE void setPollInterval(int ms);
    Q_INVOKABLE QString autoStartProfile() const;
    Q_INVOKABLE void setAutoStartProfile(const QString& name);

    Q_INVOKABLE bool exportConfig(const QUrl& url) const;
    Q_INVOKABLE bool importConfig(const QUrl& url);

    Q_INVOKABLE void addProgramFromUrl(const QUrl& url, const QString& profile);

    Q_INVOKABLE void openFileLocation(const QString& path) const;
    Q_INVOKABLE void copyPath(const QString& path) const;

    Q_INVOKABLE QStringList profiles() const;
    Q_INVOKABLE void addProfile(const QString& name);
    Q_INVOKABLE void removeProfile(const QString& name);
    Q_INVOKABLE void renameProfile(const QString& oldName, const QString& newName);
    Q_INVOKABLE void startProfile(const QString& name);
    Q_INVOKABLE void stopProfile(const QString& name);
    Q_INVOKABLE QString profileBind(const QString& name) const;
    Q_INVOKABLE void setProfileBind(const QString& name, const QString& bind);
    Q_INVOKABLE QString profileColor(const QString& name) const;
    Q_INVOKABLE void setProfileColor(const QString& name, const QString& color);
    Q_INVOKABLE QString profileIcon(const QString& name) const;
    Q_INVOKABLE void setProfileIcon(const QString& name, const QString& icon);

    Q_INVOKABLE int uptimeSeconds(const QString& id) const;
    Q_INVOKABLE int profileRunningCount(const QString& name) const;
    Q_INVOKABLE int profileTotalCount(const QString& name) const;

signals:

    void errorMessage(const QString& text);
    void profilesChanged();
    void tick();
    void processExited(const QString& name, const QString& profile);

private slots:
    void onListChanged();
    void onRunStateChanged(const QString& id, bool running);
    void onErrorOccurred(const QString& id, const QString& message);

private:
    int indexOfId(const QString& id) const;

    ProcessManager*     m_manager = nullptr;
    QList<ProcessEntry> m_rows;
};

#endif
