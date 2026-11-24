#ifndef AUTOSTARTMANAGER_H
#define AUTOSTARTMANAGER_H

#include <QObject>

class AutostartManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool available READ isAvailable CONSTANT)

public:
    explicit AutostartManager(QObject* parent = nullptr);

    bool isEnabled() const;
    bool isAvailable() const;

    Q_INVOKABLE void setEnabled(bool on);

    // Ярлык на рабочем столе, запускающий профиль (ProcessLauncher --profile "…").
    Q_INVOKABLE bool createProfileShortcut(const QString& profile) const;

signals:
    void enabledChanged();

private:
    QString shortcutPath() const;
    bool createShortcut() const;
};

#endif
