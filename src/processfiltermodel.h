#ifndef PROCESSFILTERMODEL_H
#define PROCESSFILTERMODEL_H

#include <QSortFilterProxyModel>

class ProcessFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ProcessFilterModel(QObject* parent = nullptr);

    Q_INVOKABLE void setSearchText(const QString& text);
    Q_INVOKABLE void setActiveProfile(const QString& profile);

protected:
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex& sourceParent) const override;

private:
    QString m_search;
    QString m_profile;
};

#endif
