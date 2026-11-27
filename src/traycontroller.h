#ifndef TRAYCONTROLLER_H
#define TRAYCONTROLLER_H

#include <QObject>
#include <QHash>
#include <QIcon>
#include <QList>

QT_BEGIN_NAMESPACE
class QWindow;
class QSystemTrayIcon;
class QMenu;
QT_END_NAMESPACE

class ProcessManager;
struct ProcessEntry;

class TrayController : public QObject
{
    Q_OBJECT

public:

    explicit TrayController(QWindow* window, QObject* parent = nullptr);
    ~TrayController() override;

    Q_INVOKABLE bool isAvailable() const;
    // Показать окно поверх остальных и отдать ему фокус. Windows не даёт
    // фоновому процессу просто так забрать фокус — тут это обходится.
    Q_INVOKABLE void bringToFront(QWindow* window);
    void setWindow(QWindow* window);
    void setProcessManager(ProcessManager* manager);

public slots:
    void showWindow();
    void hideWindow();
    void toggleWindow();
    void notify(const QString& title, const QString& message);

private slots:
    void onActivated(int reason);

private:
    void rebuildMenu();
    void addProgramActions(QMenu* menu, const QList<ProcessEntry>& entries);
    QIcon programIcon(const QString& path, bool running);

    QWindow*          m_window = nullptr;
    QSystemTrayIcon*  m_tray = nullptr;
    QMenu*            m_menu = nullptr;
    QList<QMenu*>     m_subMenus;
    ProcessManager*   m_manager = nullptr;
    QHash<QString, QIcon> m_iconCache;
    QHash<QString, QIcon> m_runningIconCache;
};

#endif
