#include "updatechecker.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QProcess>
#include <QFileInfo>

namespace {

const QString kCurrentVersion = QStringLiteral("1.3");
const QString kApiUrl =
    QStringLiteral("https://api.github.com/repos/Ruha24/ProcessLauncher/releases/latest");
}

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , m_net(new QNetworkAccessManager(this))
{
}

QString UpdateChecker::currentVersion() const
{
    return kCurrentVersion;
}

bool UpdateChecker::canDownload() const
{
    return !m_assetUrl.isEmpty();
}

QString UpdateChecker::platformSuffix()
{
#if defined(Q_OS_WIN)
    return QStringLiteral(".exe");
#elif defined(Q_OS_MACOS)
    return QStringLiteral(".dmg");
#else
    return QString();
#endif
}

QString UpdateChecker::normalize(const QString& v)
{
    QString s = v.trimmed();
    if (s.startsWith(QLatin1Char('v')) || s.startsWith(QLatin1Char('V')))
        s = s.mid(1);
    return s;
}

int UpdateChecker::compareVersions(const QString& a, const QString& b)
{
    const QStringList pa = normalize(a).split(QLatin1Char('.'));
    const QStringList pb = normalize(b).split(QLatin1Char('.'));
    const int n = qMax(pa.size(), pb.size());
    for (int i = 0; i < n; ++i) {
        const int va = i < pa.size() ? pa.at(i).toInt() : 0;
        const int vb = i < pb.size() ? pb.at(i).toInt() : 0;
        if (va != vb)
            return va - vb;
    }
    return 0;
}

void UpdateChecker::checkForUpdates()
{
    QNetworkRequest req{QUrl(kApiUrl)};
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("ProcessLauncher-UpdateChecker"));
    req.setRawHeader("Accept", "application/vnd.github+json");

    QNetworkReply* reply = m_net->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit checkFailed(reply->errorString());
            return;
        }

        const QByteArray raw = reply->readAll();
        QJsonParseError perr{};
        const QJsonDocument doc = QJsonDocument::fromJson(raw, &perr);
        if (doc.isNull() || !doc.isObject()) {
            emit checkFailed(QStringLiteral("Invalid response"));
            return;
        }

        const QJsonObject obj = doc.object();
        const QString latestTag = obj.value(QStringLiteral("tag_name")).toString();
        const QString htmlUrl = obj.value(QStringLiteral("html_url")).toString();
        const QString notes = obj.value(QStringLiteral("body")).toString();

        if (latestTag.isEmpty()) {
            emit checkFailed(QStringLiteral("No release found"));
            return;
        }

        m_assetUrl.clear();
        m_assetName.clear();
        const QString suffix = platformSuffix();
        if (!suffix.isEmpty()) {
            const QJsonArray assets = obj.value(QStringLiteral("assets")).toArray();
            for (const QJsonValue& av : assets) {
                const QJsonObject a = av.toObject();
                const QString name = a.value(QStringLiteral("name")).toString();
                if (name.endsWith(suffix, Qt::CaseInsensitive)) {
                    m_assetUrl = a.value(QStringLiteral("browser_download_url")).toString();
                    m_assetName = name;
                    break;
                }
            }
        }
        emit assetChanged();

        if (compareVersions(latestTag, kCurrentVersion) > 0)
            emit updateAvailable(normalize(latestTag), htmlUrl, notes);
        else
            emit upToDate();
    });
}

void UpdateChecker::downloadUpdate()
{
    if (m_assetUrl.isEmpty()) {
        emit downloadFailed(QStringLiteral("No installer available for this OS"));
        return;
    }

    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QDir().mkpath(dir);
    const QString target = QDir(dir).absoluteFilePath(
        m_assetName.isEmpty() ? QStringLiteral("ProcessLauncher-update") : m_assetName);

    auto* file = new QFile(target);
    if (!file->open(QIODevice::WriteOnly)) {
        delete file;
        emit downloadFailed(QStringLiteral("Cannot write to Downloads folder"));
        return;
    }

    QNetworkRequest req{QUrl(m_assetUrl)};
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("ProcessLauncher-UpdateChecker"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = m_net->get(req);

    connect(reply, &QNetworkReply::readyRead, this, [reply, file]() {
        file->write(reply->readAll());
    });

    connect(reply, &QNetworkReply::downloadProgress, this,
            [this](qint64 received, qint64 total) {
                if (total > 0)
                    emit downloadProgress(double(received) / double(total));
            });

    connect(reply, &QNetworkReply::finished, this, [this, reply, file, target]() {
        reply->deleteLater();
        file->write(reply->readAll());
        file->close();
        file->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QFile::remove(target);
            emit downloadFailed(reply->errorString());
            return;
        }

        m_downloadedPath = target;
        emit downloadFinished(target);
    });
}

void UpdateChecker::runInstaller()
{
    if (m_downloadedPath.isEmpty() || !QFileInfo::exists(m_downloadedPath))
        return;

    QDesktopServices::openUrl(QUrl::fromLocalFile(m_downloadedPath));
}
