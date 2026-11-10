#include "hotkeymanager.h"

#include <QStringList>
#include <utility>

#if defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_OS_MACOS)
#include <Carbon/Carbon.h>
#endif

namespace {

// Модификаторы в независимом от системы виде.
enum GenericMod : quint32 {
    ModCtrl  = 0x1,
    ModAlt   = 0x2,
    ModShift = 0x4,
    ModMeta  = 0x8,   // Win на Windows, Cmd на macOS
};

struct ParsedBind {
    quint32 mods = 0;
    QString key;      // "A".."Z", "0".."9", "F1".."F24"
};

bool tokenize(const QString& bind, ParsedBind& out)
{
    const QStringList parts = bind.trimmed().split(QLatin1Char('+'),
                                                   Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return false;

    ParsedBind result;
    for (const QString& raw : parts) {
        const QString p = raw.trimmed();
        const QString low = p.toLower();
        if (low == QLatin1String("ctrl") || low == QLatin1String("control")) {
            result.mods |= ModCtrl;
        } else if (low == QLatin1String("alt") || low == QLatin1String("option")
                   || low == QLatin1String("opt")) {
            result.mods |= ModAlt;
        } else if (low == QLatin1String("shift")) {
            result.mods |= ModShift;
        } else if (low == QLatin1String("win") || low == QLatin1String("meta")
                   || low == QLatin1String("cmd") || low == QLatin1String("command")) {
            result.mods |= ModMeta;
        } else {
            if (!result.key.isEmpty())
                return false;   // две обычные клавиши в одной привязке
            result.key = p.toUpper();
        }
    }
    if (result.key.isEmpty())
        return false;

    if (result.key.size() == 1) {
        const ushort u = result.key.at(0).unicode();
        if (!((u >= '0' && u <= '9') || (u >= 'A' && u <= 'Z')))
            return false;
    } else if (result.key.at(0) == QLatin1Char('F')) {
        bool ok = false;
        const int n = result.key.mid(1).toInt(&ok);
        if (!ok || n < 1 || n > 24)
            return false;
    } else {
        return false;
    }

    out = result;
    return true;
}

#if defined(Q_OS_MACOS)
// Коды клавиш macOS привязаны к физическим клавишам и идут не по порядку,
// поэтому нужна таблица.
bool macKeyCode(const QString& key, quint32& code)
{
    static const QHash<QString, quint32> map = {
        {QStringLiteral("A"), kVK_ANSI_A}, {QStringLiteral("B"), kVK_ANSI_B},
        {QStringLiteral("C"), kVK_ANSI_C}, {QStringLiteral("D"), kVK_ANSI_D},
        {QStringLiteral("E"), kVK_ANSI_E}, {QStringLiteral("F"), kVK_ANSI_F},
        {QStringLiteral("G"), kVK_ANSI_G}, {QStringLiteral("H"), kVK_ANSI_H},
        {QStringLiteral("I"), kVK_ANSI_I}, {QStringLiteral("J"), kVK_ANSI_J},
        {QStringLiteral("K"), kVK_ANSI_K}, {QStringLiteral("L"), kVK_ANSI_L},
        {QStringLiteral("M"), kVK_ANSI_M}, {QStringLiteral("N"), kVK_ANSI_N},
        {QStringLiteral("O"), kVK_ANSI_O}, {QStringLiteral("P"), kVK_ANSI_P},
        {QStringLiteral("Q"), kVK_ANSI_Q}, {QStringLiteral("R"), kVK_ANSI_R},
        {QStringLiteral("S"), kVK_ANSI_S}, {QStringLiteral("T"), kVK_ANSI_T},
        {QStringLiteral("U"), kVK_ANSI_U}, {QStringLiteral("V"), kVK_ANSI_V},
        {QStringLiteral("W"), kVK_ANSI_W}, {QStringLiteral("X"), kVK_ANSI_X},
        {QStringLiteral("Y"), kVK_ANSI_Y}, {QStringLiteral("Z"), kVK_ANSI_Z},
        {QStringLiteral("0"), kVK_ANSI_0}, {QStringLiteral("1"), kVK_ANSI_1},
        {QStringLiteral("2"), kVK_ANSI_2}, {QStringLiteral("3"), kVK_ANSI_3},
        {QStringLiteral("4"), kVK_ANSI_4}, {QStringLiteral("5"), kVK_ANSI_5},
        {QStringLiteral("6"), kVK_ANSI_6}, {QStringLiteral("7"), kVK_ANSI_7},
        {QStringLiteral("8"), kVK_ANSI_8}, {QStringLiteral("9"), kVK_ANSI_9},
        {QStringLiteral("F1"),  kVK_F1},  {QStringLiteral("F2"),  kVK_F2},
        {QStringLiteral("F3"),  kVK_F3},  {QStringLiteral("F4"),  kVK_F4},
        {QStringLiteral("F5"),  kVK_F5},  {QStringLiteral("F6"),  kVK_F6},
        {QStringLiteral("F7"),  kVK_F7},  {QStringLiteral("F8"),  kVK_F8},
        {QStringLiteral("F9"),  kVK_F9},  {QStringLiteral("F10"), kVK_F10},
        {QStringLiteral("F11"), kVK_F11}, {QStringLiteral("F12"), kVK_F12},
        {QStringLiteral("F13"), kVK_F13}, {QStringLiteral("F14"), kVK_F14},
        {QStringLiteral("F15"), kVK_F15}, {QStringLiteral("F16"), kVK_F16},
        {QStringLiteral("F17"), kVK_F17}, {QStringLiteral("F18"), kVK_F18},
        {QStringLiteral("F19"), kVK_F19}, {QStringLiteral("F20"), kVK_F20},
    };
    const auto it = map.constFind(key);
    if (it == map.constEnd())
        return false;   // F21–F24 на клавиатурах Mac не бывает
    code = it.value();
    return true;
}

const quint32 kHotKeySignature = 0x504C686B;   // 'PLhk'

OSStatus hotKeyHandler(EventHandlerCallRef, EventRef event, void* userData)
{
    EventHotKeyID hk{};
    const OSStatus st = GetEventParameter(event, kEventParamDirectObject,
                                          typeEventHotKeyID, nullptr,
                                          sizeof(hk), nullptr, &hk);
    if (st == noErr && userData && hk.signature == kHotKeySignature)
        static_cast<HotkeyManager*>(userData)->handleNativeHotkey(hk.id);
    return noErr;
}
#endif

#if defined(Q_OS_WIN)
LRESULT CALLBACK llKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
#endif

}

#if defined(Q_OS_WIN)
HotkeyManager* HotkeyManager::s_instance = nullptr;
#endif

HotkeyManager::HotkeyManager(QObject* parent)
    : QObject(parent)
{
#if defined(Q_OS_WIN)
    s_instance = this;
    installHook();
#elif defined(Q_OS_MACOS)
    const EventTypeSpec spec = {kEventClassKeyboard, kEventHotKeyPressed};
    EventHandlerRef handler = nullptr;
    if (InstallEventHandler(GetApplicationEventTarget(), &hotKeyHandler,
                            1, &spec, this, &handler) == noErr) {
        m_eventHandler = handler;
    }
#endif
}

HotkeyManager::~HotkeyManager()
{
    clearAll();
#if defined(Q_OS_WIN)
    removeHook();
    if (s_instance == this)
        s_instance = nullptr;
#elif defined(Q_OS_MACOS)
    if (m_eventHandler) {
        RemoveEventHandler(static_cast<EventHandlerRef>(m_eventHandler));
        m_eventHandler = nullptr;
    }
#endif
}

bool HotkeyManager::isValidBind(const QString& bind)
{
    ParsedBind parsed;
    return tokenize(bind, parsed);
}

bool HotkeyManager::parseBind(const QString& bind, quint32& mods, quint32& vk)
{
    ParsedBind parsed;
    if (!tokenize(bind, parsed))
        return false;

#if defined(Q_OS_WIN)
    mods = 0;
    if (parsed.mods & ModCtrl)  mods |= MOD_CONTROL;
    if (parsed.mods & ModAlt)   mods |= MOD_ALT;
    if (parsed.mods & ModShift) mods |= MOD_SHIFT;
    if (parsed.mods & ModMeta)  mods |= MOD_WIN;

    if (parsed.key.size() == 1)
        vk = parsed.key.at(0).unicode();   // VK-коды цифр и букв совпадают с ASCII
    else
        vk = VK_F1 + (parsed.key.mid(1).toInt() - 1);
    return true;
#elif defined(Q_OS_MACOS)
    mods = 0;
    if (parsed.mods & ModCtrl)  mods |= controlKey;
    if (parsed.mods & ModAlt)   mods |= optionKey;
    if (parsed.mods & ModShift) mods |= shiftKey;
    if (parsed.mods & ModMeta)  mods |= cmdKey;
    return macKeyCode(parsed.key, vk);
#else
    Q_UNUSED(mods)
    Q_UNUSED(vk)
    return false;
#endif
}

bool HotkeyManager::setHotkey(const QString& id, const QString& bind)
{
    clearHotkey(id);

    if (bind.trimmed().isEmpty())
        return true;

    Binding b;
    b.id = id;
    if (!parseBind(bind, b.mods, b.vk))
        return false;
    if (!registerNative(b))
        return false;

    m_bindings.append(b);
    return true;
}

void HotkeyManager::clearHotkey(const QString& id)
{
    for (int i = m_bindings.size() - 1; i >= 0; --i) {
        if (m_bindings.at(i).id == id) {
            unregisterNative(m_bindings[i]);
            m_bindings.removeAt(i);
        }
    }
}

void HotkeyManager::clearAll()
{
    for (Binding& b : m_bindings)
        unregisterNative(b);
    m_bindings.clear();
}

bool HotkeyManager::registerNative(Binding& b)
{
#if defined(Q_OS_MACOS)
    if (!m_eventHandler)
        return false;

    EventHotKeyID hkId;
    hkId.signature = kHotKeySignature;
    hkId.id = m_nextNativeId++;

    EventHotKeyRef ref = nullptr;
    // Система откажет, если сочетание уже занято или запрещено (например,
    // macOS 15 не даёт регистрировать хоткеи только с Option / Option+Shift).
    if (RegisterEventHotKey(b.vk, b.mods, hkId, GetApplicationEventTarget(),
                            0, &ref) != noErr || !ref) {
        return false;
    }
    b.nativeId = hkId.id;
    b.nativeRef = ref;
    return true;
#else
    // Windows: отдельной регистрации нет — привязки сверяет общий перехватчик.
    Q_UNUSED(b)
    return true;
#endif
}

void HotkeyManager::unregisterNative(Binding& b)
{
#if defined(Q_OS_MACOS)
    if (b.nativeRef) {
        UnregisterEventHotKey(static_cast<EventHotKeyRef>(b.nativeRef));
        b.nativeRef = nullptr;
    }
#else
    Q_UNUSED(b)
#endif
}

#if defined(Q_OS_MACOS)
void HotkeyManager::handleNativeHotkey(quint32 nativeId)
{
    for (const Binding& b : std::as_const(m_bindings)) {
        if (b.nativeId == nativeId) {
            emit activated(b.id);
            return;
        }
    }
}
#endif

#if defined(Q_OS_WIN)
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

void HotkeyManager::installHook()
{
    if (m_hook)
        return;
    m_hook = SetWindowsHookExW(WH_KEYBOARD_LL, &llKeyboardProc,
                               GetModuleHandleW(nullptr), 0);
}

void HotkeyManager::removeHook()
{
    if (m_hook) {
        UnhookWindowsHookEx(static_cast<HHOOK>(m_hook));
        m_hook = nullptr;
    }
}

bool HotkeyManager::handleKey(quint32 vk, quint32 activeMods)
{
    for (const Binding& b : std::as_const(m_bindings)) {
        if (b.vk == vk && b.mods == activeMods) {
            emit activated(b.id);
            return true;
        }
    }
    return false;
}

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
