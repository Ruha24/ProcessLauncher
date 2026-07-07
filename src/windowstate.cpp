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
