#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <QObject>
#include <QHash>
#include <QSet>
#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
class QProcess;
class QTimer;
QT_END_NAMESPACE

class JobController;
class HotkeyManager;

// Одна запись в списке программ (только метаданные — состояние запуска
// хранится отдельно, в группах процессов внутри ProcessManager).
struct ProcessEntry
{
    QString id;     // стабильный уникальный ключ (UUID)
    QString path;   // полный путь к исполняемому файлу
    QString name;   // отображаемое имя (basename)
    QString bind;   // горячая клавиша / метка
};

// Кроссплатформенный менеджер запуска/остановки программ.
//
// Каждый запуск помещается в «группу процессов» (job object на Windows),
// поэтому отслеживается ВСЁ дерево: TLauncher, запустивший java, считается
// запущенным, пока жив хоть один процесс дерева, даже если сам лаунчер вышел.
// Остановка убивает всю группу целиком.
//
// Готов к использованию из QML: это QObject с сигналами.
class ProcessManager : public QObject
{
    Q_OBJECT

public:
    explicit ProcessManager(QObject* parent = nullptr);
    ~ProcessManager() override;

    // Возвращает id созданной записи (пустую строку при некорректном пути).
    QString addProgram(const QString& path, const QString& bind = QString());
    void    removeProgram(const QString& id);
    void    setBind(const QString& id, const QString& bind);

    void start(const QString& id);
    void stop(const QString& id);
    void stopAll();

    bool isRunning(const QString& id) const;

    // Детерминированный порядок (по name, затем id) — удобно для UI/тестов.
    QList<ProcessEntry> entries() const;

    // Путь к файлу конфигурации (public — удобно для тестов/диагностики).
    QString configFilePath() const;

signals:
    void runStateChanged(const QString& id, bool running);
    void listChanged();
    void errorOccurred(const QString& id, const QString& message);

private slots:
    void pollJobs();   // периодически проверяет, не опустели ли группы

private:
    void save() const;
    void load();
    void syncHotkey(const QString& id);   // (пере)регистрирует хоткей по бинду

    QHash<QString, ProcessEntry>    m_entries;  // id -> метаданные
    QHash<QString, JobController*>  m_jobs;      // id -> живая группа процессов
    QSet<QString>                   m_untracked; // запущено, но группа недоступна
    QTimer*                         m_pollTimer = nullptr;
    HotkeyManager*                  m_hotkeys = nullptr;
};

#endif // PROCESSMANAGER_H
