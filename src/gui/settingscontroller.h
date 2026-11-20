#ifndef SETTINGSCONTROLLER_H
#define SETTINGSCONTROLLER_H

#include <QObject>
#include <QStringList>

class QProcess;

// UI-independent logic of the Settings tab (SettingsWidget and ExtraSettingsWidget), shared by
// the Widgets and QML UIs. Loading it applies the stored Program/ settings; each setter saves the
// setting and applies it. The UI asks every question; the report runs in steps so it can ask.
class SettingsController : public QObject
{
    Q_OBJECT
public:
    explicit SettingsController(QObject* parent = nullptr);

    static QString copyrightText();

    // Modifier remap. Keys are "caps", "lshift", "lctrl", "lalt", "lwin"; each is mapped to one
    // of the modifiers, by index in modifierNames(). The right-hand keys follow the left ones.
    static QStringList modifierNames();
    // The index the key is mapped to, or -1 if it isn't mapped to a modifier
    int modifierIndex(const QString& key) const;
    // Indices for caps, shift, ctrl, alt and win; saved at once
    void setModifiers(int caps, int shift, int ctrl, int alt, int win);
#ifdef Q_OS_MACOS
    // ckb <0.0.41 stored a Command/Control swap; it's turned into the remap once. Returns
    // whether that happened (the UI then reloads the indices).
    bool migrateCmdSwap();
#endif

    bool autoFirmwareCheck() const;
    void setAutoFirmwareCheck(bool enabled);
    bool autoUpdateCheck() const;
    void setAutoUpdateCheck(bool enabled);
    // Qt 5.6 to 5.15 only; takes effect after a restart
    bool hiDpi() const;
    void setHiDpi(bool enabled);

    // Frame rate 1-60 (30 if the stored one is out of range); above 30 the UI warns
    inline int frameRate() const { return _frameRate; }
    void setFrameRate(int rate);
    static inline bool frameRateWarning(int rate) { return rate > 30; }
    // Brightness per mode instead of shared by all devices
    bool perModeBrightness() const;
    void setPerModeBrightness(bool enabled);
    // Saves the shared brightness when it changed (called on the app's timer)
    void pollUpdates();
    bool dither() const;
    void setDither(bool enabled);
    bool trayIconSuppressed() const;
    void setTrayIconSuppressed(bool suppressed);
    bool monochromeTrayIcon() const;
    void setMonochromeTrayIcon(bool monochrome);
    bool startDelay() const;
    void setStartDelay(bool enabled);
    bool disablePreviewOnFocusLoss() const;
    void setDisablePreviewOnFocusLoss(bool disabled);
    bool msaaDisabled() const;
    void setMsaaDisabled(bool disabled);
    // Idle timer (Linux): unsupported compositors can't use it
    static bool idleTimerSupported();
    bool idleTimerEnabled() const;
    void setIdleTimerEnabled(bool enabled);
    int idleTimerDuration() const;
    void setIdleTimerDuration(int minutes);
#if defined(Q_OS_MACOS) && defined(OS_MAC_LEGACY)
    bool mouseAccelDisabled() const;
    void setMouseAccelDisabled(bool disabled);
    bool scrollAccelDisabled() const;
    void setScrollAccelDisabled(bool disabled);
    int scrollSpeed() const;
    void setScrollSpeed(int speed);
#endif

    // Animations: the folders searched and a new scan (returns the "N animations found" text)
    static QString animationPaths();
    QString rescanAnimations();

    // Report: startReport() runs ckb-next-dev-detect (false if it couldn't be started), then
    // reportFinished() follows. On success the UI asks for a folder, calls saveReport() and,
    // if that worked, showReport(); finally finishReport(), removing the script's report
    // after a success.
    inline bool reportRunning() const { return _devDetect != nullptr; }
    bool startReport();
    // Copies the report into the folder; false if it couldn't be copied
    bool saveReport(const QString& folder);
    // Shows the copied report in the file manager
    void showReport();
    void finishReport(bool removeReport);

#ifdef Q_OS_MACOS
    // Opens a terminal running the uninstall script
    void uninstall();
#endif

signals:
    // ok: the script succeeded and wrote its report; otherwise errorOutput and exitCode tell why
    void reportFinished(bool ok, const QString& errorOutput, int exitCode);

private:
    void devDetectFinished(int exitCode);

    int _frameRate = 30;
    int _lastSharedDimming = -2;
    int _scrollSpeedValue = 3;
    bool _scrollAccelDisabled = false;
    QProcess* _devDetect = nullptr;
    QString _reportFolder;
    QString _savedReport;
};

#endif // SETTINGSCONTROLLER_H
