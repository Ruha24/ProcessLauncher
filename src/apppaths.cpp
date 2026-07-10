#include "apppaths.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>

namespace {
const QString kMarker = QStringLiteral("portable.ini");
}

namespace AppPaths
{
bool isPortable()
{
    const QString exeDir = QCoreApplication::applicationDirPath();
    return QFileInfo::exists(exeDir + QLatin1Char('/') + kMarker);
}

QString dataDir()
{
    QString dir;
    if (isPortable())
        dir = QCoreApplication::applicationDirPath() + QStringLiteral("/data");
    else
        dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir;
}

QString configFile(const QString& fileName)
{
    return dataDir() + QLatin1Char('/') + fileName;
}

QString settingsFile()
{
    return dataDir() + QStringLiteral("/settings.ini");
}
}
