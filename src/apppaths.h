#ifndef APPPATHS_H
#define APPPATHS_H

#include <QString>

namespace AppPaths
{
bool isPortable();
QString dataDir();
QString configFile(const QString& fileName);
QString settingsFile();
}

#endif
