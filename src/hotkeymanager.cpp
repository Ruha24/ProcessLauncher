#include "hotkeymanager.h"

#include <QGuiApplication>
#include <QStringList>

#ifdef Q_OS_WIN
#include <windows.h>
#ifndef MOD_NOREPEAT
#define MOD_NOREPEAT 0x4000
#endif
#endif

HotkeyManager::HotkeyManager(QObject* parent)
    : QObject(parent)
{
#ifdef Q_OS_WIN
    qApp->installNativeEventFilter(this);
#endif
}

HotkeyManager::~HotkeyManager()
{
    clearAll();
#ifdef Q_OS_WIN
    if (qApp)
        qApp->removeNativeEventFilter(this);
#endif
}

bool HotkeyManager::parseBind(const QString& bind, Native& out)
{
#ifdef Q_OS_WIN
    const QStringList parts = bind.split('+', Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return false;

    quint32 mods = 0;
    QString keyToken;
    for (const QString& raw : parts) {
        const QString p = raw.trimmed();
        const QString low = p.toLower();
        if (low == "ctrl" || low == "control")      mods |= MOD_CONTROL;
        else if (low == "alt")                      mods |= MOD_ALT;
        else if (low == "shift")                    mods |= MOD_SHIFT;
        else if (low == "win" || low == "meta")     mods |= MOD_WIN;
        else                                        keyToken = p;  // сама клавиша
    }
    if (keyToken.isEmpty())
        return false;

    quint32 vk = 0;
    if (keyToken.size() == 1) {
        const QChar c = keyToken.at(0).toUpper();
        const ushort u = c.unicode();
        if ((u >= '0' && u <= '9') || (u >= 'A' && u <= 'Z'))
            vk = u;
        else
            return false;
    } else if (keyToken.at(0).toUpper() == 'F') {
        bool ok = false;
        const int n = keyToken.mid(1).toInt(&ok);
        if (!ok || n < 1 || n > 24)
            return false;
        vk = VK_F1 + (n - 1);
    } else {
        return false;
    }

    out.mods = mods;
    out.vk = vk;
    return true;
#else
    Q_UNUSED(bind)
    Q_UNUSED(out)
    return false;
#endif
}

bool HotkeyManager::setHotkey(const QString& id, const QString& bind)
{
    clearHotkey(id);

    if (bind.trimmed().isEmpty())
        return true;

#ifdef Q_OS_WIN
    Native n;
    if (!parseBind(bind, n))
        return false;

    const int hotkeyId = m_nextHotkeyId++;
    if (!RegisterHotKey(nullptr, hotkeyId, n.mods | MOD_NOREPEAT, n.vk))
        return false;

    m_idToHotkey.insert(id, hotkeyId);
    m_hotkeyToId.insert(hotkeyId, id);
    return true;
#else
    Q_UNUSED(bind)
    return false;
#endif
}

void HotkeyManager::clearHotkey(const QString& id)
{
    auto it = m_idToHotkey.find(id);
    if (it == m_idToHotkey.end())
        return;

    const int hotkeyId = it.value();
#ifdef Q_OS_WIN
    UnregisterHotKey(nullptr, hotkeyId);
#endif
    m_hotkeyToId.remove(hotkeyId);
    m_idToHotkey.erase(it);
}

void HotkeyManager::clearAll()
{
    const QList<QString> ids = m_idToHotkey.keys();
    for (const QString& id : ids)
        clearHotkey(id);
}

bool HotkeyManager::nativeEventFilter(const QByteArray& eventType,
                                      void* message, qintptr* result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(result)
#ifdef Q_OS_WIN
    auto* msg = static_cast<MSG*>(message);
    if (msg && msg->message == WM_HOTKEY) {
        const int hotkeyId = static_cast<int>(msg->wParam);
        const auto it = m_hotkeyToId.constFind(hotkeyId);
        if (it != m_hotkeyToId.constEnd()) {
            emit activated(it.value());
            return true;
        }
    }
#else
    Q_UNUSED(message)
#endif
    return false;
}
