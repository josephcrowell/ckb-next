#include "appcontroller.h"
#include "ckbsettings.h"
#include "kbfirmware.h"
#include "kbmanager.h"
#include "media.h"
#include "xcb/xwindowdetector.h"
#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QSocketNotifier>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

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

void AppController::posixSignalHandler(int signal){
    int ret = write(signalHandlerFd[0], &signal, sizeof(signal));
    if(ret == -1)
        qDebug() << "Error on PosixSignalHandler write";
}
