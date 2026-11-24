#ifndef ICONPROVIDER_H
#define ICONPROVIDER_H

#include <QQuickImageProvider>
#include <QFileIconProvider>
#include <QHash>
#include <QPixmap>

#include <QIcon>

// Иконка для источника: картинка (.png/.ico/…) — как есть, файл или папка —
// системная иконка, ссылка — значок сети. Пустая, если ничего не нашлось.
QIcon iconForSource(const QString& source);

class IconProvider : public QQuickImageProvider
{
public:
    IconProvider();

    QPixmap requestPixmap(const QString& id, QSize* size,
                          const QSize& requestedSize) override;

private:
    QFileIconProvider     m_fileIcons;
    QHash<QString, QPixmap> m_cache;
};

#endif
