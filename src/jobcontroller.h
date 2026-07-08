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

private:
    void closeHandle();

#ifdef Q_OS_WIN
    void* m_job = nullptr;
#else
    qint64 m_pid = 0;
#endif
};

#endif
