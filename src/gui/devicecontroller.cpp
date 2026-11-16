#include "devicecontroller.h"
#include "kb.h"
#include "kbwindowinfo.h"
#include "xcb/xwindowdetector.h"
#include <QDebug>

DeviceController::DeviceController(Kb* device, XWindowDetector* windowDetector, QObject* parent) :
    QObject(parent), _device(device)
{
#ifdef USE_XCB_EWMH
    if(windowDetector)
        connect(windowDetector, &XWindowDetector::activeWindowChanged, this, &DeviceController::switchToModeByFocus);
#else
    Q_UNUSED(windowDetector);
#endif
}

// Returns true if a match is found
static inline bool checkForWinInfoMatch(KbWindowInfo* kbinfo, XWindowInfo* wininfo) {
    if(kbinfo->isEmpty() || !kbinfo->isEnabled())
        return false;

    QVector<KbWindowInfo::MatchPair>& rules = kbinfo->items;
    bool result = false;
    for(int i = 0; i < rules.length(); i++){
        const KbWindowInfo::MatchPair& mp = rules.at(i);

        Qt::CaseSensitivity sensitivity = Qt::CaseSensitive;
        if(mp.flags.testFlag(KbWindowInfo::MATCH_CASE_INSENSITIVE))
            sensitivity = Qt::CaseInsensitive;

        const QString* target = nullptr;

        switch(mp.type){
        case KbWindowInfo::MATCH_TYPE_WINDOW_TITLE:
            target = &wininfo->windowTitle;
            break;
        case KbWindowInfo::MATCH_TYPE_PROGRAM_PATH:
            target = &wininfo->program;
            break;
        case KbWindowInfo::MATCH_TYPE_WM_CLASS_NAME:
            target = &wininfo->wm_class_name;
            break;
        case KbWindowInfo::MATCH_TYPE_WM_INSTANCE_NAME:
            target = &wininfo->wm_instance_name;
            break;
        default:
            qDebug() << "Invalid match type" << mp.type;
            return false;
        }

        // Do the comparison
        if(mp.flags.testFlag(KbWindowInfo::MATCH_SUBSTRING))
            result = target->contains(mp.item, sensitivity);
        else if(mp.flags.testFlag(KbWindowInfo::MATCH_STARTS_WITH))
            result = target->startsWith(mp.item, sensitivity);
        else if(mp.flags.testFlag(KbWindowInfo::MATCH_ENDS_WITH))
            result = target->endsWith(mp.item, sensitivity);
        else
            result = !target->compare(mp.item, sensitivity);

        // If it's an OR and we found a match, return immediately
        // If it's an AND and we haven't found a match, also return immediately
        if(mp.op == KbWindowInfo::MATCH_OP_OR && result)
            break;
        else if(mp.op == KbWindowInfo::MATCH_OP_AND && !result)
            break;
    }
    return result;
}

void DeviceController::switchToModeByFocus(XWindowInfo win) {
    if(win.isEmpty())
        return;

    KbProfile* currentProfile = _device->currentProfile();
    int len = currentProfile->modes().length();
    for(int i = 0; i < len; i++)
    {
        KbMode* loopMode = currentProfile->modes().at(i);
        if(!checkForWinInfoMatch(loopMode->winInfo(), &win))
            continue;

        if(!prevmode)
            prevmode = _device->currentMode();

        // Set the new mode
        _device->setCurrentMode(loopMode);
        return;
    }
    // If we got here, we found no match
    const int mode = currentProfile->indexOf(prevmode);
    if(prevmode && mode != -1)
        _device->setCurrentMode(prevmode);

    prevmode = nullptr;
}
