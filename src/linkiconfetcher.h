#ifndef LINKICONFETCHER_H
#define LINKICONFETCHER_H

#include <QObject>
#include <QList>
#include <QUrl>

class QNetworkAccessManager;

// Находит иконку для ссылки и сохраняет её в папку настроек (icons/):
//  - steam://rungameid/<id> — маленькая иконка игры из кеша Steam (локально);
//  - http(s)://… — иконка сайта: <link rel="icon"> со страницы или /favicon.ico.
// Результат приходит сигналом finished(); ссылки других видов не поддерживаются.
class LinkIconFetcher : public QObject
{
    Q_OBJECT

public:
    explicit LinkIconFetcher(QObject* parent = nullptr);

    static bool canFetch(const QString& link);
    void fetch(const QString& id, const QString& link);

signals:
    // iconPath пуст, если иконку найти не удалось.
    void finished(const QString& id, const QString& iconPath);

private:
    QString steamIcon(const QString& link) const;
    void fetchPage(const QString& id, const QUrl& url);
    void tryCandidates(const QString& id, const QString& host, QList<QUrl> candidates);
    static QString saveIcon(const QImage& image, const QString& baseName);

    QNetworkAccessManager* m_net = nullptr;
};

#endif
