#include "ckbsettings.h"
#include "kbmanager.h"
#include "kbfirmware.h"
#include "keyaction.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <cstdlib>
#include <QShortcut>
#include <QShowEvent>
#include <QMessageBox>
#include <QMenuBar>
#include <unistd.h>
#include <ckbnextconfig.h>
#include <QProcess>
#include "appcontroller.h"
#include "monotonicclock.h"

#ifndef DISABLE_UPDATER
#include "ckbupdater.h"
#include "ckbupdaterwidget.h"
#endif

#include "ckbsystemtrayicon.h"
#include "daemonwarndialog.h"



MainWindow* MainWindow::mainWindow = nullptr;

#if defined(Q_OS_MACOS) && !defined(OS_MAC_LEGACY)
bool is_catalina_or_higher(){
    // Get macOS version. If Catalina or higher, start the daemon agent as the current user to request for HID permission.
    QString macOSver = QSysInfo::productVersion();
    // Split major/minor
    QVector<QStringRef> verVector = macOSver.splitRef('.');
    // Check if Catalina or greater
    return (verVector.count() == 2 && verVector.at(0) == QString("10") && verVector.at(1).toInt() >= 15);
}

void MainWindow::appleRequestHidTimer(){
    // Destroy the timer immediately if we have devices connected
    if(KbManager::devices().count()){
        catalinaTimer->stop();
        catalinaTimer->deleteLater();
        catalinaTimer = nullptr;
        return;
    }

    QProcess launchctl;
    launchctl.setProgram("launchctl");
    // Start the agent only if the state on the previous run wasn't "running" or pre exec
    if(!catalinaAgentStarted){
        // Start the service
        launchctl.setArguments(QStringList() << "start" << "org.ckb-next.daemon");
        launchctl.start();
        launchctl.waitForFinished();
        qDebug() << "Launchctl start returned" << launchctl.exitCode();
        catalinaAgentStarted = true;
    }

    // Get EUID
    uid_t euid = geteuid();

    // This will most likely block the UI thread, but hopefully it won't be too bad
    QThread::msleep(1500);

    // Call launchctl
    launchctl.setArguments(QStringList() << "print" << QString("gui/%1/org.ckb-next.daemon").arg(euid));
    launchctl.start();
    launchctl.waitForFinished();
    if(launchctl.exitCode() == 0){
        QString str(launchctl.readAllStandardOutput());

        int statestart = str.indexOf("state = ") + 8;
        int statelen = str.indexOf('\n', statestart) - statestart;
        if(statelen > 20)
            return;

        // Extract the state from the output
        QStringRef agentState(&str, statestart, statelen);
        if(agentState == QString("running") || agentState == QString("spawned (pre-exec)")){
            catalinaAgentStarted = true;
            return;
        }
        catalinaAgentStarted = false;

        // Extract the daemon's return code
        int statusstart = str.lastIndexOf("last exit code = ") + 17;
        int statuslen = str.indexOf('\n', statusstart) - statusstart;
        if(statuslen < 3 && statuslen > 0){
            hid_req_ret status = (hid_req_ret)QStringRef(&str, statusstart, statuslen).toInt();
            // We do not need to do anything if the request succeeds, other than wait for the loop to run again
            if(status == REQUEST_ALREADY_ALLOWED){
                qDebug() << "We have HID access!";
                catalinaTimer->stop();
                catalinaTimer->deleteLater();
                catalinaTimer = nullptr;
                // Ask user to restart daemon only if we first had to request permission
                if(prevHidRet == REQUEST_SUCCEEDED)
                    QProcess::execute("open", QStringList() << "-a" << "Terminal" << "/Applications/ckb-next.app/Contents/Resources/daemon-restart.sh");
            }
            else
                qDebug() << "HID agent encountered an unknown error";
            prevHidRet = status;
        } else if (statuslen == 14) {
            qDebug() << "Agent was never started. Something went wrong.";
        }
    }
}
#endif

MainWindow::MainWindow(const bool silent, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mainWindow = this;

    app = new AppController(this);
    app->setWindowVisibleCheck([this](){ return isVisible(); });
    app->init();
    connect(KbManager::kbManager(), SIGNAL(kbConnected(Kb*)), this, SLOT(addDevice(Kb*)));
    connect(KbManager::kbManager(), SIGNAL(kbDisconnected(Kb*)), this, SLOT(removeDevice(Kb*)));
    connect(KbManager::scanTimer(), SIGNAL(timeout()), this, SLOT(timerTick()));
    connect(app, &AppController::showWindowRequested, this, &MainWindow::showWindow);
    connect(app, &AppController::firmwareUpdateAvailable, this, &MainWindow::firmwareUpdateAvailable);

    // Set up tray icon
    restoreAction = new QAction(tr("Restore"), this);
    closeAction = new QAction(tr("Quit"), this);
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addAction(closeAction);
    trayIcon = new CkbSystemTrayIcon(getIcon(), getIconName(), this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->show();
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(iconClicked(QSystemTrayIcon::ActivationReason)));
    connect(trayIcon, &CkbSystemTrayIcon::scrollRequested, KbManager::kbManager(), &KbManager::brightnessScroll);
    toggleTrayIcon(!CkbSettings::get("Program/SuppressTrayIcon").toBool());

#ifdef Q_OS_MACOS
    // Make a custom "Close" menu action for OSX, as the default one brings up the "still running" popup unnecessarily
    QMenuBar* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);
    this->menuBar()->addMenu("ckb-next")->addAction(closeAction);
#else
    // On linux, add a handler for Ctrl+Q
    new QShortcut(QKeySequence("Ctrl+Q"), this, SLOT(quitApp()));
#endif

    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(quitApp()));
    connect(closeAction, SIGNAL(triggered()), this, SLOT(quitApp()));
    connect(restoreAction, SIGNAL(triggered()), this, SLOT(showWindow()));
    connect(qApp, SIGNAL(applicationStateChanged(Qt::ApplicationState)), this, SLOT(stateChange(Qt::ApplicationState)));

    ui->tabWidget->addTab(settingsWidget = new SettingsWidget(this), QString(tr("Settings")));
    settingsWidget->setVersion(KbManager::ckbGuiVersion());
    connect(app, &AppController::statusTextChanged, settingsWidget, &SettingsWidget::setStatus);

    DaemonWarnDialog *warnDialog = new DaemonWarnDialog(mainWindow);

    if(!app->checkDaemon(silent)){
        // show the main window (otherwise only the dialog will be visible)
        // finally show the dialog
        showWindow();
        warnDialog->open();
    }
#ifndef DISABLE_UPDATER
    if(!CkbSettings::get("Program/DisableAutoUpdCheck", false).toBool())
        checkForCkbUpdates();

    connect(settingsWidget, &SettingsWidget::checkForUpdates, this, &MainWindow::checkForCkbUpdates);
#endif

#if defined(Q_OS_MACOS) && !defined(OS_MAC_LEGACY)
    if(is_catalina_or_higher()){
        // Load the agent as it'll be unloaded on first installation
        {
            QProcess launchctl;
            launchctl.setProgram("launchctl");
            launchctl.setArguments(QStringList() << "load" << "/Library/LaunchAgents/org.ckb-next.daemon_agent.plist");
            launchctl.start();
            launchctl.waitForFinished();
            qDebug() << "Launchctl load returned" << launchctl.exitCode();
        }

        // Start it
        catalinaTimer = new QTimer(this);
        connect(catalinaTimer, &QTimer::timeout, this, &MainWindow::appleRequestHidTimer);
        catalinaTimer->setInterval(11000);
        catalinaTimer->start();
        QTimer::singleShot(2000, this, &MainWindow::appleRequestHidTimer);
    }
#endif

    app->startScan();
}

void MainWindow::checkForCkbUpdates(){
#ifndef DISABLE_UPDATER
    updater = new CkbUpdater(this);
    connect(updater, &CkbUpdater::checkedForNewVer, this, &MainWindow::checkedForNewVer);
    updater->checkForNewVersion();
#endif
}

void MainWindow::toggleTrayIcon(bool visible){
    trayIcon->setVisible(visible);
}

void MainWindow::addDevice(Kb* device){
    // Connected already?
    foreach(KbWidget* w, kbWidgets){
        if(w->device == device)
            return;
    }
    // Add the keyboard
    KbWidget* widget = new KbWidget(this, device, app->windowDetector());
    kbWidgets.append(widget);
    // Add to tabber; switch to this device if the user is on the settings screen
    int count = ui->tabWidget->count();
    ui->tabWidget->insertTab(count - 1, widget, widget->name());
    if(ui->tabWidget->currentIndex() == count)
        ui->tabWidget->setCurrentIndex(count - 1);
}

void MainWindow::removeDevice(Kb* device){
    foreach(KbWidget* w, kbWidgets){
        // Remove this device from the UI
        if(w->device == device){
            int i = kbWidgets.indexOf(w);
            ui->tabWidget->removeTab(i);
            kbWidgets.removeAt(i);
            w->deleteLater();
        }
    }
}

void MainWindow::firmwareUpdateAvailable(Kb* device, CkbVersionNumber version){
    foreach(KbWidget* w, kbWidgets){
        if(w->device != device)
            continue;
        w->updateFwButton();
        // Don't run this method here because it will lock up the timer and prevent devices from working properly
        // Use a queued invocation instead
        metaObject()->invokeMethod(this, "showFwUpdateNotification", Qt::QueuedConnection, Q_ARG(QWidget*, w), Q_ARG(CkbVersionNumber, version));
        return;
    }
}

void MainWindow::showFwUpdateNotification(QWidget* widget, CkbVersionNumber version){
    static bool isShowing = false;
    if(isShowing)
        return;
    isShowing = true;
    showWindow();
    KbWidget* w = (KbWidget*)widget;
    // Ask for update
    if(QMessageBox::information(this, "Firmware update", tr("A new firmware is available for your %1 (v%2)\nWould you like to install it now?").arg(w->device->usbModel, version.toString()), QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No), QMessageBox::Yes) == QMessageBox::Yes){
        // If accepted, switch to the firmware tab and bring up the update window
        w->showDeviceTab();
        ui->tabWidget->setCurrentIndex(kbWidgets.indexOf(w));
        w->showFwUpdate();
    }
    isShowing = false;
}

void MainWindow::showEvent(QShowEvent* event){
    KeyAction::setScreenCursorWindow(windowHandle());
    QMainWindow::showEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event){
    // If the window is hidden already or the event is non-spontaneous (can happen on OSX when using the Quit menu), accept it and close
    if(!event->spontaneous() || isHidden()){
        event->accept();
        return;
    }
    if(!CkbSettings::get("Popups/BGWarning").toBool()){
        QMessageBox::information(this, "ckb-next", tr("ckb-next will still run in the background.\nTo close it, choose Quit from the tray menu\nor click \"Quit\" on the Settings screen."));
        CkbSettings::set("Popups/BGWarning", true);
    }
    hide();
    event->ignore();
}

void MainWindow::timerTick(){
    // AppController handles commands and firmware checks on the same timer, before this
    // Poll for setting updates
    settingsWidget->pollUpdates();
}

void MainWindow::iconClicked(QSystemTrayIcon::ActivationReason reason){
    // On Linux, hide/show the app when the tray icon is clicked
    // On OSX this just shows the menu
#ifndef Q_OS_MACOS
    if(reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger){
        if(isVisible())
            hide();
        else
            showWindow();
    }
#endif
}

void MainWindow::showWindow(){
    showNormal();
    raise();
    activateWindow();
}

void MainWindow::stateChange(Qt::ApplicationState state){
    // On OSX it's possible for the app to be brought to the foreground without the window actually reappearing.
    // We want to make sure it's shown when this happens.
#ifdef Q_OS_MAC
    static qint64 lastStateChange = 0;
    qint64 now = MonotonicClock::msecs();
    if(state == Qt::ApplicationActive){
        // This happens once at startup so ignore it. Also don't allow it to be called more than once every 2s.
        if(lastStateChange != 0 && now >= lastStateChange + 2 * 1000)
            showWindow();
        lastStateChange = now;
    }
#endif
}

void MainWindow::quitApp(){
    app->quit();
}

QIcon MainWindow::getIcon() {
    // on initial launch (first time using ckb) this association checked
    // will not be present, so force it to true, as we do not want to break default behaviour
    if (CkbSettings::get("Program/RGBIcon", QVariant(true)).toBool())
        return QIcon(":/img/ckb-next.png");

    return QIcon(":/img/ckb-next-monochrome.png");
}

QString MainWindow::getIconName() {
    // Same as above but return QStrings
    if (CkbSettings::get("Program/RGBIcon", QVariant(true)).toBool())
        return "ckb-next";

    return "ckb-next-monochrome";
}

void MainWindow::setTabsEnabled(bool e){
    ui->tabWidget->tabBar()->setEnabled(e);
    QWidget* currentWidget = ui->tabWidget->currentWidget();
    if(!currentWidget){
        qDebug() << "currentWidget in setTabsEnabled is null";
        return;
    }
    KbWidget* kbw = dynamic_cast<KbWidget*>(currentWidget);
    // The cast can fail if the app is quitting
    if(!kbw)
        return;
    kbw->setTabBarEnabled(e);
}

MainWindow::~MainWindow(){
    foreach(KbWidget* w, kbWidgets)
        delete w;
    kbWidgets.clear();
    app->shutdown();
    delete ui;
}

void MainWindow::checkedForNewVer(const QString& ver, const QString& changelog){
#ifndef DISABLE_UPDATER
    if(!ver.isEmpty()) {
        settingsWidget->setUpdateButtonText(tr("Update to v") + ver);
        showWindow();
        CkbUpdaterDialog updDialog(ver, changelog, this);
        updDialog.exec();
    } else {
        settingsWidget->setUpdateButtonText(tr("Up to date"));
    }
    updater->deleteLater();
    settingsWidget->enableUpdateButton();
#endif
}

void MainWindow::syncTrayIcon(){
    trayIcon->setIcon(getIcon(), getIconName());
}
