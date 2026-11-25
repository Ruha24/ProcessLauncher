#ifndef LANGUAGEMANAGER_H
#define LANGUAGEMANAGER_H

#include <QObject>
#include <QString>
#include <QTranslator>

class QQmlEngine;
class WindowState;

// Язык интерфейса: "" — как в системе, "en" или "ru". Переключается на лету:
// QML пересчитывает qsTr, а C++ строит строки заново (меню трея — при
// каждом открытии, модели — по languageChanged).
class LanguageManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)

public:
    explicit LanguageManager(WindowState* settings, QObject* parent = nullptr);

    void setEngine(QQmlEngine* engine);

    QString language() const;
    void setLanguage(const QString& language);

signals:
    void languageChanged();

private:
    void apply();

    WindowState* m_settings = nullptr;
    QQmlEngine*  m_engine = nullptr;
    QTranslator  m_translator;
    bool         m_installed = false;
    QString      m_language;
};

#endif
