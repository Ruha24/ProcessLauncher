#include "hotkeymanager.h"

#include <QGuiApplication>
#include <QStringList>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef Q_OS_WIN
HotkeyManager* HotkeyManager::s_instance = nullptr;

namespace {
LRESULT CALLBACK llKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
}
#endif

HotkeyManager::HotkeyManager(QObject* parent)
    : QObject(parent)
{
#ifdef Q_OS_WIN
    s_instance = this;
    installHook();
#endif
}

bool HotkeyManager::isValidBind(const QString& bind)
{
    const QString trimmed = bind.trimmed();
    if (trimmed.isEmpty())
        return false;

    const QStringList parts = trimmed.split('+', Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return false;

    QString keyToken;
    for (const QString& raw : parts) {
        const QString p = raw.trimmed();
        const QString low = p.toLower();
        if (low == "ctrl" || low == "control" || low == "alt"
            || low == "shift" || low == "win" || low == "meta")
            continue;
        if (!keyToken.isEmpty())
            return false;
        keyToken = p;
    }
    if (keyToken.isEmpty())
        return false;

    if (keyToken.size() == 1) {
        const QChar c = keyToken.at(0).toUpper();
        const ushort u = c.unicode();
        return (u >= '0' && u <= '9') || (u >= 'A' && u <= 'Z');
    }
    if (keyToken.at(0).toUpper() == 'F') {
        bool ok = false;
        const int n = keyToken.mid(1).toInt(&ok);
        return ok && n >= 1 && n <= 24;
    }
    return false;
}

HotkeyManager::~HotkeyManager()
{
    clearAll();
#ifdef Q_OS_WIN
    removeHook();
    if (s_instance == this)
        s_instance = nullptr;
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
        else                                        keyToken = p;
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

    Binding b;
    b.id = id;
    b.mods = n.mods;
    b.vk = n.vk;
    m_bindings.append(b);
    return true;
#else
    Q_UNUSED(bind)
    return false;
#endif
}

void HotkeyManager::clearHotkey(const QString& id)
{
    for (int i = m_bindings.size() - 1; i >= 0; --i) {
        if (m_bindings.at(i).id == id)
            m_bindings.removeAt(i);
    }
}

void HotkeyManager::clearAll()
{
    m_bindings.clear();
}

#ifdef Q_OS_WIN
namespace {
quint32 currentModifiers()
{
    quint32 mods = 0;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) mods |= MOD_CONTROL;
    if (GetAsyncKeyState(VK_MENU)    & 0x8000) mods |= MOD_ALT;
    if (GetAsyncKeyState(VK_SHIFT)   & 0x8000) mods |= MOD_SHIFT;
    if ((GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000))
        mods |= MOD_WIN;
    return mods;
}

LRESULT CALLBACK llKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && HotkeyManager::isHookInstance()
        && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        auto* kb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        if (kb)
            HotkeyManager::dispatchKey(kb->vkCode, currentModifiers());
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
}
#endif

void HotkeyManager::installHook()
{
#ifdef Q_OS_WIN
    if (m_hook)
        return;
    m_hook = SetWindowsHookExW(WH_KEYBOARD_LL, &llKeyboardProc,
                               GetModuleHandleW(nullptr), 0);
#endif
}

void HotkeyManager::removeHook()
{
#ifdef Q_OS_WIN
    if (m_hook) {
        UnhookWindowsHookEx(static_cast<HHOOK>(m_hook));
        m_hook = nullptr;
    }
#endif
}

bool HotkeyManager::handleKey(quint32 vk, quint32 activeMods)
{
    for (const Binding& b : m_bindings) {
        if (b.vk == vk && b.mods == activeMods) {
            emit activated(b.id);
            return true;
        }
    }
    return false;
}

#ifdef Q_OS_WIN
bool HotkeyManager::isHookInstance()
{
    return s_instance != nullptr;
}

void HotkeyManager::dispatchKey(quint32 vk, quint32 activeMods)
{
    if (s_instance)
        s_instance->handleKey(vk, activeMods);
}
#endif
