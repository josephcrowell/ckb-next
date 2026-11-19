#ifndef DEVICECONTROLLER_H
#define DEVICECONTROLLER_H

#include <QObject>
#include <QStringList>
#include "xcb/xwindowinfo.h"

class Kb;
class KbMode;
class ModeListTableModel;
class XWindowDetector;

// UI-independent logic for one device, shared by the Widgets and QML UIs
class DeviceController : public QObject
{
    Q_OBJECT
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
    bool selectProfile(int index);

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

    void saveToHardware();

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

private slots:
    void updateCurrentMode();

private:
    Kb* _device;
    ModeListTableModel* _modes;
    KbMode* _currentMode = nullptr;
    // Mode to return to when the focused window no longer matches any mode
    KbMode* prevmode = nullptr;
};

#endif // DEVICECONTROLLER_H
