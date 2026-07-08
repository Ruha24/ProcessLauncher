#include "jobcontroller.h"

#ifdef Q_OS_WIN
#include <windows.h>

JobController::JobController() = default;

JobController::~JobController()
{
    closeHandle();
}

bool JobController::assign(qint64 pid)
{
    if (!m_job) {
        HANDLE job = CreateJobObject(nullptr, nullptr);
        if (!job)
            return false;

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation,
                                     &info, sizeof(info))) {
            CloseHandle(job);
            return false;
        }
        m_job = job;
    }

    HANDLE hProc = OpenProcess(PROCESS_SET_QUOTA | PROCESS_TERMINATE, FALSE,
                               static_cast<DWORD>(pid));
    if (!hProc)
        return false;

    const BOOL ok = AssignProcessToJobObject(static_cast<HANDLE>(m_job), hProc);
    CloseHandle(hProc);
    return ok != FALSE;
}

bool JobController::isAlive() const
{
    if (!m_job)
        return false;

    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION acc{};
    if (!QueryInformationJobObject(static_cast<HANDLE>(m_job),
                                   JobObjectBasicAccountingInformation,
                                   &acc, sizeof(acc), nullptr)) {
        return false;
    }
    return acc.ActiveProcesses > 0;
}

void JobController::terminate()
{
    if (!m_job)
        return;

    TerminateJobObject(static_cast<HANDLE>(m_job), 1);
    closeHandle();
}

void JobController::closeHandle()
{
    if (m_job) {
        CloseHandle(static_cast<HANDLE>(m_job));
        m_job = nullptr;
    }
}

#else

#include <csignal>
#include <cerrno>
#include <sys/types.h>
#include <unistd.h>

JobController::JobController() = default;

JobController::~JobController()
{
    closeHandle();
}

bool JobController::assign(qint64 pid)
{
    if (pid <= 0)
        return false;
    m_pid = pid;
    return true;
}

bool JobController::isAlive() const
{
    if (m_pid <= 0)
        return false;

    const pid_t pgid = static_cast<pid_t>(m_pid);
    if (killpg(pgid, 0) == 0)
        return true;
    if (errno == EPERM)
        return true;
    if (kill(static_cast<pid_t>(m_pid), 0) == 0)
        return true;
    return false;
}

void JobController::terminate()
{
    if (m_pid <= 0)
        return;

    const pid_t pgid = static_cast<pid_t>(m_pid);
    if (killpg(pgid, SIGTERM) != 0)
        kill(static_cast<pid_t>(m_pid), SIGTERM);

    closeHandle();
}

void JobController::closeHandle()
{
    m_pid = 0;
}

#endif
