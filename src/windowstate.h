#ifndef WINDOWSTATE_H
#define WINDOWSTATE_H

#include <QObject>
#include <QVariantMap>
#include <QVariant>
#include <QString>

class WindowState : public QObject
{
    Q_OBJECT
public:
    explicit WindowState(QObject* parent = nullptr);

    Q_INVOKABLE QVariantMap load() const;
    Q_INVOKABLE void save(int x, int y, int width, int height);

    Q_INVOKABLE QVariant value(const QString& key, const QVariant& def) const;
    Q_INVOKABLE void setValue(const QString& key, const QVariant& val);
    Q_INVOKABLE bool boolValue(const QString& key, bool def) const;
};

#endif
