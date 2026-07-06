#ifndef TRAYCONTROLLER_H
#define TRAYCONTROLLER_H

#include <QObject>

QT_BEGIN_NAMESPACE
class QWindow;
class QSystemTrayIcon;
class QMenu;
QT_END_NAMESPACE

// Системный трей: сворачивание окна в трей, клик по иконке показывает/прячет
// окно, контекстное меню с «Показать» и «Выход». Управляет окном приложения
// (root QML ApplicationWindow, который является QWindow).
class TrayController : public QObject
{
    Q_OBJECT

public:
    // window — root-объект QML (ApplicationWindow). Может быть nullptr, тогда
    // трей просто не будет управлять окном (безопасная деградация).
    explicit TrayController(QWindow* window, QObject* parent = nullptr);
    ~TrayController() override;

    Q_INVOKABLE bool isAvailable() const;
    void setWindow(QWindow* window);

public slots:
    void showWindow();
    void hideWindow();
    void toggleWindow();

private slots:
    void onActivated(int reason);   // QSystemTrayIcon::ActivationReason as int

private:
    QWindow*          m_window = nullptr;
    QSystemTrayIcon*  m_tray = nullptr;
    QMenu*            m_menu = nullptr;
};

#endif // TRAYCONTROLLER_H
