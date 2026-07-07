#include "processfiltermodel.h"
#include "processlistmodel.h"

ProcessFilterModel::ProcessFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

void ProcessFilterModel::setSearchText(const QString& text)
{
    const QString trimmed = text.trimmed();
    if (m_search == trimmed)
        return;
    m_search = trimmed;
    invalidateFilter();
}

bool ProcessFilterModel::filterAcceptsRow(int sourceRow,
                                          const QModelIndex& sourceParent) const
{
    if (m_search.isEmpty())
        return true;

    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    const QString name =
        sourceModel()->data(idx, ProcessListModel::NameRole).toString();
    return name.contains(m_search, Qt::CaseInsensitive);
}
