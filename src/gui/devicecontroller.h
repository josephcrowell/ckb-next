#ifndef DEVICECONTROLLER_H
#define DEVICECONTROLLER_H

#include <QObject>
#include "xcb/xwindowinfo.h"

class Kb;
class KbMode;
class XWindowDetector;

// UI-independent logic for one device, shared by the Widgets and QML UIs
class DeviceController : public QObject
{
    Q_OBJECT
public:
    DeviceController(Kb* device, XWindowDetector* windowDetector, QObject* parent = nullptr);

    inline Kb* device() const { return _device; }

public slots:
    // Switch to the first mode whose window rules match, or back to the mode used before
    void switchToModeByFocus(XWindowInfo win);

private:
    Kb* _device;
    // Mode to return to when the focused window no longer matches any mode
    KbMode* prevmode = nullptr;
};

#endif // DEVICECONTROLLER_H
