#ifndef SHORTCUTRESOLVER_H
#define SHORTCUTRESOLVER_H

#include <QString>

namespace ShortcutResolver {

bool isShortcut(const QString& path);

QString resolve(const QString& path, bool* ok = nullptr);

}

#endif
