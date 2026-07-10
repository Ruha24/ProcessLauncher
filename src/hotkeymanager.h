#ifndef HOTKEYMANAGER_H
#define HOTKEYMANAGER_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QList>

class HotkeyManager : public QObject
{
    Q_OBJECT

public:
    explicit HotkeyManager(QObject* parent = nullptr);
    ~HotkeyManager() override;

    bool setHotkey(const QString& id, const QString& bind);
    void clearHotkey(const QString& id);
    void clearAll();

    static bool isValidBind(const QString& bind);

#ifdef Q_OS_WIN
    static bool isHookInstance();
    static void dispatchKey(quint32 vk, quint32 activeMods);
#endif

signals:
    void activated(const QString& id);

private:
    struct Native { quint32 mods = 0; quint32 vk = 0; };
    static bool parseBind(const QString& bind, Native& out);

    struct Binding {
        QString  id;
        quint32  mods = 0;
        quint32  vk = 0;
    };

    void installHook();
    void removeHook();
    bool handleKey(quint32 vk, quint32 activeMods);

    QList<Binding> m_bindings;

#ifdef Q_OS_WIN
    void* m_hook = nullptr;
    static HotkeyManager* s_instance;
#endif
};

#endif
