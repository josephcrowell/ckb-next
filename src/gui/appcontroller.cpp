#include "appcontroller.h"
#include "ckbsettings.h"
#include "kb.h"
#include "kbfirmware.h"
#include "kbmanager.h"
#include "media.h"
#include "xcb/xwindowdetector.h"
#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <QSharedMemory>
#include <QSocketNotifier>
#include <QTimer>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

extern QSharedMemory appShare;
extern QString devpath;
#if defined(Q_OS_MACOS) && !defined(OS_MAC_LEGACY)
// From mainwindow.cpp
bool is_catalina_or_higher();
#endif

// The status strings below keep the MainWindow translation context they were written in

int AppController::signalHandlerFd[2] = {0, 0};
AppController* AppController::_instance = nullptr;

AppController::AppController(QObject* parent) : QObject(parent)
{
    _instance = this;
}

AppController::~AppController(){
    if(_instance == this)
        _instance = nullptr;
}

void AppController::init(){
    _firmware = new KbFirmware();

    // Start device manager
    KbManager::init(QCoreApplication::applicationVersion());
    connect(KbManager::kbManager(), &KbManager::kbConnected, this, &AppController::addDevice);
    connect(KbManager::kbManager(), &KbManager::kbDisconnected, this, &AppController::removeDevice);
    connect(KbManager::kbManager(), &KbManager::versionUpdated, this, &AppController::updateStatus);
    connect(KbManager::scanTimer(), &QTimer::timeout, this, &AppController::timerTick);

    // Set up signal handler
    socketpair(AF_UNIX, SOCK_STREAM, 0, signalHandlerFd);

    sigNotifier = new QSocketNotifier(signalHandlerFd[1], QSocketNotifier::Read, this);
    connect(sigNotifier, &QSocketNotifier::activated, this, &AppController::readSignal);

    signal(SIGINT, AppController::posixSignalHandler);
    signal(SIGTERM, AppController::posixSignalHandler);
    signal(SIGHUP, AppController::posixSignalHandler);

#ifdef USE_XCB_EWMH
    qRegisterMetaType<XWindowInfo>("XWindowInfo");
    _windowDetector = new XWindowDetector();
    _windowDetector->start();
#endif
}

void AppController::startScan(){
    KbManager::kbManager()->scanKeyboards();
}

void AppController::shutdown(){
    KbManager::stop();
    deinitAudioSubsystem();
    delete _firmware;
    _firmware = nullptr;
    CkbSettings::cleanUp();
#ifdef USE_XCB_EWMH
    if(_windowDetector){
        _windowDetector->terminateEventLoop();
        _windowDetector->wait();
    }
#endif
}

void AppController::quit(){
    qApp->quit();
}

void AppController::readSignal(){
    sigNotifier->setEnabled(false);
    int sig = -1;
    int ret = read(signalHandlerFd[1], &sig, sizeof(sig));

    if(ret == -1){
        qDebug() << "Error on QSignalHandler read";
        return;
    }

    qDebug() << "\nSignal" << sig << "caught. Quitting...";
    if(sig == SIGHUP){
        // Restart, but with a delay
        QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList() << "-b" << "-d" << "-s");
    }
    quit();
    sigNotifier->setEnabled(true);
}

bool AppController::checkDaemon(bool silent){
    // check, whether daemon is running
    // the daemon creates the root device path on initialization and thus it
    // can be assumed, that the daemon is not running if doesn't exist
    // `.arg(0)` is necessary to interpolate the correct suffix into the path
    // see `./kbmanager.cpp` for details
    QFileInfo rootDevPath(devpath.arg(0));
    if(rootDevPath.exists() || silent)
        return true;
    setStatusText(QCoreApplication::translate("MainWindow","The ckb-next daemon is not running."));
    return false;
}

void AppController::setStatusText(const QString& text){
    _statusText = text;
    emit statusTextChanged(text);
}

void AppController::addDevice(Kb* device){
    // Connected already?
    if(_devices.contains(device))
        return;
    _devices.append(device);
    // Update connected device count
    updateStatus();
}

void AppController::removeDevice(Kb* device){
    _devices.removeAll(device);
    fwNotified.remove(device);
    // Update connected device count
    updateStatus();
}

void AppController::updateStatus(){
    const CkbVersionNumber& daemonVersion = KbManager::ckbDaemonVersion();
    if(daemonVersion.isNull()){
        setStatusText(QCoreApplication::translate("MainWindow","Driver inactive"));
        return;
    }
    int count = _devices.count();
    // Warn if the daemon version doesn't match the GUI
    QString daemonWarning;
    if(!daemonVersion.isEqualVerStr(KbManager::ckbGuiVersion()))
        daemonWarning = QCoreApplication::translate("MainWindow","<br /><br /><b>Warning:</b> Driver version mismatch (") + daemonVersion.toString() + QCoreApplication::translate("MainWindow","). Please upgrade ckb-next") + QString(KbManager::ckbDaemonVersion() > KbManager::ckbGuiVersion() ? "" : "-daemon") + QCoreApplication::translate("MainWindow",". If the problem persists, try rebooting.");
    if(count == 0){
#if defined(Q_OS_MACOS) && !defined(OS_MAC_LEGACY)
        QProcess kextstat;
        kextstat.start("kextstat", QStringList() << "-l" << "-b" << "org.pqrs.driver.Karabiner.VirtualHIDDevice.v060800");

        if(!kextstat.waitForFinished())
            qDebug() << "Kextstat error";

        QString kextstatOut(kextstat.readAll());
        if(kextstatOut.isEmpty())
            daemonWarning.append(QCoreApplication::translate("MainWindow","<br /><b>Warning:</b> System Extension by \"Fumihiko Takayama\" is not allowed in Security & Privacy. Please allow it and then unplug and replug your devices."));
        if(is_catalina_or_higher())
            daemonWarning.append(QCoreApplication::translate("MainWindow","<br /><b>Warning:</b> Make sure ckb-next-daemon is allowed in Security & Privacy -> Input monitoring.<br />Please allow for up to 10 seconds for the daemon restart prompt to show up after allowing input monitoring."));
#elif defined(Q_OS_LINUX)
            if(!(QFileInfo("/dev/uinput").exists() || QFileInfo("/dev/input/uinput").exists())){
                QProcess modprobe;
                modprobe.start("modprobe", QStringList("uinput"));

                if(!modprobe.waitForFinished())
                    qDebug() << "Modprobe error";

                if(modprobe.exitCode())
                    daemonWarning.append(QCoreApplication::translate("MainWindow","<br /><b>Warning:</b> The uinput module could not be loaded. If this issue persists after rebooting, compile a kernel with CONFIG_INPUT_UINPUT=y."));
            }
#endif
        setStatusText(QCoreApplication::translate("MainWindow","No devices connected") + daemonWarning);
    }
    else if(count == 1)
        setStatusText(QCoreApplication::translate("MainWindow","1 device connected") + daemonWarning);
    else
        setStatusText(QString(QCoreApplication::translate("MainWindow","%1 devices connected")).arg(count) + daemonWarning);
}

void AppController::checkFwUpdates(){
    if(!windowVisible || !windowVisible())
        return;
    for(Kb* device : std::as_const(_devices)){
        // Offer a firmware upgrade if a new version is available
        CkbVersionNumber version = _firmware->versionForBoard(device->productID);
        if(version > device->firmware.app){
            if(fwNotified.contains(device))
                continue;
            fwNotified.insert(device);
            emit firmwareUpdateAvailable(device, version);
            // Don't display more than one of these at once
            return;
        }
    }
}

void AppController::timerTick(){
    // Check shared memory for changes
    if(appShare.lock()){
        void* shmData = appShare.data();
        QStringList commands = QString((const char*)shmData).split("\n");
        // Restore PID, remove all other data
        snprintf((char*)appShare.data(), appShare.size(), "PID %ld", (long)getpid());
        appShare.unlock();
        // Parse commands
        foreach(const QString& line, commands){
            // Old ckb option line - bring application to foreground
            if(line == "Open")
                emit showWindowRequested();
            if(line.startsWith("Option ")){
                // New ckb option line
                QString option = line.section(' ', 1);
                if(option == "Open")
                    // Bring to foreground
                    emit showWindowRequested();
                else if(option == "Close")
                    // Quit application
                    quit();
                else if(option.startsWith("SwitchToProfile"))
                    emit switchToProfileCLI(option.section(' ', 1));
                else if(option.startsWith("SwitchToMode: "))
                    emit switchToModeCLI(option.section(' ', 1));
                else if(option == QLatin1String("Sleep"))
                    KbManager::kbManager()->forceDimLights();
            }
        }
    }
    // Check for firmware updates (when appropriate)
    if(!CkbSettings::get("Program/DisableAutoFWCheck").toBool()){
        _firmware->checkUpdates();
        checkFwUpdates();
    }
}

void AppController::posixSignalHandler(int signal){
    int ret = write(signalHandlerFd[0], &signal, sizeof(signal));
    if(ret == -1)
        qDebug() << "Error on PosixSignalHandler write";
}
