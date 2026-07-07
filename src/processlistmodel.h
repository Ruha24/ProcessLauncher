#ifndef PROCESSLISTMODEL_H
#define PROCESSLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QString>
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
    };
    Q_ENUM(Role)

    explicit ProcessListModel(ProcessManager* manager, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addProgram(const QString& path, const QString& bind);
    Q_INVOKABLE void removeProgram(const QString& id);
    Q_INVOKABLE void setBind(const QString& id, const QString& bind);
    Q_INVOKABLE void start(const QString& id);
    Q_INVOKABLE void stop(const QString& id);
    Q_INVOKABLE void stopAll();

    Q_INVOKABLE void addProgramFromUrl(const QUrl& url, const QString& bind);

signals:

    void errorMessage(const QString& text);

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
