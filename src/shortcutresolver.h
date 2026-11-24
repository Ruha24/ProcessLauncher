#ifndef SHORTCUTRESOLVER_H
#define SHORTCUTRESOLVER_H

#include <QString>

namespace ShortcutResolver {

bool isShortcut(const QString& path);

QString resolve(const QString& path, bool* ok = nullptr);

// Иконка, указанная в самом ярлыке (IconFile= в .url — так делает Steam).
// Пусто, если её нет или файла не существует.
QString iconFile(const QString& path);

}

#endif
