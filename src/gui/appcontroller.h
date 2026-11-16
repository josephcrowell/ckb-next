#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>

class KbFirmware;
class QSocketNotifier;
class XWindowDetector;

// Application-wide work that doesn't depend on the UI: device manager, firmware table,
// active window detection, POSIX signals and quitting
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

    inline KbFirmware* firmware() const { return _firmware; }
    // Null when active window detection isn't built in
    inline XWindowDetector* windowDetector() const { return _windowDetector; }

public slots:
    void quit();

private slots:
    void readSignal();

private:
    static void posixSignalHandler(int signal);
    static int signalHandlerFd[2];
    static AppController* _instance;

    KbFirmware* _firmware = nullptr;
    XWindowDetector* _windowDetector = nullptr;
    QSocketNotifier* sigNotifier = nullptr;
};

#endif // APPCONTROLLER_H
