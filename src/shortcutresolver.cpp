#include "shortcutresolver.h"

#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QUrl>
#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shlobj.h>
#include <objbase.h>
#endif

namespace {

bool isLnk(const QString& path)
{
    return QFileInfo(path).suffix().compare(QLatin1String("lnk"),
                                            Qt::CaseInsensitive) == 0;
}

bool isUrl(const QString& path)
{
    return QFileInfo(path).suffix().compare(QLatin1String("url"),
                                            Qt::CaseInsensitive) == 0;
}

#ifdef Q_OS_WIN
QString resolveLnk(const QString& path, bool* ok)
{
    QString result;
    const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool needUninit = (init == S_OK || init == S_FALSE);

    IShellLinkW* link = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_IShellLinkW,
                                   reinterpret_cast<void**>(&link)))) {
        IPersistFile* file = nullptr;
        if (SUCCEEDED(link->QueryInterface(IID_IPersistFile,
                                           reinterpret_cast<void**>(&file)))) {
            if (SUCCEEDED(file->Load(
                    reinterpret_cast<const wchar_t*>(path.utf16()), STGM_READ))) {
                link->Resolve(nullptr, SLR_NO_UI | SLR_NOUPDATE);

                wchar_t target[MAX_PATH] = {0};
                if (SUCCEEDED(link->GetPath(target, MAX_PATH, nullptr, 0))) {
                    const QString t = QString::fromWCharArray(target);
                    if (!t.isEmpty())
                        result = t;
                }
            }
            file->Release();
        }
        link->Release();
    }

    if (needUninit)
        CoUninitialize();

    if (ok)
        *ok = !result.isEmpty();
    return result;
}
#endif

QString resolveUrlFile(const QString& path, bool* ok)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (ok) *ok = false;
        return QString();
    }

    QString urlValue;
    QTextStream in(&f);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.startsWith(QLatin1String("URL="), Qt::CaseInsensitive)) {
            urlValue = line.mid(4).trimmed();
            break;
        }
    }
    f.close();

    if (urlValue.isEmpty()) {
        if (ok) *ok = false;
        return QString();
    }

    const QUrl url(urlValue);
    QString localPath;
    if (url.isLocalFile())
        localPath = url.toLocalFile();
    else if (!url.scheme().isEmpty() && url.scheme() != QLatin1String("file"))
        localPath = QString();
    else
        localPath = urlValue;

    const bool valid = !localPath.isEmpty() && QFileInfo(localPath).isFile();
    if (ok) *ok = valid;
    return valid ? localPath : QString();
}

}

namespace ShortcutResolver {

bool isShortcut(const QString& path)
{
    return isLnk(path) || isUrl(path);
}

QString resolve(const QString& path, bool* ok)
{
    if (isLnk(path)) {
#ifdef Q_OS_WIN
        return resolveLnk(path, ok);
#else
        if (ok) *ok = false;
        return QString();
#endif
    }
    if (isUrl(path))
        return resolveUrlFile(path, ok);

    if (ok) *ok = false;
    return QString();
}

}
