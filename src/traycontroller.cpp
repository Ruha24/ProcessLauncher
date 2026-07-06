#include "traycontroller.h"

#include <QWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QIcon>

TrayController::TrayController(QWindow* window, QObject* parent)
    : QObject(parent)
    , m_window(window)
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;

    m_tray = new QSystemTrayIcon(this);
    m_tray->setIcon(QIcon(QStringLiteral(":/icons/app.png")));
    m_tray->setToolTip(QStringLiteral("Process Launcher"));

    m_menu = new QMenu();
    QAction* showAction = m_menu->addAction(tr("Show"));
    connect(showAction, &QAction::triggered, this, &TrayController::showWindow);

    m_menu->addSeparator();

    QAction* quitAction = m_menu->addAction(tr("Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_tray->setContextMenu(m_menu);

    connect(m_tray, &QSystemTrayIcon::activated,
            this, [this](QSystemTrayIcon::ActivationReason reason) {
                onActivated(static_cast<int>(reason));
            });

    m_tray->show();
}

TrayController::~TrayController()
{
    delete m_menu;
}

bool TrayController::isAvailable() const
{
    return m_tray != nullptr;
}

void TrayController::setWindow(QWindow* window)
{
    m_window = window;
}

void TrayController::showWindow()
{
    if (!m_window)
        return;
    m_window->show();
    m_window->raise();
    m_window->requestActivate();
}

void TrayController::hideWindow()
{
    if (m_window)
        m_window->hide();
}

void TrayController::toggleWindow()
{
    if (!m_window)
        return;
    if (m_window->isVisible())
        hideWindow();
    else
        showWindow();
}

void TrayController::onActivated(int reason)
{
    if (reason == QSystemTrayIcon::Trigger)
        toggleWindow();
}
