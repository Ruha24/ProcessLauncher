#include "processlistmodel.h"

ProcessListModel::ProcessListModel(ProcessManager* manager, QObject* parent)
    : QAbstractListModel(parent)
    , m_manager(manager)
{
    Q_ASSERT(m_manager);
    m_rows = m_manager->entries();

    connect(m_manager, &ProcessManager::listChanged,
            this, &ProcessListModel::onListChanged);
    connect(m_manager, &ProcessManager::runStateChanged,
            this, &ProcessListModel::onRunStateChanged);
}

int ProcessListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

QVariant ProcessListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return QVariant();

    const ProcessEntry& e = m_rows.at(index.row());
    switch (role) {
    case IdRole:      return e.id;
    case NameRole:    return e.name;
    case BindRole:    return e.bind;
    case PathRole:    return e.path;
    case RunningRole: return m_manager->isRunning(e.id);  // единый источник правды
    }
    return QVariant();
}

QHash<int, QByteArray> ProcessListModel::roleNames() const
{
    return {
        {IdRole,      QByteArrayLiteral("procId")},
        {NameRole,    QByteArrayLiteral("name")},
        {BindRole,    QByteArrayLiteral("bind")},
        {PathRole,    QByteArrayLiteral("path")},
        {RunningRole, QByteArrayLiteral("running")},
    };
}

void ProcessListModel::addProgram(const QString& path, const QString& bind)
{
    m_manager->addProgram(path, bind);   // manager эмитит listChanged -> onListChanged
}

void ProcessListModel::addProgramFromUrl(const QUrl& url, const QString& bind)
{
    const QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    m_manager->addProgram(path, bind);
}

void ProcessListModel::removeProgram(const QString& id)
{
    m_manager->removeProgram(id);
}

void ProcessListModel::setBind(const QString& id, const QString& bind)
{
    m_manager->setBind(id, bind);
}

void ProcessListModel::start(const QString& id)
{
    m_manager->start(id);
}

void ProcessListModel::stop(const QString& id)
{
    m_manager->stop(id);
}

void ProcessListModel::stopAll()
{
    m_manager->stopAll();
}

int ProcessListModel::indexOfId(const QString& id) const
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows.at(i).id == id)
            return i;
    }
    return -1;
}

// Состав/порядок списка изменился (add/remove/bind). Полный, но корректный
// reset — для десятков элементов это дёшево и гарантирует консистентность.
void ProcessListModel::onListChanged()
{
    beginResetModel();
    m_rows = m_manager->entries();
    endResetModel();
}

// Изменился только флаг «запущен» одной строки — точечное обновление роли,
// без полного сброса (важно для правила «dataChanged с конкретными ролями»).
void ProcessListModel::onRunStateChanged(const QString& id, bool running)
{
    Q_UNUSED(running)   // актуальное значение data() возьмёт из manager->isRunning()

    const int row = indexOfId(id);
    if (row < 0)
        return;

    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {RunningRole});
}
