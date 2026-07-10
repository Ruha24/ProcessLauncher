#ifndef STARTUPMANAGER_H
#define STARTUPMANAGER_H

#include <QObject>
#include <QAbstractListModel>
#include <QList>
#include <QString>

struct StartupEntry
{
    QString name;
    QString command;   // путь к exe / командная строка
    QString source;    // "Registry (HKCU)" | "Startup folder"
    bool    enabled = true;
};

class StartupModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        CommandRole,
        SourceRole,
        EnabledRole,
    };
    Q_ENUM(Role)

    explicit StartupModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setEnabled(int row, bool on);
    Q_INVOKABLE void removeEntry(int row);
    Q_INVOKABLE bool available() const;

signals:
    void errorMessage(const QString& text);

private:
    QList<StartupEntry> collect() const;

    QList<StartupEntry> m_rows;
};

#endif
