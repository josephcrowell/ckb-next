#ifndef DEVICECONTROLLER_H
#define DEVICECONTROLLER_H

#include <QAbstractItemModel>
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
    Q_PROPERTY(QAbstractItemModel* modeModel READ modeModel CONSTANT)
    Q_PROPERTY(int currentModeIndex READ currentModeIndex NOTIFY currentModeChanged)
    Q_PROPERTY(bool canManageModeEvents READ canManageModeEvents CONSTANT)
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
    // The same model for QML, which reads it through ModeListTableModel's named roles
    QAbstractItemModel* modeModel() const;
    // Row of the device's current mode, or -1
    int currentModeIndex() const;
    // Window rules need active window detection and the QML event manager
    bool canManageModeEvents() const;

    // Profiles, in device order
    QStringList profileNames() const;
    int currentProfileIndex() const;
    int hardwareProfileIndex() const;
    // Switch to the profile at index. False if index is past the last profile.
    Q_INVOKABLE bool selectProfile(int index);

    // The mode last made current, or nullptr after it was deleted
    inline KbMode* currentMode() const { return _currentMode; }
    // Switch to the mode at row. On the "New mode" row, add a mode and return its row; else -1.
    Q_INVOKABLE int selectMode(int row);
    // Rename the mode at row; ignored on the "New mode" row
    Q_INVOKABLE void renameMode(int row, const QString& name);
    // Whether row holds the current mode
    Q_INVOKABLE bool isCurrentModeRow(int row) const;
    // Modes required by the hardware can't be deleted
    Q_INVOKABLE bool canDeleteMode() const;
    Q_INVOKABLE bool canMoveModeUp(int row) const;
    Q_INVOKABLE bool canMoveModeDown(int row) const;
    // These act on the current mode, which is at row
    Q_INVOKABLE void duplicateMode(int row);
    Q_INVOKABLE void deleteMode(int row);
    Q_INVOKABLE void moveModeUp(int row);
    Q_INVOKABLE void moveModeDown(int row);
    // Move the mode at row from so it ends up at row to, as dragging does
    Q_INVOKABLE void moveMode(int from, int to);
    // Toggle the current mode's window rules. True if it has none yet, so they need editing.
    Q_INVOKABLE bool toggleModeEvents(int row);
    Q_INVOKABLE void enableModeEvents();

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
    // The device's current mode changed (not emitted when only its row moves)
    void currentModeChanged();
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
