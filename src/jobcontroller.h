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

    void* m_job = nullptr;
};

#endif
