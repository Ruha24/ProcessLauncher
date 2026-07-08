#include "autostartmanager.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QFile>
#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shlobj.h>
#include <objbase.h>
#endif

namespace {
const QString kLinkName = QStringLiteral("ProcessLauncher.lnk");
}

AutostartManager::AutostartManager(QObject* parent)
    : QObject(parent)
{
}

bool AutostartManager::isAvailable() const
{
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    return true;
#else
    return false;
#endif
}

QString AutostartManager::shortcutPath() const
{
#ifdef Q_OS_WIN
    PWSTR raw = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_Startup, 0, nullptr, &raw) != S_OK) {
        if (raw)
            CoTaskMemFree(raw);
        return QString();
    }
    const QString dir = QString::fromWCharArray(raw);
    CoTaskMemFree(raw);
    return QDir(dir).absoluteFilePath(kLinkName);
#elif defined(Q_OS_MACOS)
    const QString home = QDir::homePath();
    return home + QStringLiteral("/Library/LaunchAgents/com.processlauncher.autostart.plist");
#else
    return QString();
#endif
}

bool AutostartManager::isEnabled() const
{
    const QString path = shortcutPath();
    return !path.isEmpty() && QFile::exists(path);
}

bool AutostartManager::createShortcut() const
{
#ifdef Q_OS_WIN
    const QString linkPath = shortcutPath();
    if (linkPath.isEmpty())
        return false;

    const QString exePath = QDir::toNativeSeparators(
        QCoreApplication::applicationFilePath());
    const QString workDir = QDir::toNativeSeparators(
        QFileInfo(exePath).absolutePath());

    const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool needUninit = (init == S_OK || init == S_FALSE);

    bool ok = false;
    IShellLinkW* link = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_IShellLinkW,
                                   reinterpret_cast<void**>(&link)))) {
        link->SetPath(reinterpret_cast<const wchar_t*>(exePath.utf16()));
        link->SetWorkingDirectory(
            reinterpret_cast<const wchar_t*>(workDir.utf16()));
        link->SetDescription(L"Process Launcher");

        IPersistFile* file = nullptr;
        if (SUCCEEDED(link->QueryInterface(IID_IPersistFile,
                                           reinterpret_cast<void**>(&file)))) {
            const HRESULT saved = file->Save(
                reinterpret_cast<const wchar_t*>(linkPath.utf16()), TRUE);
            ok = SUCCEEDED(saved);
            file->Release();
        }
        link->Release();
    }

    if (needUninit)
        CoUninitialize();
    return ok;
#elif defined(Q_OS_MACOS)
    const QString plistPath = shortcutPath();
    if (plistPath.isEmpty())
        return false;

    QDir().mkpath(QFileInfo(plistPath).absolutePath());

    const QString exePath = QCoreApplication::applicationFilePath();
    const QString plist =
        QStringLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
            "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
            "<plist version=\"1.0\">\n"
            "<dict>\n"
            "    <key>Label</key>\n"
            "    <string>com.processlauncher.autostart</string>\n"
            "    <key>ProgramArguments</key>\n"
            "    <array>\n"
            "        <string>%1</string>\n"
            "    </array>\n"
            "    <key>RunAtLoad</key>\n"
            "    <true/>\n"
            "</dict>\n"
            "</plist>\n").arg(exePath);

    QFile f(plistPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    f.write(plist.toUtf8());
    f.close();
    return true;
#else
    return false;
#endif
}

void AutostartManager::setEnabled(bool on)
{
    if (on == isEnabled())
        return;

    if (on) {
        createShortcut();
    } else {
        const QString path = shortcutPath();
        if (!path.isEmpty())
            QFile::remove(path);
    }
    emit enabledChanged();
}
