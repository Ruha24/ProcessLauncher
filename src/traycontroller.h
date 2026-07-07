#ifndef TRAYCONTROLLER_H
#define TRAYCONTROLLER_H

#include <QObject>

QT_BEGIN_NAMESPACE
class QWindow;
class QSystemTrayIcon;
class QMenu;
QT_END_NAMESPACE

class TrayController : public QObject
{
    Q_OBJECT

public:

    explicit TrayController(QWindow* window, QObject* parent = nullptr);
    ~TrayController() override;

    Q_INVOKABLE bool isAvailable() const;
    void setWindow(QWindow* window);

public slots:
    void showWindow();
    void hideWindow();
    void toggleWindow();

private slots:
    void onActivated(int reason);

private:
    QWindow*          m_window = nullptr;
    QSystemTrayIcon*  m_tray = nullptr;
    QMenu*            m_menu = nullptr;
};

#endif
