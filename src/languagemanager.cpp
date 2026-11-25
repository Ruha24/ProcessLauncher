#include "languagemanager.h"
#include "windowstate.h"

#include <QCoreApplication>
#include <QLocale>
#include <QQmlEngine>

namespace {
const QString kKey = QStringLiteral("language");

bool isSupported(const QString& language)
{
    return language.isEmpty() || language == QLatin1String("en")
        || language == QLatin1String("ru");
}
}

LanguageManager::LanguageManager(WindowState* settings, QObject* parent)
    : QObject(parent)
    , m_settings(settings)
{
    const QString saved = m_settings->value(kKey, QString()).toString();
    m_language = isSupported(saved) ? saved : QString();
    apply();
}

void LanguageManager::setEngine(QQmlEngine* engine)
{
    m_engine = engine;
}

QString LanguageManager::language() const
{
    return m_language;
}

void LanguageManager::setLanguage(const QString& language)
{
    if (!isSupported(language) || language == m_language)
        return;
    m_language = language;
    m_settings->setValue(kKey, m_language);
    apply();
    emit languageChanged();
}

void LanguageManager::apply()
{
    if (m_installed) {
        QCoreApplication::removeTranslator(&m_translator);
        m_installed = false;
    }

    // Исходные строки — английские, для "en" перевод не нужен. Системный язык —
    // первый язык интерфейса ОС: иначе QTranslator пошёл бы дальше по списку и
    // включил бы русский, скажем, у английской Windows с русским вторым языком.
    if (m_language != QLatin1String("en")) {
        const QLocale locale = m_language.isEmpty()
            ? QLocale(QLocale::system().uiLanguages().value(0))
            : QLocale(m_language);
        if (m_translator.load(locale, QStringLiteral("ProcessLauncher"), QStringLiteral("_"),
                              QStringLiteral(":/i18n")))
            m_installed = QCoreApplication::installTranslator(&m_translator);
    }

    if (m_engine)
        m_engine->retranslate();
}
