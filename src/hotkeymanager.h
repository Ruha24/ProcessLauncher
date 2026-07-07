#ifndef HOTKEYMANAGER_H
#define HOTKEYMANAGER_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QAbstractNativeEventFilter>

class HotkeyManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit HotkeyManager(QObject* parent = nullptr);
    ~HotkeyManager() override;

    bool setHotkey(const QString& id, const QString& bind);
    void clearHotkey(const QString& id);
    void clearAll();

    static bool isValidBind(const QString& bind);

    bool nativeEventFilter(const QByteArray& eventType, void* message,
                           qintptr* result) override;

signals:
    void activated(const QString& id);

private:
    struct Native { quint32 mods = 0; quint32 vk = 0; };
    static bool parseBind(const QString& bind, Native& out);

    int m_nextHotkeyId = 1;
    QHash<QString, int> m_idToHotkey;
    QHash<int, QString> m_hotkeyToId;
};

#endif
