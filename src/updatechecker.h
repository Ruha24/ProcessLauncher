#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QString>

class QNetworkAccessManager;

class UpdateChecker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
public:
    explicit UpdateChecker(QObject* parent = nullptr);

    QString currentVersion() const;

    Q_INVOKABLE void checkForUpdates();

signals:
    // Новая версия найдена: её номер и URL страницы релиза
    void updateAvailable(const QString& version, const QString& url);
    void upToDate();
    void checkFailed(const QString& reason);

private:
    static int compareVersions(const QString& a, const QString& b);
    static QString normalize(const QString& v);

    QNetworkAccessManager* m_net = nullptr;
};

#endif
