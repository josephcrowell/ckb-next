#ifndef SETTINGSCONTROLLER_H
#define SETTINGSCONTROLLER_H

#include <QObject>
#include <QStringList>
#include <QVariantList>

class QProcess;

// UI-independent logic of the Settings tab (SettingsWidget and ExtraSettingsWidget), shared by
// the Widgets and QML UIs. Loading it applies the stored Program/ settings; each setter saves the
// setting and applies it. The UI asks every question; the report runs in steps so it can ask.
class SettingsController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString copyrightText READ copyrightText CONSTANT)
    Q_PROPERTY(QStringList modifierNames READ modifierNames CONSTANT)
    Q_PROPERTY(bool autoFirmwareCheck READ autoFirmwareCheck WRITE setAutoFirmwareCheck NOTIFY autoFirmwareCheckChanged)
    Q_PROPERTY(bool updaterAvailable READ updaterAvailable CONSTANT)
    Q_PROPERTY(bool autoUpdateCheck READ autoUpdateCheck WRITE setAutoUpdateCheck NOTIFY autoUpdateCheckChanged)
    Q_PROPERTY(bool trayIconSuppressed READ trayIconSuppressed WRITE setTrayIconSuppressed NOTIFY trayIconSuppressedChanged)
    Q_PROPERTY(bool monochromeTrayIcon READ monochromeTrayIcon WRITE setMonochromeTrayIcon NOTIFY monochromeTrayIconChanged)
    Q_PROPERTY(bool perModeBrightness READ perModeBrightness WRITE setPerModeBrightness NOTIFY perModeBrightnessChanged)
    Q_PROPERTY(bool startDelay READ startDelay WRITE setStartDelay NOTIFY startDelayChanged)
    Q_PROPERTY(bool disablePreviewOnFocusLoss READ disablePreviewOnFocusLoss WRITE setDisablePreviewOnFocusLoss NOTIFY disablePreviewOnFocusLossChanged)
    Q_PROPERTY(bool msaaDisabled READ msaaDisabled WRITE setMsaaDisabled NOTIFY msaaDisabledChanged)
    Q_PROPERTY(bool idleTimerSupported READ idleTimerSupported CONSTANT)
    Q_PROPERTY(bool idleTimerEnabled READ idleTimerEnabled WRITE setIdleTimerEnabled NOTIFY idleTimerEnabledChanged)
    Q_PROPERTY(int idleTimerDuration READ idleTimerDuration WRITE setIdleTimerDuration NOTIFY idleTimerDurationChanged)
    Q_PROPERTY(int frameRate READ frameRate WRITE setFrameRate NOTIFY frameRateChanged)
    Q_PROPERTY(bool showFrameRateWarning READ showFrameRateWarning NOTIFY frameRateChanged)
    Q_PROPERTY(bool dither READ dither WRITE setDither NOTIFY ditherChanged)
    Q_PROPERTY(QString animationPaths READ animationPaths CONSTANT)
    Q_PROPERTY(QString animationCountText READ animationCountText NOTIFY animationsScanned)
public:
    explicit SettingsController(QObject* parent = nullptr);

    static QString copyrightText();

    // Modifier remap. Keys are "caps", "lshift", "lctrl", "lalt", "lwin"; each is mapped to one
    // of the modifiers, by index in modifierNames(). The right-hand keys follow the left ones.
    static QStringList modifierNames();
    // The index the key is mapped to, or -1 if it isn't mapped to a modifier
    Q_INVOKABLE int modifierIndex(const QString& key) const;
    // Indices for caps, shift, ctrl, alt and win; saved at once
    Q_INVOKABLE void setModifiers(int caps, int shift, int ctrl, int alt, int win);
#ifdef Q_OS_MACOS
    // ckb <0.0.41 stored a Command/Control swap; it's turned into the remap once. Returns
    // whether that happened (the UI then reloads the indices).
    bool migrateCmdSwap();
#endif

    bool autoFirmwareCheck() const;
    void setAutoFirmwareCheck(bool enabled);
    // False when built with DISABLE_UPDATER (distribution packages)
    static bool updaterAvailable();
    bool autoUpdateCheck() const;
    void setAutoUpdateCheck(bool enabled);
    // Qt 5.6 to 5.15 only; takes effect after a restart
    bool hiDpi() const;
    void setHiDpi(bool enabled);

    // Frame rate 1-60 (30 if the stored one is out of range); above 30 the UI warns
    inline int frameRate() const { return _frameRate; }
    void setFrameRate(int rate);
    static inline bool frameRateWarning(int rate) { return rate > 30; }
    inline bool showFrameRateWarning() const { return frameRateWarning(_frameRate); }
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
    // As read on load: CkbSettings writes this one later, from another thread
    inline bool msaaDisabled() const { return _msaaDisabled; }
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
    Q_INVOKABLE QString rescanAnimations();
    // The text of the last scan, empty before the first
    inline QString animationCountText() const { return _animationCountText; }
    // One map per loaded script: name, path, version, author, year, license and the
    // description as plain text (the details dialog's columns)
    Q_INVOKABLE QVariantList animationDetails() const;

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
    void autoFirmwareCheckChanged();
    void autoUpdateCheckChanged();
    void trayIconSuppressedChanged();
    void monochromeTrayIconChanged();
    void perModeBrightnessChanged();
    void startDelayChanged();
    void disablePreviewOnFocusLossChanged();
    void msaaDisabledChanged();
    void idleTimerEnabledChanged();
    void idleTimerDurationChanged();
    void frameRateChanged();
    void ditherChanged();
    void animationsScanned();
    // ok: the script succeeded and wrote its report; otherwise errorOutput and exitCode tell why
    void reportFinished(bool ok, const QString& errorOutput, int exitCode);

private:
    void devDetectFinished(int exitCode);

    int _frameRate = 30;
    int _lastSharedDimming = -2;
    int _scrollSpeedValue = 3;
    bool _scrollAccelDisabled = false;
    bool _msaaDisabled = false;
    QProcess* _devDetect = nullptr;
    QString _reportFolder;
    QString _savedReport;
    QString _animationCountText;
};

#endif // SETTINGSCONTROLLER_H
