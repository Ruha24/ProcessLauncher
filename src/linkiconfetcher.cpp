#include "linkiconfetcher.h"
#include "apppaths.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QImage>
#include <QImageReader>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTimer>
#include <QCoreApplication>
#include <algorithm>

namespace {

constexpr int kTimeoutMs = 10000;
constexpr qint64 kMaxPageBytes = 1024 * 1024;
constexpr int kIconSize = 64;

const QRegularExpression& steamAppIdRe()
{
    // steam://rungameid/730, steam://run/730, steam://launch/730
    static const QRegularExpression re(
        QStringLiteral("^steam://(?:rungameid|run|launch)/(\\d+)"),
        QRegularExpression::CaseInsensitiveOption);
    return re;
}

QString steamRoot()
{
#ifdef Q_OS_WIN
    const QSettings reg(QStringLiteral("HKEY_CURRENT_USER\\Software\\Valve\\Steam"),
                        QSettings::NativeFormat);
    const QString path = reg.value(QStringLiteral("SteamPath")).toString();
    if (!path.isEmpty() && QFileInfo(path).isDir())
        return path;
    return QStringLiteral("C:/Program Files (x86)/Steam");
#elif defined(Q_OS_MACOS)
    return QDir::homePath() + QStringLiteral("/Library/Application Support/Steam");
#else
    return QDir::homePath() + QStringLiteral("/.steam/steam");
#endif
}

QNetworkRequest makeRequest(const QUrl& url)
{
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("Mozilla/5.0 (compatible; ProcessLauncher/%1)")
                      .arg(QCoreApplication::applicationVersion()));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setTransferTimeout(kTimeoutMs);
    return req;
}

QString attr(const QString& tag, const QString& name)
{
    const QRegularExpression re(
        QStringLiteral("\\b%1\\s*=\\s*(?:\"([^\"]*)\"|'([^']*)'|([^\\s>]+))").arg(name),
        QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(tag);
    if (!m.hasMatch())
        return QString();
    for (int i = 1; i <= 3; ++i)
        if (!m.captured(i).isNull())
            return m.captured(i).trimmed();
    return QString();
}

// Иконки, объявленные на странице, — от самой подходящей к худшей.
QList<QUrl> iconCandidates(const QByteArray& html, const QUrl& base)
{
    struct Cand { QUrl url; int score; };
    QList<Cand> found;

    const bool svgOk = QImageReader::supportedImageFormats().contains("svg");
    static const QRegularExpression linkRe(QStringLiteral("<link\\b[^>]*>"),
                                           QRegularExpression::CaseInsensitiveOption);
    const QString text = QString::fromUtf8(html);
    auto it = linkRe.globalMatch(text);
    while (it.hasNext()) {
        const QString tag = it.next().captured(0);
        const QString rel = attr(tag, QStringLiteral("rel")).toLower();
        const QString href = attr(tag, QStringLiteral("href"));
        if (href.isEmpty() || !rel.contains(QLatin1String("icon")))
            continue;
        if (rel.contains(QLatin1String("mask")))
            continue;   // safari mask-icon — одноцветный силуэт

        const QUrl url = base.resolved(QUrl(href));
        const bool isSvg = url.path().endsWith(QLatin1String(".svg"), Qt::CaseInsensitive)
                        || attr(tag, QStringLiteral("type")).contains(QLatin1String("svg"));
        if (isSvg && !svgOk)
            continue;

        int score = 16;
        static const QRegularExpression sizeRe(QStringLiteral("(\\d+)x\\d+"));
        const auto sm = sizeRe.match(attr(tag, QStringLiteral("sizes")));
        if (sm.hasMatch())
            score = sm.captured(1).toInt();
        if (rel.contains(QLatin1String("apple-touch-icon")))
            score = qMax(score, 180);
        if (isSvg)
            score = 128;
        // Слишком большие картинки не нужны: в списке иконка 28 px.
        if (score > 256)
            score = 256 - score / 100;
        found.append({url, score});
    }

    std::stable_sort(found.begin(), found.end(),
                     [](const Cand& a, const Cand& b) { return a.score > b.score; });
    QList<QUrl> result;
    for (const Cand& c : found)
        result.append(c.url);
    return result;
}

}

LinkIconFetcher::LinkIconFetcher(QObject* parent)
    : QObject(parent)
    , m_net(new QNetworkAccessManager(this))
{
}

bool LinkIconFetcher::canFetch(const QString& link)
{
    if (steamAppIdRe().match(link).hasMatch())
        return true;
    const QUrl url(link);
    return (url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https"))
        && !url.host().isEmpty();
}

void LinkIconFetcher::fetch(const QString& id, const QString& link)
{
    if (steamAppIdRe().match(link).hasMatch()) {
        const QString icon = steamIcon(link);
        // Отвечаем асинхронно, как и для сайтов, — вызывающему так проще.
        QTimer::singleShot(0, this, [this, id, icon]() { emit finished(id, icon); });
        return;
    }

    const QUrl url(link);
    if (!canFetch(link)) {
        QTimer::singleShot(0, this, [this, id]() { emit finished(id, QString()); });
        return;
    }
    fetchPage(id, url);
}

QString LinkIconFetcher::steamIcon(const QString& link) const
{
    const QString appId = steamAppIdRe().match(link).captured(1);
    const QDir cache(steamRoot() + QStringLiteral("/appcache/librarycache"));

    // Новый формат кеша: librarycache/<id>/<хеш>.jpg — маленькая иконка игры.
    QString source;
    const QDir appDir(cache.filePath(appId));
    static const QRegularExpression hashRe(QStringLiteral("^[0-9a-f]{40}\\.jpg$"));
    const QFileInfoList files = appDir.entryInfoList(QDir::Files);
    for (const QFileInfo& fi : files) {
        if (hashRe.match(fi.fileName()).hasMatch()) {
            source = fi.absoluteFilePath();
            break;
        }
    }
    // Старый формат: librarycache/<id>_icon.jpg.
    if (source.isEmpty() && cache.exists(appId + QStringLiteral("_icon.jpg")))
        source = cache.filePath(appId + QStringLiteral("_icon.jpg"));
    if (source.isEmpty())
        return QString();

    // Копируем к себе: Steam может почистить свой кеш.
    const QImage image(source);
    return image.isNull() ? QString() : saveIcon(image, QStringLiteral("steam_") + appId);
}

void LinkIconFetcher::fetchPage(const QString& id, const QUrl& url)
{
    QNetworkReply* reply = m_net->get(makeRequest(url));
    connect(reply, &QNetworkReply::downloadProgress, reply, [reply](qint64 got, qint64) {
        if (got > kMaxPageBytes)
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, id, url, reply]() {
        reply->deleteLater();
        const QUrl finalUrl = reply->url().isValid() ? reply->url() : url;

        QList<QUrl> candidates;
        if (reply->error() == QNetworkReply::NoError)
            candidates = iconCandidates(reply->read(kMaxPageBytes), finalUrl);

        // Запасной вариант — /favicon.ico в корне сайта.
        QUrl favicon = finalUrl;
        favicon.setPath(QStringLiteral("/favicon.ico"));
        favicon.setQuery(QString());
        favicon.setFragment(QString());
        if (!candidates.contains(favicon))
            candidates.append(favicon);

        tryCandidates(id, finalUrl.host(), candidates);
    });
}

void LinkIconFetcher::tryCandidates(const QString& id, const QString& host,
                                    QList<QUrl> candidates)
{
    if (candidates.isEmpty()) {
        emit finished(id, QString());
        return;
    }

    const QUrl next = candidates.takeFirst();
    QNetworkReply* reply = m_net->get(makeRequest(next));
    connect(reply, &QNetworkReply::finished, this,
            [this, id, host, candidates, reply]() {
        reply->deleteLater();
        QImage image;
        if (reply->error() == QNetworkReply::NoError)
            image.loadFromData(reply->readAll());

        if (image.isNull() || image.width() < 8) {
            tryCandidates(id, host, candidates);
            return;
        }

        QString base = host;
        if (base.startsWith(QLatin1String("www.")))
            base = base.mid(4);
        static const QRegularExpression bad(QStringLiteral("[^A-Za-z0-9._-]"));
        base.replace(bad, QStringLiteral("_"));
        emit finished(id, saveIcon(image, QStringLiteral("site_") + base));
    });
}

QString LinkIconFetcher::saveIcon(const QImage& image, const QString& baseName)
{
    const QString dir = AppPaths::dataDir() + QStringLiteral("/icons");
    QDir().mkpath(dir);
    const QString path = dir + QLatin1Char('/') + baseName + QStringLiteral(".png");

    QImage out = image;
    if (out.width() > kIconSize || out.height() > kIconSize)
        out = out.scaled(kIconSize, kIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return out.save(path, "PNG") ? path : QString();
}
