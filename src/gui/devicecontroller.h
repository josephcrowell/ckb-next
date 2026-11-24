#ifndef DEVICECONTROLLER_H
#define DEVICECONTROLLER_H

#include <QList>
#include <QObject>
#include <QPair>
#include <QStringList>
#include "batterystatus.h"
#include "ckbversionnumber.h"
#include "xcb/xwindowinfo.h"

class Kb;
class KbMode;
class ModeListTableModel;
class XWindowDetector;

// UI-independent logic for one device, shared by the Widgets and QML UIs
class DeviceController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(bool hasBattery READ hasBattery CONSTANT)
    Q_PROPERTY(bool showBatteryIndicator READ showBatteryIndicator WRITE setShowBatteryIndicator NOTIFY showBatteryIndicatorChanged)
    Q_PROPERTY(QString batteryIconName READ batteryIconName NOTIFY batteryTextChanged)
    Q_PROPERTY(QString batteryLevelText READ batteryLevelText NOTIFY batteryTextChanged)
    Q_PROPERTY(QString batteryStatusText READ batteryStatusText NOTIFY batteryTextChanged)
    Q_PROPERTY(QString batteryToolTip READ batteryToolTip NOTIFY batteryTextChanged)
    Q_PROPERTY(QStringList profileNames READ profileNames NOTIFY profilesChanged)
    Q_PROPERTY(int hardwareProfileIndex READ hardwareProfileIndex NOTIFY profilesChanged)
    Q_PROPERTY(int currentProfileIndex READ currentProfileIndex NOTIFY currentProfileChanged)
    Q_PROPERTY(bool canSaveToHardware READ canSaveToHardware CONSTANT)
    Q_PROPERTY(bool hasLightTab READ hasLightTab CONSTANT)
    Q_PROPERTY(bool hasBindTab READ hasBindTab CONSTANT)
    Q_PROPERTY(bool hasLegacyLightTab READ hasLegacyLightTab CONSTANT)
    Q_PROPERTY(bool hasKeyboardPerfTab READ hasKeyboardPerfTab CONSTANT)
    Q_PROPERTY(bool hasMousePerfTab READ hasMousePerfTab CONSTANT)
public:
    DeviceController(Kb* device, XWindowDetector* windowDetector, QObject* parent = nullptr);

    inline Kb* device() const { return _device; }
    // Modes of the current profile, plus a "New mode" row at the end
    inline ModeListTableModel* modes() const { return _modes; }

    // Profiles, in device order
    QStringList profileNames() const;
    int currentProfileIndex() const;
    int hardwareProfileIndex() const;
    // Switch to the profile at index. False if index is past the last profile.
    Q_INVOKABLE bool selectProfile(int index);

    // The mode last made current, or nullptr after it was deleted
    inline KbMode* currentMode() const { return _currentMode; }
    // Switch to the mode at row. On the "New mode" row, add a mode and return its row; else -1.
    int selectMode(int row);
    // Whether row holds the current mode
    bool isCurrentModeRow(int row) const;
    // Modes required by the hardware can't be deleted
    bool canDeleteMode() const;
    bool canMoveModeUp(int row) const;
    bool canMoveModeDown(int row) const;
    // These act on the current mode, which is at row
    void duplicateMode(int row);
    void deleteMode(int row);
    void moveModeUp(int row);
    void moveModeDown(int row);
    // Toggle the current mode's window rules. True if it has none yet, so they need editing.
    bool toggleModeEvents(int row);
    void enableModeEvents();

    Q_INVOKABLE void saveToHardware();
    // Only devices with the hwload feature keep profiles in hardware
    bool canSaveToHardware() const;

    // Which sub-tabs the device page has, besides Device
    bool hasLightTab() const;
    bool hasBindTab() const;
    bool hasLegacyLightTab() const;
    bool hasKeyboardPerfTab() const;
    bool hasMousePerfTab() const;

    // Device information
    QString modelName() const;
    QString serial() const;
    QString firmwareVersion() const;
    QString bootloaderVersion() const;
    // Not every wireless device has a radio firmware or bootloader
    bool hasRadioFirmware() const;
    QString radioFirmwareVersion() const;
    bool hasRadioBootloader() const;
    QString radioBootloaderVersion() const;

    // Poll rates the device supports, fastest first, as "<n> ms"
    QStringList pollRates() const;
    bool hasPollRate() const;
    bool canSetPollRate() const;
    // Index of the current poll rate in pollRates() (0 if unknown)
    int pollRateIndex() const;
    void setPollRate(int index);

    // Choose the device's layout (stored, an old global one, or from the locale) and apply it.
    // Call once, where KbWidget's constructor did.
    void setupLayout();
    // Layouts the device supports, as (KeyMap::Layout, name); empty without the "bind" feature
    inline const QList<QPair<int, QString>>& layouts() const { return _layouts; }
    // Index of the layout setupLayout() chose in layouts()
    inline int layoutIndex() const { return _layoutIndex; }
    // Apply and store the layout at index in layouts()
    void setLayout(int index);

    // The device's short name, as shown on its tab and battery icon
    QString name() const;

    bool hasBattery() const;
    bool showBatteryIndicator() const;
    void setShowBatteryIndicator(bool show);
    // Battery tray icon: image name under :/img/ (the plain monochrome icon until the first
    // reading), "Battery: <level> (<n>%)", "Status: <status>" and the tool tip
    QString batteryIconName() const;
    QString batteryLevelText() const;
    QString batteryStatusText() const;
    QString batteryToolTip() const;

    enum FirmwareCheck {
        FIRMWARE_CHECK_FAILED,
        FIRMWARE_NEEDS_NEWER_CKB,
        FIRMWARE_UP_TO_DATE,
        FIRMWARE_UPGRADE_AVAILABLE,
    };
    bool hasFirmwareUpdates() const;
    // Whether the list of firmware versions has been downloaded yet
    bool firmwareListDownloaded() const;
    // "Check for updates", "Up to date" or "Upgrade to v<version>"
    QString firmwareButtonText() const;
    // Compare the device's firmware with the newest one, downloading the list first if needed
    FirmwareCheck checkFirmware(CkbVersionNumber& newVersion);

public slots:
    // Switch to the first mode whose window rules match, or back to the mode used before
    void switchToModeByFocus(XWindowInfo win);
    // Switch by name, from the command line
    void switchToProfile(const QString& profile);
    void switchToMode(const QString& mode);

signals:
    // The profile list needs rebuilding
    void profilesChanged();
    // switchToProfile() selected the profile at index
    void profileSwitched(int index);
    // The device's current profile changed, by any route
    void currentProfileChanged();
    // Device information, poll rate, layout or battery indicator setting changed
    void infoUpdated();
    // Only for devices with a battery
    void batteryChanged(uint level, BatteryStatus status);
    void batteryTextChanged();
    void showBatteryIndicatorChanged(bool show);

private slots:
    void updateCurrentMode();

private:
    Kb* _device;
    ModeListTableModel* _modes;
    KbMode* _currentMode = nullptr;
    QList<QPair<int, QString>> _layouts;
    int _layoutIndex = -1;
    // Whether the daemon has reported the battery yet
    bool _batteryKnown = false;
    // Mode to return to when the focused window no longer matches any mode
    KbMode* prevmode = nullptr;
};

#endif // DEVICECONTROLLER_H
