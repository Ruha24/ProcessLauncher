#include "windowstate.h"

#include <QSettings>

WindowState::WindowState(QObject* parent)
    : QObject(parent)
{
}

QVariantMap WindowState::load() const
{
    QSettings s;
    s.beginGroup(QStringLiteral("window"));
    QVariantMap m;
    m[QStringLiteral("x")]      = s.value(QStringLiteral("x"), -1).toInt();
    m[QStringLiteral("y")]      = s.value(QStringLiteral("y"), -1).toInt();
    m[QStringLiteral("width")]  = s.value(QStringLiteral("width"), 0).toInt();
    m[QStringLiteral("height")] = s.value(QStringLiteral("height"), 0).toInt();
    s.endGroup();
    return m;
}

void WindowState::save(int x, int y, int width, int height)
{
    QSettings s;
    s.beginGroup(QStringLiteral("window"));
    s.setValue(QStringLiteral("x"), x);
    s.setValue(QStringLiteral("y"), y);
    s.setValue(QStringLiteral("width"), width);
    s.setValue(QStringLiteral("height"), height);
    s.endGroup();
}

QVariant WindowState::value(const QString& key, const QVariant& def) const
{
    QSettings s;
    s.beginGroup(QStringLiteral("app"));
    const QVariant v = s.value(key, def);
    s.endGroup();
    return v;
}

void WindowState::setValue(const QString& key, const QVariant& val)
{
    QSettings s;
    s.beginGroup(QStringLiteral("app"));
    s.setValue(key, val);
    s.endGroup();
}

bool WindowState::boolValue(const QString& key, bool def) const
{
    QSettings s;
    s.beginGroup(QStringLiteral("app"));
    const QVariant v = s.value(key);
    s.endGroup();
    if (!v.isValid())
        return def;
    if (v.typeId() == QMetaType::Bool)
        return v.toBool();
    const QString str = v.toString().trimmed().toLower();
    if (str == QLatin1String("true") || str == QLatin1String("1"))
        return true;
    if (str == QLatin1String("false") || str == QLatin1String("0"))
        return false;
    return def;
}
