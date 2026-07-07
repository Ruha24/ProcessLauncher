#ifndef WINDOWSTATE_H
#define WINDOWSTATE_H

#include <QObject>
#include <QVariantMap>

class WindowState : public QObject
{
    Q_OBJECT
public:
    explicit WindowState(QObject* parent = nullptr);

    Q_INVOKABLE QVariantMap load() const;
    Q_INVOKABLE void save(int x, int y, int width, int height);
};

#endif
