#include "updatechecker.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace {
// Текущая версия приложения. Поднимай при каждом релизе.
const QString kCurrentVersion = QStringLiteral("1.0");
// GitHub API последнего релиза.
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

QString UpdateChecker::normalize(const QString& v)
{
    QString s = v.trimmed();
    if (s.startsWith(QLatin1Char('v')) || s.startsWith(QLatin1Char('V')))
        s = s.mid(1);
    return s;
}

// Возвращает: >0 если a новее b, <0 если a старее, 0 если равны.
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

        if (latestTag.isEmpty()) {
            emit checkFailed(QStringLiteral("No release found"));
            return;
        }

        if (compareVersions(latestTag, kCurrentVersion) > 0)
            emit updateAvailable(normalize(latestTag), htmlUrl);
        else
            emit upToDate();
    });
}
