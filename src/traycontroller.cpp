#include "traycontroller.h"

#include <QWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QPainter>
#include <QPixmap>

#include "processmanager.h"

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
    // Меню собирается заново при каждом открытии — так статусы программ
    // и список профилей всегда актуальны без отдельной синхронизации.
    connect(m_menu, &QMenu::aboutToShow, this, &TrayController::rebuildMenu);
    rebuildMenu();

    m_tray->setContextMenu(m_menu);

    connect(m_tray, &QSystemTrayIcon::activated,
            this, [this](QSystemTrayIcon::ActivationReason reason) {
                onActivated(static_cast<int>(reason));
            });

    m_tray->show();
}

TrayController::~TrayController()
{
    qDeleteAll(m_subMenus);
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

void TrayController::setProcessManager(ProcessManager* manager)
{
    m_manager = manager;
}

void TrayController::rebuildMenu()
{
    if (!m_menu)
        return;

    m_menu->clear();
    qDeleteAll(m_subMenus);
    m_subMenus.clear();

    QAction* showAction = m_menu->addAction(tr("Show"));
    connect(showAction, &QAction::triggered, this, &TrayController::showWindow);

    if (m_manager) {
        const QList<ProcessEntry> all = m_manager->entries();
        const QStringList profiles = m_manager->profiles();

        if (!all.isEmpty()) {
            m_menu->addSeparator();

            if (profiles.size() <= 1) {
                // Один профиль — подменю было бы лишним шагом.
                addProgramActions(m_menu, all);
            } else {
                for (const QString& profile : profiles) {
                    QList<ProcessEntry> entries;
                    for (const ProcessEntry& e : all)
                        if (e.profile == profile)
                            entries.append(e);

                    const int running = m_manager->profileRunningCount(profile);
                    const QString title = entries.isEmpty()
                        ? profile
                        : QStringLiteral("%1  (%2/%3)").arg(profile).arg(running).arg(entries.size());

                    auto* sub = new QMenu(title);
                    m_subMenus.append(sub);
                    m_menu->addMenu(sub);

                    if (entries.isEmpty()) {
                        sub->addAction(tr("No programs"))->setEnabled(false);
                        continue;
                    }

                    QAction* startAll = sub->addAction(tr("Start all"));
                    startAll->setEnabled(running < entries.size());
                    connect(startAll, &QAction::triggered, this, [this, profile]() {
                        if (m_manager) m_manager->startProfile(profile);
                    });

                    QAction* stopAll = sub->addAction(tr("Stop all"));
                    stopAll->setEnabled(running > 0);
                    connect(stopAll, &QAction::triggered, this, [this, profile]() {
                        if (m_manager) m_manager->stopProfile(profile);
                    });

                    sub->addSeparator();
                    addProgramActions(sub, entries);
                }
            }

            bool anyRunning = false;
            for (const ProcessEntry& e : all)
                if (m_manager->isRunning(e.id)) { anyRunning = true; break; }

            if (anyRunning) {
                m_menu->addSeparator();
                QAction* stopEverything = m_menu->addAction(tr("Stop all programs"));
                connect(stopEverything, &QAction::triggered, this, [this]() {
                    if (m_manager) m_manager->stopAll();
                });
            }
        }
    }

    m_menu->addSeparator();
    QAction* quitAction = m_menu->addAction(tr("Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
}

void TrayController::addProgramActions(QMenu* menu, const QList<ProcessEntry>& entries)
{
    menu->setToolTipsVisible(true);
    for (const ProcessEntry& e : entries) {
        const bool running = m_manager->isRunning(e.id);
        const bool openOnly = ProcessManager::isOpenOnly(e);
        // В Windows у пункта с иконкой галочка не рисуется, поэтому
        // запущенную программу отмечает зелёная точка на иконке.
        QAction* a = menu->addAction(programIcon(e.path, running), e.name);
        a->setToolTip(openOnly ? tr("Click to open")
                      : running ? tr("Running — click to stop") : tr("Click to start"));
        const QString id = e.id;
        connect(a, &QAction::triggered, this, [this, id]() {
            if (!m_manager)
                return;
            if (m_manager->isRunning(id))
                m_manager->stop(id);
            else
                m_manager->start(id);
        });
    }
}

QIcon TrayController::programIcon(const QString& path, bool running)
{
    auto& cache = running ? m_runningIconCache : m_iconCache;
    auto it = cache.constFind(path);
    if (it != cache.constEnd())
        return *it;

    static QFileIconProvider provider;
    QIcon base = QFileInfo::exists(path) ? provider.icon(QFileInfo(path))
               : ProcessManager::isLink(path) ? provider.icon(QFileIconProvider::Network)
               : QIcon();

    if (!running) {
        cache.insert(path, base);
        return base;
    }

    // Рисуем в 32px — на HiDPI-экранах меню берёт крупный размер.
    const int size = 32;
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    if (!base.isNull())
        base.paint(&p, 0, 0, size, size);
    const qreal d = size * 0.5;
    const QRectF dot(size - d, size - d, d, d);
    p.setPen(QPen(Qt::white, size * 0.08));
    p.setBrush(QColor(0x22, 0xC5, 0x5E));
    p.drawEllipse(dot.adjusted(1, 1, -1, -1));
    p.end();

    const QIcon icon(pm);
    cache.insert(path, icon);
    return icon;
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

void TrayController::notify(const QString& title, const QString& message)
{
    if (m_tray)
        m_tray->showMessage(title, message, QSystemTrayIcon::Information, 5000);
}

void TrayController::onActivated(int reason)
{

    if (reason == QSystemTrayIcon::Trigger)
        toggleWindow();
}
