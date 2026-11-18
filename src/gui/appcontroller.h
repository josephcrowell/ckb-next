#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QList>
#include <QObject>
#include <QSet>
#include <functional>
#include "ckbversionnumber.h"

class Kb;
class KbFirmware;
class QSocketNotifier;
class XWindowDetector;

// Application-wide work that doesn't depend on the UI: device manager, firmware table,
// active window detection, POSIX signals, command-line requests, status text and quitting
class AppController : public QObject
{
    Q_OBJECT
public:
    explicit AppController(QObject* parent = nullptr);
    ~AppController();

    static AppController* instance() { return _instance; }

    // Starts the device manager, signal handling and window detection
    void init();
    // Looks for devices; call once the UI listens to KbManager
    void startScan();
    // Stops everything init() started; call after the device UI is gone
    void shutdown();

    // Returns false and sets the status text if the daemon isn't running (unless silent)
    bool checkDaemon(bool silent);
    inline QString statusText() const { return _statusText; }
    // Connected devices in the order they were connected
    inline const QList<Kb*>& devices() const { return _devices; }
    // Firmware update prompts are only offered while this returns true (main window visible)
    inline void setWindowVisibleCheck(std::function<bool()> check) { windowVisible = std::move(check); }

    inline KbFirmware* firmware() const { return _firmware; }
    // Null when active window detection isn't built in
    inline XWindowDetector* windowDetector() const { return _windowDetector; }

    // Checks for a new ckb-next version unless Program/DisableAutoUpdCheck is set
    void autoCheckForUpdates();

public slots:
    void quit();
    // Checks for a new ckb-next version and emits updateChecked (no-op with DISABLE_UPDATER)
    void checkForUpdates();

signals:
    // An empty version means no update was found (up to date, or the check failed)
    void updateChecked(const QString& version, const QString& changelog);
    void showWindowRequested();
    void switchToProfileCLI(QString profile);
    void switchToModeCLI(QString mode);
    void statusTextChanged(const QString& text);
    // Emitted once per device when a newer firmware is available
    void firmwareUpdateAvailable(Kb* device, CkbVersionNumber version);

private slots:
    void readSignal();
    void addDevice(Kb* device);
    void removeDevice(Kb* device);
    void updateStatus();
    void timerTick();

private:
    static void posixSignalHandler(int signal);
    static int signalHandlerFd[2];
    static AppController* _instance;

    void setStatusText(const QString& text);
    void checkFwUpdates();

    KbFirmware* _firmware = nullptr;
    XWindowDetector* _windowDetector = nullptr;
    QSocketNotifier* sigNotifier = nullptr;
    QList<Kb*> _devices;
    // Devices whose firmware update has already been offered
    QSet<Kb*> fwNotified;
    QString _statusText;
    std::function<bool()> windowVisible;
};

#endif // APPCONTROLLER_H
