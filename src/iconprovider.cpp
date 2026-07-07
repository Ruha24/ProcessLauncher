#include "iconprovider.h"

#include <QFileInfo>
#include <QUrl>
#include <QPainter>

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
    const QFileInfo info(path);
    if (info.exists()) {
        const QIcon icon = m_fileIcons.icon(info);
        if (!icon.isNull())
            pm = icon.pixmap(edge, edge);
    }

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
