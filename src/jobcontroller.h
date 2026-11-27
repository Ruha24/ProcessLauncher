#ifndef JOBCONTROLLER_H
#define JOBCONTROLLER_H

#include <QtGlobal>
#include <QDateTime>
#include <QList>
#include <QSet>
#include <QString>

class JobController
{
public:
    // Сколько ресурсов занимают процессы: процессорное время с запуска (нс)
    // и память. -1 — узнать не удалось.
    struct Usage {
        qint64 cpuNs = -1;
        qint64 memBytes = -1;
    };

    JobController();
    ~JobController();

    JobController(const JobController&) = delete;
    JobController& operator=(const JobController&) = delete;

    bool assign(qint64 pid);

#ifdef Q_OS_WIN
    // Принимает во владение дескриптор процесса (например, из ShellExecuteEx
    // при запуске от администратора). Если процесс удалось поместить в job —
    // работаем как обычно; если нет — следим за ним и останавливаем его
    // через этот дескриптор. Дескриптор закрывается контроллером.
    void adoptProcessHandle(void* process);
#endif

    bool isAlive() const;

    // Все процессы под контролем: в Windows — весь job (с дочерними),
    // в macOS — группа процессов.
    Usage usage() const;
    static Usage pidUsage(qint64 pid);

    void terminate();

    // Должны ли процессы умирать вместе с лаунчером (в том числе если он
    // упал или был убит). Можно менять и для уже запущенных процессов.
    void setKillOnClose(bool on);

    // Для процессов, которые не удалось поместить в job (например, запущенных
    // с повышением прав): жив ли ещё процесс с таким PID.
    static bool isPidAlive(qint64 pid);

    // Пытается завершить процесс по PID. false — нет прав (или PID неверный).
    static bool terminatePid(qint64 pid);

    // PID всех процессов под контролем (в Windows — весь job).
    QList<qint64> pids() const;

    struct ProcessInfo {
        qint64  pid = 0;
        QString path;   // полный путь к исполняемому файлу, разделители «/»
    };
    // Процессы системы, к пути которых у нас есть доступ. fileNames (в нижнем
    // регистре) — только процессы с такими именами файлов; в Windows так не
    // приходится открывать каждый процесс. В macOS имя процесса внутри .app
    // не совпадает с именем бандла, поэтому там фильтр не применяется.
    static QList<ProcessInfo> runningProcesses(const QSet<QString>& fileNames);
    // Когда процесс запустился; пустая дата — неизвестно.
    static QDateTime startTime(qint64 pid);

private:
    void closeHandle();

    bool m_killOnClose = true;

#ifdef Q_OS_WIN
    void* m_job = nullptr;
    void* m_process = nullptr;   // запасной путь, если job недоступен
#else
    qint64 m_pid = 0;
#endif
};

#endif
