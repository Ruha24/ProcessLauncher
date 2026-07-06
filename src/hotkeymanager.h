#ifndef HOTKEYMANAGER_H
#define HOTKEYMANAGER_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QAbstractNativeEventFilter>

// Регистрирует системные (глобальные) горячие клавиши и уведомляет о нажатии
// сигналом activated(id). Клавиша срабатывает, даже когда окно свёрнуто.
//
// Формат бинда: "1", "A", "F5" или с модификаторами "Ctrl+Shift+1",
// "Alt+G". Пустой бинд = хоткей не регистрируется.
//
// Windows: RegisterHotKey + QAbstractNativeEventFilter (WM_HOTKEY).
// Другие ОС: заглушка (регистрация возвращает false) — сборка не ломается.
class HotkeyManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit HotkeyManager(QObject* parent = nullptr);
    ~HotkeyManager() override;

    // Привязать бинд к id. Возвращает false, если строка не распознана,
    // клавиша уже занята или платформа не поддерживается. Пустой bind
    // просто снимает прежнюю привязку для этого id.
    bool setHotkey(const QString& id, const QString& bind);
    void clearHotkey(const QString& id);
    void clearAll();

    bool nativeEventFilter(const QByteArray& eventType, void* message,
                           qintptr* result) override;

signals:
    void activated(const QString& id);

private:
    struct Native { quint32 mods = 0; quint32 vk = 0; };
    static bool parseBind(const QString& bind, Native& out);

    int m_nextHotkeyId = 1;
    QHash<QString, int> m_idToHotkey;   // entry id -> WinAPI hotkey id
    QHash<int, QString> m_hotkeyToId;   // WinAPI hotkey id -> entry id
};

#endif // HOTKEYMANAGER_H
