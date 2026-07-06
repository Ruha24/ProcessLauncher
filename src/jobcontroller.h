#ifndef JOBCONTROLLER_H
#define JOBCONTROLLER_H

#include <QtGlobal>

// Кроссплатформенная обёртка над «группой процессов».
//
// Windows: job object с флагом KILL_ON_JOB_CLOSE. Процесс и ВСЕ его потомки
//   (например TLauncher -> java) попадают в один job. terminate() убивает всё
//   дерево, isAlive() смотрит число активных процессов в job'е — поэтому
//   состояние «запущен» корректно, даже если исходный процесс (лаунчер) уже
//   завершился, а его ребёнок (java) ещё работает.
//
// Другие ОС: сейчас заглушка (assign/terminate/isAlive безопасно no-op/false).
//   Логика запуска в ProcessManager от этого не ломается — просто дерево не
//   отслеживается, как раньше. Реализацию под Linux/macOS (process groups,
//   setsid + killpg) можно добавить здесь позже, не трогая остальной код.
class JobController
{
public:
    JobController();
    ~JobController();

    JobController(const JobController&) = delete;
    JobController& operator=(const JobController&) = delete;

    // Создать job (если ещё не создан) и поместить в него процесс по PID.
    // Возвращает true при успехе. На не-Windows возвращает false (no-op).
    bool assign(qint64 pid);

    // Есть ли в группе хотя бы один живой процесс.
    bool isAlive() const;

    // Убить всю группу (дерево процессов). Идемпотентно.
    void terminate();

private:
    void closeHandle();

    void* m_job = nullptr;  // HANDLE на Windows; nullptr на других ОС
};

#endif // JOBCONTROLLER_H
