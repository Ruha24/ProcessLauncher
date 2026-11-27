#include "jobcontroller.h"

#include <QByteArray>
#include <QList>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

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

namespace {
qint64 fileTime(const FILETIME& ft)
{
    return (static_cast<qint64>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}

// PROCESS_MEMORY_COUNTERS_EX2 (Windows 10 21H2+): в MinGW её нет.
struct MemCountersEx2 {
    PROCESS_MEMORY_COUNTERS_EX ex;
    SIZE_T  privateWorkingSetSize;
    ULONG64 sharedCommitUsage;
};

// Прибавляет к usage время и память процесса. Память — «частный рабочий
// набор», как в столбце «Память» Диспетчера задач; на старых Windows —
// выделенная частная память.
bool addHandleUsage(HANDLE h, JobController::Usage& usage)
{
    FILETIME created, exited, kernel, user;
    if (!GetProcessTimes(h, &created, &exited, &kernel, &user))
        return false;
    usage.cpuNs = qMax<qint64>(usage.cpuNs, 0) + (fileTime(kernel) + fileTime(user)) * 100;

    qint64 mem = -1;
    MemCountersEx2 c2{};
    c2.ex.cb = sizeof(c2);
    if (GetProcessMemoryInfo(h, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&c2), sizeof(c2))) {
        mem = static_cast<qint64>(c2.privateWorkingSetSize);
    } else {
        PROCESS_MEMORY_COUNTERS_EX c{};
        c.cb = sizeof(c);
        if (GetProcessMemoryInfo(h, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&c), sizeof(c)))
            mem = static_cast<qint64>(c.PrivateUsage);
    }
    if (mem >= 0)
        usage.memBytes = qMax<qint64>(usage.memBytes, 0) + mem;
    return true;
}
}

JobController::Usage JobController::usage() const
{
    Usage result;
    if (!m_job) {
        if (m_process)
            addHandleUsage(static_cast<HANDLE>(m_process), result);
        return result;
    }

    // Время — по всему job, включая уже завершившиеся процессы: так оно не
    // скачет вниз, когда дочерний процесс выходит.
    qint64 jobCpu = -1;
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION acc{};
    if (QueryInformationJobObject(static_cast<HANDLE>(m_job),
                                  JobObjectBasicAccountingInformation,
                                  &acc, sizeof(acc), nullptr)) {
        jobCpu = (acc.TotalUserTime.QuadPart + acc.TotalKernelTime.QuadPart) * 100;
    }

    // Память — сумма по живым процессам job.
    constexpr DWORD kMaxPids = 512;
    QByteArray buf(int(sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST)
                       + kMaxPids * sizeof(ULONG_PTR)), '\0');
    auto* list = reinterpret_cast<JOBOBJECT_BASIC_PROCESS_ID_LIST*>(buf.data());
    if (QueryInformationJobObject(static_cast<HANDLE>(m_job), JobObjectBasicProcessIdList,
                                  list, static_cast<DWORD>(buf.size()), nullptr)
        || GetLastError() == ERROR_MORE_DATA) {
        for (DWORD i = 0; i < list->NumberOfProcessIdsInList; ++i) {
            HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                                   static_cast<DWORD>(list->ProcessIdList[i]));
            if (!h)
                continue;
            addHandleUsage(h, result);
            CloseHandle(h);
        }
    }

    if (jobCpu >= 0)
        result.cpuNs = jobCpu;
    return result;
}

JobController::Usage JobController::pidUsage(qint64 pid)
{
    Usage result;
    if (pid <= 0)
        return result;
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
    if (!h)
        return result;
    addHandleUsage(h, result);
    CloseHandle(h);
    return result;
}

QList<qint64> JobController::pids() const
{
    QList<qint64> result;
    if (!m_job) {
        if (m_process)
            result.append(static_cast<qint64>(GetProcessId(static_cast<HANDLE>(m_process))));
        return result;
    }

    constexpr DWORD kMaxPids = 512;
    QByteArray buf(int(sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST)
                       + kMaxPids * sizeof(ULONG_PTR)), '\0');
    auto* list = reinterpret_cast<JOBOBJECT_BASIC_PROCESS_ID_LIST*>(buf.data());
    if (QueryInformationJobObject(static_cast<HANDLE>(m_job), JobObjectBasicProcessIdList,
                                  list, static_cast<DWORD>(buf.size()), nullptr)
        || GetLastError() == ERROR_MORE_DATA) {
        for (DWORD i = 0; i < list->NumberOfProcessIdsInList; ++i)
            result.append(static_cast<qint64>(list->ProcessIdList[i]));
    }
    return result;
}

QList<JobController::ProcessInfo> JobController::runningProcesses(const QSet<QString>& fileNames)
{
    QList<ProcessInfo> result;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return result;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) {
        if (pe.th32ProcessID <= 4)   // Idle и System
            continue;
        if (!fileNames.isEmpty()
            && !fileNames.contains(QString::fromWCharArray(pe.szExeFile).toLower()))
            continue;

        HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
        if (!h)
            continue;
        wchar_t path[4096];
        DWORD size = DWORD(sizeof(path) / sizeof(path[0]));
        if (QueryFullProcessImageNameW(h, 0, path, &size)) {
            result.append({static_cast<qint64>(pe.th32ProcessID),
                           QString::fromWCharArray(path, int(size))
                               .replace(QLatin1Char('\\'), QLatin1Char('/'))});
        }
        CloseHandle(h);
    }
    CloseHandle(snap);
    return result;
}

QDateTime JobController::startTime(qint64 pid)
{
    if (pid <= 0)
        return QDateTime();
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
    if (!h)
        return QDateTime();
    FILETIME created, exited, kernel, user;
    QDateTime result;
    if (GetProcessTimes(h, &created, &exited, &kernel, &user)) {
        // FILETIME — сотни наносекунд с 1601 года.
        const qint64 unix100ns = fileTime(created) - 116444736000000000LL;
        if (unix100ns > 0)
            result = QDateTime::fromMSecsSinceEpoch(unix100ns / 10000);
    }
    CloseHandle(h);
    return result;
}

#else

#include <csignal>
#include <cerrno>
#include <sys/types.h>
#include <unistd.h>
#ifdef Q_OS_MACOS
#include <libproc.h>
#include <sys/proc_info.h>
#include <mach/mach_time.h>
#endif

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

JobController::Usage JobController::pidUsage(qint64 pid)
{
    Usage result;
#ifdef Q_OS_MACOS
    proc_taskinfo info{};
    if (pid > 0
        && proc_pidinfo(static_cast<pid_t>(pid), PROC_PIDTASKINFO, 0,
                        &info, sizeof(info)) == static_cast<int>(sizeof(info))) {
        // Время задачи — в единицах mach_absolute_time (на Apple Silicon это
        // не наносекунды).
        static const mach_timebase_info_data_t tb = [] {
            mach_timebase_info_data_t t{};
            mach_timebase_info(&t);
            return t;
        }();
        const double scale = tb.denom ? double(tb.numer) / tb.denom : 1.0;
        result.cpuNs = static_cast<qint64>(
            double(info.pti_total_user + info.pti_total_system) * scale);
        result.memBytes = static_cast<qint64>(info.pti_resident_size);
    }
#else
    Q_UNUSED(pid);
#endif
    return result;
}

JobController::Usage JobController::usage() const
{
    if (m_pid <= 0)
        return Usage();
#ifdef Q_OS_MACOS
    // Все процессы группы — лаунчер запускает программу в своей группе.
    QList<pid_t> pids(512);
    const int bytes = proc_listpids(PROC_PGRP_ONLY, static_cast<uint32_t>(m_pid),
                                    pids.data(), int(pids.size() * sizeof(pid_t)));
    Usage result;
    const int n = bytes > 0 ? qMin(int(bytes / int(sizeof(pid_t))), int(pids.size())) : 0;
    for (int i = 0; i < n; ++i) {
        if (pids.at(i) <= 0)
            continue;
        const Usage u = pidUsage(pids.at(i));
        if (u.cpuNs >= 0)
            result.cpuNs = qMax<qint64>(result.cpuNs, 0) + u.cpuNs;
        if (u.memBytes >= 0)
            result.memBytes = qMax<qint64>(result.memBytes, 0) + u.memBytes;
    }
    if (result.cpuNs < 0)
        return pidUsage(m_pid);
    return result;
#else
    return pidUsage(m_pid);
#endif
}

QList<qint64> JobController::pids() const
{
    QList<qint64> result;
    if (m_pid <= 0)
        return result;
#ifdef Q_OS_MACOS
    QList<pid_t> buf(512);
    const int bytes = proc_listpids(PROC_PGRP_ONLY, static_cast<uint32_t>(m_pid),
                                    buf.data(), int(buf.size() * sizeof(pid_t)));
    const int n = bytes > 0 ? qMin(int(bytes / int(sizeof(pid_t))), int(buf.size())) : 0;
    for (int i = 0; i < n; ++i)
        if (buf.at(i) > 0)
            result.append(buf.at(i));
#endif
    if (!result.contains(m_pid))
        result.append(m_pid);
    return result;
}

QList<JobController::ProcessInfo> JobController::runningProcesses(const QSet<QString>& fileNames)
{
    Q_UNUSED(fileNames);
    QList<ProcessInfo> result;
#ifdef Q_OS_MACOS
    const int count = proc_listallpids(nullptr, 0);
    if (count <= 0)
        return result;
    QList<pid_t> buf(count + 64);
    const int n = qMin(proc_listallpids(buf.data(), int(buf.size() * sizeof(pid_t))),
                       int(buf.size()));
    char path[PROC_PIDPATHINFO_MAXSIZE];
    for (int i = 0; i < n; ++i) {
        if (buf.at(i) <= 0)
            continue;
        if (proc_pidpath(buf.at(i), path, sizeof(path)) > 0)
            result.append({static_cast<qint64>(buf.at(i)), QString::fromUtf8(path)});
    }
#endif
    return result;
}

QDateTime JobController::startTime(qint64 pid)
{
#ifdef Q_OS_MACOS
    proc_bsdinfo info{};
    if (pid > 0
        && proc_pidinfo(static_cast<pid_t>(pid), PROC_PIDTBSDINFO, 0,
                        &info, sizeof(info)) == static_cast<int>(sizeof(info)))
        return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(info.pbi_start_tvsec));
#else
    Q_UNUSED(pid);
#endif
    return QDateTime();
}

#endif
