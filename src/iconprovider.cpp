#include "iconprovider.h"

#include <QFileInfo>
#include <QUrl>
#include <QPainter>

QIcon iconForSource(const QString& source)
{
    static QFileIconProvider provider;
    static const QStringList imageExts = {
        QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
        QStringLiteral("ico"), QStringLiteral("bmp"), QStringLiteral("gif"),
        QStringLiteral("svg"), QStringLiteral("webp"),
    };

    const QFileInfo info(source);
    if (info.isFile() && imageExts.contains(info.suffix().toLower())) {
        const QIcon icon(info.absoluteFilePath());
        if (!icon.isNull() && !icon.availableSizes().isEmpty())
            return icon;
        // Некоторые форматы QIcon не перечисляет размеры — пробуем как картинку.
        const QPixmap pm(info.absoluteFilePath());
        if (!pm.isNull())
            return QIcon(pm);
    }
    if (info.exists())
        return provider.icon(info);
    if (source.contains(QLatin1String("://")))
        return provider.icon(QFileIconProvider::Network);
    return QIcon();
}

IconProvider::IconProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap IconProvider::requestPixmap(const QString& id, QSize* size,
                                    const QSize& requestedSize)
{
    const int edge = requestedSize.width() > 0 ? requestedSize.width() : 32;

    const QString path = QUrl::fromPercentEncoding(id.toUtf8());

    const QString cacheKey = path + QLatin1Char('@') + QString::number(edge);
    if (const auto it = m_cache.constFind(cacheKey); it != m_cache.constEnd()) {
        if (size)
            *size = it->size();
        return *it;
    }

    QPixmap pm;
    const QIcon icon = iconForSource(path);
    if (!icon.isNull())
        pm = icon.pixmap(edge, edge);

    if (pm.isNull()) {
        pm = QPixmap(edge, edge);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0x3a, 0x3f, 0x4b));
        p.drawRoundedRect(pm.rect().adjusted(2, 2, -2, -2), 6, 6);
        p.end();
    }

    m_cache.insert(cacheKey, pm);
    if (size)
        *size = pm.size();
    return pm;
}
