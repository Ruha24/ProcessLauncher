#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QString>

class QNetworkAccessManager;

class UpdateChecker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(bool canDownload READ canDownload NOTIFY assetChanged)
public:
    explicit UpdateChecker(QObject* parent = nullptr);

    QString currentVersion() const;
    bool    canDownload() const;

    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void downloadUpdate();
    Q_INVOKABLE void runInstaller();

signals:
    void updateAvailable(const QString& version, const QString& url);
    void upToDate();
    void checkFailed(const QString& reason);
    void assetChanged();

    void downloadProgress(double fraction);
    void downloadFinished(const QString& path);
    void downloadFailed(const QString& reason);

private:
    static int compareVersions(const QString& a, const QString& b);
    static QString normalize(const QString& v);
    static QString platformSuffix();
    QNetworkAccessManager* m_net = nullptr;
    QString m_assetUrl;
    QString m_assetName;
    QString m_downloadedPath;
};

#endif
