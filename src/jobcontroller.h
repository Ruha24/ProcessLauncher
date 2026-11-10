#ifndef JOBCONTROLLER_H
#define JOBCONTROLLER_H

#include <QtGlobal>

class JobController
{
public:
    JobController();
    ~JobController();

    JobController(const JobController&) = delete;
    JobController& operator=(const JobController&) = delete;

    bool assign(qint64 pid);

    bool isAlive() const;

    void terminate();

    // Должны ли процессы умирать вместе с лаунчером (в том числе если он
    // упал или был убит). Можно менять и для уже запущенных процессов.
    void setKillOnClose(bool on);

    // Для процессов, которые не удалось поместить в job (например, запущенных
    // с повышением прав): жив ли ещё процесс с таким PID.
    static bool isPidAlive(qint64 pid);

private:
    void closeHandle();

    bool m_killOnClose = true;

#ifdef Q_OS_WIN
    void* m_job = nullptr;
#else
    qint64 m_pid = 0;
#endif
};

#endif
