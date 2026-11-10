#ifndef HOTKEYMANAGER_H
#define HOTKEYMANAGER_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QList>

// Глобальные горячие клавиши.
//   Windows — низкоуровневый перехват клавиатуры (WH_KEYBOARD_LL).
//   macOS   — системные хоткеи Carbon (RegisterEventHotKey); особых
//             разрешений (Accessibility / Input Monitoring) не требуют.
//
// Формат привязки одинаков на обеих системах: "Ctrl+Alt+K", "Shift+F5".
// Модификаторы: Ctrl, Alt (Option), Shift, Win (Cmd на macOS).
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

#if defined(Q_OS_WIN)
    static bool isHookInstance();
    static void dispatchKey(quint32 vk, quint32 activeMods);
#elif defined(Q_OS_MACOS)
    void handleNativeHotkey(quint32 nativeId);
#endif

signals:
    void activated(const QString& id);

private:
    struct Binding {
        QString  id;
        quint32  mods = 0;
        quint32  vk = 0;
        quint32  nativeId = 0;        // macOS: номер зарегистрированного хоткея
        void*    nativeRef = nullptr; // macOS: EventHotKeyRef
    };

    // Переводит строку привязки в модификаторы и код клавиши текущей системы.
    static bool parseBind(const QString& bind, quint32& mods, quint32& vk);

    bool registerNative(Binding& b);
    void unregisterNative(Binding& b);

    QList<Binding> m_bindings;

#if defined(Q_OS_WIN)
    void installHook();
    void removeHook();
    bool handleKey(quint32 vk, quint32 activeMods);

    void* m_hook = nullptr;
    static HotkeyManager* s_instance;
#elif defined(Q_OS_MACOS)
    void*   m_eventHandler = nullptr;   // EventHandlerRef
    quint32 m_nextNativeId = 1;
#endif
};

#endif
