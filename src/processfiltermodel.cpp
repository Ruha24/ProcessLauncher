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

void ProcessFilterModel::setActiveProfile(const QString& profile)
{
    if (m_profile == profile)
        return;
    m_profile = profile;
    invalidateFilter();
}

bool ProcessFilterModel::filterAcceptsRow(int sourceRow,
                                          const QModelIndex& sourceParent) const
{
    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);

    if (!m_profile.isEmpty()) {
        const QString profile =
            sourceModel()->data(idx, ProcessListModel::ProfileRole).toString();
        if (profile != m_profile)
            return false;
    }

    if (!m_search.isEmpty()) {
        const QString name =
            sourceModel()->data(idx, ProcessListModel::NameRole).toString();
        if (!name.contains(m_search, Qt::CaseInsensitive))
            return false;
    }

    return true;
}
