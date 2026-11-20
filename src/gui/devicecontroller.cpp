#include "devicecontroller.h"
#include "appcontroller.h"
#include "ckbsettings.h"
#include "kb.h"
#include "kbfirmware.h"
#include "kbwindowinfo.h"
#include "modelisttablemodel.h"
#include "xcb/xwindowdetector.h"
#include <QDebug>

DeviceController::DeviceController(Kb* device, XWindowDetector* windowDetector, QObject* parent) :
    QObject(parent), _device(device), _modes(new ModeListTableModel(device, this))
{
#ifdef USE_XCB_EWMH
    if(windowDetector)
        connect(windowDetector, &XWindowDetector::activeWindowChanged, this, &DeviceController::switchToModeByFocus);
#else
    Q_UNUSED(windowDetector);
#endif
    connect(device, &Kb::profileRenamed, this, &DeviceController::profilesChanged);
    connect(device, &Kb::profileAdded, this, &DeviceController::profilesChanged);
    connect(device, &Kb::modeChanged, this, &DeviceController::updateCurrentMode);
    connect(device, &Kb::infoUpdated, this, &DeviceController::infoUpdated);
    if(hasBattery())
        connect(device, &Kb::batteryChanged, this, &DeviceController::batteryChanged);
    connect(AppController::instance(), &AppController::switchToProfileCLI, this, &DeviceController::switchToProfile);
    connect(AppController::instance(), &AppController::switchToModeCLI, this, &DeviceController::switchToMode);
}

QStringList DeviceController::profileNames() const {
    QStringList names;
    foreach(KbProfile* profile, _device->profiles())
        names.append(profile->name());
    return names;
}

int DeviceController::currentProfileIndex() const {
    return _device->profiles().indexOf(_device->currentProfile());
}

int DeviceController::hardwareProfileIndex() const {
    return _device->profiles().indexOf(_device->hwProfile());
}

bool DeviceController::selectProfile(int index){
    if(index >= _device->profiles().count())
        return false;
    _device->setCurrentProfile(_device->profiles()[index]);
    // Device will emit profileChanged() and modeChanged() signals to update UI
    return true;
}

void DeviceController::updateCurrentMode(){
    if(_device->currentProfile()->indexOf(_device->currentMode()) < 0)
        return;
    _currentMode = _device->currentMode();
}

int DeviceController::selectMode(int row){
    if(row > _device->currentProfile()->modeCount() - 1)
        return _modes->addNewMode();
    KbMode* mode = _device->currentProfile()->at(row);
    _device->setCurrentMode(mode);
    return -1;
}

bool DeviceController::isCurrentModeRow(int row) const {
    KbProfile* currentProfile = _device->currentProfile();
    if(row < 0 || !_currentMode || row > currentProfile->modeCount() - 1)
        return false;
    return currentProfile->modes().at(row) == _currentMode;
}

bool DeviceController::canDeleteMode() const {
    return _device->currentProfile()->modeCount() > _device->hwModeCount;
}

bool DeviceController::canMoveModeUp(int row) const {
    return row != 0;
}

bool DeviceController::canMoveModeDown(int row) const {
    return row < _device->currentProfile()->modeCount() - 1;
}

void DeviceController::duplicateMode(int row){
    KbProfile* currentProfile = _device->currentProfile();
    KbMode* newMode = _device->newMode(_currentMode);
    newMode->newId();
    _modes->profileAboutToChange();
    currentProfile->insert(row + 1, newMode);
    _modes->profileChanged();
    _device->setCurrentMode(newMode);
}

void DeviceController::deleteMode(int row){
    KbProfile* currentProfile = _device->currentProfile();
    _modes->profileAboutToChange();
    currentProfile->removeAll(_currentMode);
    _currentMode->deleteLater();
    _currentMode = nullptr;
    // Select next mode
    _modes->profileChanged();
    if(row < currentProfile->modeCount())
        _device->setCurrentMode(currentProfile->modes()[row]);
    else
        _device->setCurrentMode(currentProfile->modes().last());
}

void DeviceController::moveModeUp(int row){
    KbProfile* currentProfile = _device->currentProfile();
    _modes->profileAboutToChange();
    currentProfile->removeAll(_currentMode);
    currentProfile->insert(row - 1, _currentMode);
    _modes->profileChanged();
}

void DeviceController::moveModeDown(int row){
    KbProfile* currentProfile = _device->currentProfile();
    _modes->profileAboutToChange();
    currentProfile->removeAll(_currentMode);
    currentProfile->insert(row + 1, _currentMode);
    _modes->profileChanged();
}

bool DeviceController::toggleModeEvents(int row){
    if(row > _device->currentProfile()->modeCount() - 1)
        return false;
    _currentMode->winInfo()->setEnabled(!_currentMode->winInfo()->isEnabled());
    return _currentMode->winInfo()->isEmpty();
}

void DeviceController::enableModeEvents(){
    // If the current state is "enabled", the previous one was "disabled", which means the user
    // most likely just wanted to edit, and not disable it, so re-enable it.
    if(!_currentMode->winInfo()->isEnabled())
        _currentMode->winInfo()->setEnabled(true);
}

void DeviceController::saveToHardware(){
    _modes->profileAboutToChange();
    _device->save();
    _device->hwSave();
    emit profilesChanged();
    _modes->profileChanged();
}

QString DeviceController::modelName() const {
    return _device->usbModel;
}

QString DeviceController::serial() const {
    return _device->usbSerial;
}

QString DeviceController::firmwareVersion() const {
    return _device->firmware.app.toString();
}

QString DeviceController::bootloaderVersion() const {
    return _device->firmware.bld.toString();
}

bool DeviceController::hasRadioFirmware() const {
    return !_device->firmware.radioapp.isNull();
}

QString DeviceController::radioFirmwareVersion() const {
    return _device->firmware.radioapp.toString();
}

bool DeviceController::hasRadioBootloader() const {
    return !_device->firmware.radiobld.isNull();
}

QString DeviceController::radioBootloaderVersion() const {
    return _device->firmware.radiobld.toString();
}

// Indexed by Kb::pollrate_t
static const char* const pollRateNames[Kb::POLLRATE_COUNT] = {
    "8 ms", "4 ms", "2 ms", "1 ms", "0.5 ms", "0.25 ms", "0.1 ms",
};

QStringList DeviceController::pollRates() const {
    // Remove unsupported pollrates
    int fastest = Kb::POLLRATE_COUNT - 1;
    if(_device->maxpollrate != Kb::POLLRATE_UNKNOWN)
        fastest = _device->maxpollrate;
    QStringList rates;
    for(int i = fastest; i >= 0; i--)
        rates.append(QLatin1String(pollRateNames[i]));
    return rates;
}

bool DeviceController::hasPollRate() const {
    return _device->pollrate != Kb::POLLRATE_UNKNOWN;
}

bool DeviceController::canSetPollRate() const {
    return _device->adjrate;
}

int DeviceController::pollRateIndex() const {
    if(_device->pollrate == Kb::POLLRATE_UNKNOWN)
        return 0;
    return pollRates().count() - _device->pollrate - 1;
}

void DeviceController::setPollRate(int index){
    const QString str = pollRates().at(index);
    _device->setPollRate(str.left(str.indexOf(QLatin1String(" ms"))));
}

void DeviceController::setupLayout(){
    // Read device layout
    if(!_device->features.contains("bind")){
        _device->layout(KeyMap::GB, false);
        return;
    }
    // Load the current device's layout from the settings
    QString layoutSettingsPath("Devices/%1");
    CkbSettings settings(layoutSettingsPath.arg(_device->usbSerial));

    _layouts = KeyMap::layoutNames(_device->hwlayout);

    KeyMap::Layout layout = KeyMap::getLayout(settings.value("hwLayout").toString());
    if(layout == KeyMap::NO_LAYOUT){
        // If the layout hasn't been set yet, first check if one was set globally from a previous version
        // If not, try to pick an appropriate one that's supported by the hardware
        KeyMap::Layout oldLayout = KeyMap::getLayout(CkbSettings::get("Program/KbdLayout").toString());
        if(oldLayout == KeyMap::NO_LAYOUT){
            layout = KeyMap::locale(&_layouts);
        } else {
            CkbSettings::set("Program/KbdLayout", "");
            layout = oldLayout;
        }
    }
    // Find the position of the layout in the list
    int layoutpos = -1;
    if(layout != KeyMap::NO_LAYOUT){
        for(int i = 0; i < _layouts.count(); i++){
            if(_layouts.at(i).first == (int)layout){
                layoutpos = i;
                break;
            }
        }
    }
    // If no layout was found, pick the first one from the list
    if(layoutpos == -1){
        layout = (KeyMap::Layout)_layouts.at(0).first;
        layoutpos = 0;
    }
    _layoutIndex = layoutpos;

    // Set the layout and save it
    _device->layout(layout, false);
}

void DeviceController::setLayout(int index){
    KeyMap::Layout layout = (KeyMap::Layout)_layouts.at(index).first;
    // Only set the layout if it was changed
    if(layout == _device->getCurrentLayout())
        return;
    QString layoutSettingsPath("Devices/%1/hwLayout");
    CkbSettings::set(layoutSettingsPath.arg(_device->usbSerial), KeyMap::getLayout(layout));
    _device->layout(layout, true);
}

bool DeviceController::hasBattery() const {
    return _device->features.contains("battery");
}

bool DeviceController::showBatteryIndicator() const {
    return _device->showBatteryIndicator();
}

void DeviceController::setShowBatteryIndicator(bool show){
    if(!hasBattery())
        return;
    _device->setShowBatteryIndicator(show);
    _device->needsSave();
}

bool DeviceController::hasFirmwareUpdates() const {
    return _device->features.contains("fwupdate");
}

bool DeviceController::firmwareListDownloaded() const {
    return AppController::instance()->firmware()->hasDownloaded();
}

QString DeviceController::firmwareButtonText() const {
    KbFirmware* firmware = AppController::instance()->firmware();
    if(!firmware->hasDownloaded())
        return QCoreApplication::translate("KbWidget", "Check for updates");
    CkbVersionNumber newVersion = firmware->versionForBoard(_device->productID);
    const CkbVersionNumber& oldVersion = _device->firmware.app;
    if(newVersion.isNull() || newVersion <= oldVersion)
        return QCoreApplication::translate("KbWidget", "Up to date");
    return QCoreApplication::translate("KbWidget", "Upgrade to v%1").arg(newVersion.toString());
}

DeviceController::FirmwareCheck DeviceController::checkFirmware(CkbVersionNumber& newVersion){
    newVersion = AppController::instance()->firmware()->versionForBoard(_device->productID, true);
    const CkbVersionNumber& oldVersion = _device->firmware.app;
    if(newVersion.isNull())
        return FIRMWARE_CHECK_FAILED;
    if(newVersion.CkbTooOld())
        return FIRMWARE_NEEDS_NEWER_CKB;
    if(newVersion <= oldVersion)
        return FIRMWARE_UP_TO_DATE;
    return FIRMWARE_UPGRADE_AVAILABLE;
}

void DeviceController::switchToProfile(const QString& profile){
    int len = _device->profiles().length();
    for(int i = 0; i < len; i++){
        KbProfile* loopProfile = _device->profiles().at(i);
        if(loopProfile->name() != profile)
            continue;

        qDebug() << "Switching" << _device->usbModel << "to" << profile;
        _device->setCurrentProfile(loopProfile);

        // Also update the dropdown
        emit profileSwitched(i);
        return;
    }
}

void DeviceController::switchToMode(const QString& mode){
    KbProfile* currentProfile = _device->currentProfile();
    int len = currentProfile->modes().length();

    for(int i = 0; i < len; i++){
        KbMode* loopMode = currentProfile->modes().at(i);
        if(loopMode->name() != mode)
            continue;

        qDebug() << "Switching" << _device->usbModel << "to mode" << mode << "in" << currentProfile->name();
        _device->setCurrentMode(loopMode);

        return;
    }
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
