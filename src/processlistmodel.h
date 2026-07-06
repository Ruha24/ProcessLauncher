#ifndef PROCESSLISTMODEL_H
#define PROCESSLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QUrl>

#include "processmanager.h"

// Модель-адаптер: превращает ProcessManager в модель для QML ListView.
// Держит собственный упорядоченный снимок записей (m_rows) и синхронизирует
// его с менеджером по сигналам. Действия (add/remove/start/stop) проксируются
// в менеджер; наружу для QML они выставлены как Q_INVOKABLE.
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

    // Действия, вызываемые из QML.
    Q_INVOKABLE void addProgram(const QString& path, const QString& bind);
    Q_INVOKABLE void removeProgram(const QString& id);
    Q_INVOKABLE void setBind(const QString& id, const QString& bind);
    Q_INVOKABLE void start(const QString& id);
    Q_INVOKABLE void stop(const QString& id);
    Q_INVOKABLE void stopAll();

    // QFileDialog из QML отдаёт file:// URL — принимаем и нормализуем.
    Q_INVOKABLE void addProgramFromUrl(const QUrl& url, const QString& bind);

private slots:
    void onListChanged();
    void onRunStateChanged(const QString& id, bool running);

private:
    int indexOfId(const QString& id) const;

    ProcessManager*     m_manager = nullptr;
    QList<ProcessEntry> m_rows;   // упорядоченный снимок (как manager->entries())
};

#endif // PROCESSLISTMODEL_H
