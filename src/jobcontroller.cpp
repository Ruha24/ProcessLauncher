#include "jobcontroller.h"

#ifdef Q_OS_WIN
#include <windows.h>

JobController::JobController() = default;

JobController::~JobController()
{
    closeHandle();
}

namespace {
bool applyKillOnClose(HANDLE job, bool on)
{
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
    info.BasicLimitInformation.LimitFlags =
        on ? JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE : 0;
    return SetInformationJobObject(job, JobObjectExtendedLimitInformation,
                                   &info, sizeof(info)) != FALSE;
}
}

void JobController::setKillOnClose(bool on)
{
    m_killOnClose = on;
    if (m_job)
        applyKillOnClose(static_cast<HANDLE>(m_job), on);
}

bool JobController::assign(qint64 pid)
{
    if (!m_job) {
        HANDLE job = CreateJobObject(nullptr, nullptr);
        if (!job)
            return false;

        if (!applyKillOnClose(job, m_killOnClose)) {
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

void JobController::adoptProcessHandle(void* process)
{
    if (!process)
        return;

    if (!m_job) {
        if (HANDLE job = CreateJobObject(nullptr, nullptr)) {
            if (applyKillOnClose(job, m_killOnClose))
                m_job = job;
            else
                CloseHandle(job);
        }
    }

    if (m_job && AssignProcessToJobObject(static_cast<HANDLE>(m_job),
                                          static_cast<HANDLE>(process))) {
        CloseHandle(static_cast<HANDLE>(process));
        return;
    }

    // В job не вышло: держим дескриптор процесса сами.
    if (m_job) {
        CloseHandle(static_cast<HANDLE>(m_job));
        m_job = nullptr;
    }
    m_process = process;
}

bool JobController::isAlive() const
{
    if (m_process)
        return WaitForSingleObject(static_cast<HANDLE>(m_process), 0) == WAIT_TIMEOUT;
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
    if (m_process)
        TerminateProcess(static_cast<HANDLE>(m_process), 1);
    else if (m_job)
        TerminateJobObject(static_cast<HANDLE>(m_job), 1);
    closeHandle();
}

void JobController::closeHandle()
{
    if (m_job) {
        CloseHandle(static_cast<HANDLE>(m_job));
        m_job = nullptr;
    }
    if (m_process) {
        CloseHandle(static_cast<HANDLE>(m_process));
        m_process = nullptr;
    }
}

bool JobController::isPidAlive(qint64 pid)
{
    if (pid <= 0)
        return false;

    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                               static_cast<DWORD>(pid));
    if (!hProc) {
        // Нет доступа — процесс есть, просто он нам не по правам.
        return GetLastError() == ERROR_ACCESS_DENIED;
    }

    DWORD code = 0;
    const BOOL ok = GetExitCodeProcess(hProc, &code);
    CloseHandle(hProc);
    return ok && code == STILL_ACTIVE;
}

bool JobController::terminatePid(qint64 pid)
{
    if (pid <= 0)
        return false;

    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
    if (!hProc)
        return false;
    const BOOL ok = TerminateProcess(hProc, 1);
    CloseHandle(hProc);
    return ok != FALSE;
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

void JobController::setKillOnClose(bool on)
{
    // Здесь процессы и так переживают лаунчер; при выходе их останавливает
    // ProcessManager явным terminate().
    m_killOnClose = on;
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

bool JobController::isPidAlive(qint64 pid)
{
    if (pid <= 0)
        return false;
    return kill(static_cast<pid_t>(pid), 0) == 0 || errno == EPERM;
}

bool JobController::terminatePid(qint64 pid)
{
    if (pid <= 0)
        return false;
    return kill(static_cast<pid_t>(pid), SIGTERM) == 0;
}

#endif
