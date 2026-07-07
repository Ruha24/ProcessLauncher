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

JobController::JobController() = default;
JobController::~JobController() = default;

bool JobController::assign(qint64) { return false; }
bool JobController::isAlive() const { return false; }
void JobController::terminate() {}
void JobController::closeHandle() {}

#endif
