#include "settingscontroller.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QProcess>
#include <QStandardPaths>
#include <ckbnextconfig.h>
#include "animscript.h"
#include "appcontroller.h"
#include "ckbsettings.h"
#include "idletimer.h"
#include "kb.h"
#include "kbbind.h"
#include "kblight.h"
#include "kbmanager.h"

// Modifier keys (OS-dependent)
static const QStringList modKeys = {"caps", "lshift", "lctrl", "lalt", "lwin"};

static inline QString right(const QString& left){
    if(left[0] != 'l')
        return left;
    return "r" + left.mid(1);
}

SettingsController::SettingsController(QObject* parent) : QObject(parent)
{
    KbBind::loadGlobalRemap();

#ifdef Q_OS_LINUX
    static const QString deletedAutorunPath = QStringLiteral("Program/DeletedOldAutostart");
    // If autostart was enabled, and there's an autostart file in XDG_CONFIG_DIRS, then delete the local one.
    if(!CkbSettings::get(deletedAutorunPath).toBool()) {
        CkbSettings::set(deletedAutorunPath, QVariant::fromValue<bool>(true));

        // Have to do this manually since Qt won't separate system paths from user ones
        QStringList config_dirs = QString::fromLocal8Bit(qgetenv("XDG_CONFIG_DIRS")).split(QChar(':'), Qt::SkipEmptyParts);
        if(config_dirs.isEmpty())
            config_dirs.append(QStringLiteral("/etc/xdg"));

        for(const QString& str : config_dirs) {
            QDir d(str);
            if(!d.exists() || !d.cd(QStringLiteral("autostart")))
                continue;

            if(d.exists(QStringLiteral("ckb-next.desktop"))) {
                qDebug() << "Found autostart file in" << d.absolutePath();
                // Path copy pasted from the old autorun.cpp
                static const QDir path(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.config/autostart");
                // ckb-next wouldn't let you modify the autostart file before, so if the file exists, it's a pretty good guarantee that it's the one we created
                // If the user modified it and made it read only, then this won't be able to remove it anyway
                QFile::remove(path.absoluteFilePath(QStringLiteral("ckb-next.autostart.desktop")));
                break;
            }
        }
    }
#endif

    CkbSettings settings("Program");

    // Read frame rate from settings
    int rate = settings.value("framerate").toInt();
    if(rate <= 0 || rate > 60)
        rate = 30;
    _frameRate = rate;
    Kb::frameRate(rate);

    // Read global brightness setting (default = on, 100% brightness)
    int dimming = settings.value("GlobalBrightness").toInt();
    if(dimming < -1 || dimming > KbLight::MAX_DIM)
        dimming = 0;
    _lastSharedDimming = dimming;
    KbLight::shareDimming(dimming);

    // Read dither
    Kb::dither(settings.value("Dither").toBool());

#if defined(Q_OS_MACOS) && defined(OS_MAC_LEGACY)
    // Read OSX settings
    Kb::mouseAccel(!settings.value("DisableMouseAccel").toBool());
    _scrollAccelDisabled = settings.value("DisableScrollAccel").toBool();
    _scrollSpeedValue = settings.value("ScrollSpeed", 3).toInt();
    Kb::scrollSpeed(_scrollAccelDisabled ? _scrollSpeedValue : 0);
#endif
}

QString SettingsController::copyrightText(){
    QString copyrightText = QString("© 2014-2016 <a href=\"https://github.com/ccMSC/\" style=\"text-decoration:none;\">ccMSC</a>.<br/>© 2017-%1 <a href=\"https://github.com/ckb-next/ckb-next/graphs/contributors\" style=\"text-decoration:none;\">%2</a>.").arg(CKB_NEXT_COPYRIGHT_YEAR, QCoreApplication::translate("SettingsWidget", "The ckb-next development team"));

#ifndef OS_MAC_LEGACY
#ifdef Q_OS_MACOS
    copyrightText.append(QCoreApplication::translate("SettingsWidget", "<br/>Special thanks to <a href=\"https://github.com/tekezo\" style=\"text-decoration:none;\">tekezo</a> for <a href=\"https://github.com/tekezo/Karabiner-VirtualHIDDevice\" style=\"text-decoration:none;\">VirtualHIDDevice</a>."));
#endif
#endif
    return copyrightText;
}

QStringList SettingsController::modifierNames(){
#ifdef Q_OS_MACOS
    return {"Caps Lock", "Shift", "Control (⌃)", "Option (⌥)", "Command (⌘)"};
#else
    return {"Caps Lock", "Shift", "Control", "Alt", "Super"};
#endif
}

int SettingsController::modifierIndex(const QString& key) const {
    return modKeys.indexOf(KbBind::globalRemap(key));
}

void SettingsController::setModifiers(int caps, int shift, int ctrl, int alt, int win){
    QHash<QString, QString> newMods;
    newMods["caps"] = modKeys[caps];
    newMods["lshift"] = modKeys[shift];
    newMods["rshift"] = right(modKeys[shift]);
    newMods["lctrl"] = modKeys[ctrl];
    newMods["rctrl"] = right(modKeys[ctrl]);
    newMods["lalt"] = modKeys[alt];
    newMods["ralt"] = right(modKeys[alt]);
    newMods["lwin"] = modKeys[win];
    newMods["rwin"] = right(modKeys[win]);
    KbBind::setGlobalRemap(newMods);
    KbBind::saveGlobalRemap();
}

#ifdef Q_OS_MACOS
bool SettingsController::migrateCmdSwap(){
    CkbSettings settings("Program");
    if(!settings.value("osxCmdSwap").toBool())
        return false;
    // ckb <0.0.41
    settings.remove("osxCmdSwap");
    // Keys not mapped to a modifier count as the first one, as the UI shows them
    auto index = [this](const QString& key){ return qMax(0, modifierIndex(key)); };
    setModifiers(index("caps"), index("lshift"), modKeys.indexOf("lwin"), index("lalt"), modKeys.indexOf("lctrl"));
    return true;
}
#endif

bool SettingsController::autoFirmwareCheck() const {
    return !CkbSettings("Program").value("DisableAutoFWCheck").toBool();
}

void SettingsController::setAutoFirmwareCheck(bool enabled){
    CkbSettings::set("Program/DisableAutoFWCheck", !enabled);
}

bool SettingsController::autoUpdateCheck() const {
    return !CkbSettings("Program").value("DisableAutoUpdCheck", false).toBool();
}

void SettingsController::setAutoUpdateCheck(bool enabled){
    CkbSettings::set("Program/DisableAutoUpdCheck", !enabled);
}

bool SettingsController::hiDpi() const {
    return CkbSettings("Program").value("HiDPI", false).toBool();
}

void SettingsController::setHiDpi(bool enabled){
    CkbSettings::set("Program/HiDPI", enabled);
}

void SettingsController::setFrameRate(int rate){
    _frameRate = rate;
    CkbSettings settings("Program");
    settings.setValue("framerate", rate);
    Kb::frameRate(rate);
}

bool SettingsController::perModeBrightness() const {
    // -1 = don't share
    return KbLight::shareDimming() == -1;
}

void SettingsController::setPerModeBrightness(bool enabled){
    KbLight::shareDimming(enabled ? -1 : 0);
    pollUpdates();
}

void SettingsController::pollUpdates(){
    // Check for changes to shared brightness setting
    int dimming = KbLight::shareDimming();
    // Don't do anything if the global brightness has been dimmed by the timer
    if(dimming != _lastSharedDimming && !CkbSettings::isBusy() && !KbManager::kbManager()->getDeviceTimerDimmed()){
        CkbSettings::set("Program/GlobalBrightness", dimming);
        _lastSharedDimming = dimming;
    }
}

bool SettingsController::dither() const {
    return Kb::dither();
}

void SettingsController::setDither(bool enabled){
    CkbSettings::set("Program/Dither", enabled);
    Kb::dither(enabled);
}

bool SettingsController::trayIconSuppressed() const {
    return CkbSettings("Program").value("SuppressTrayIcon").toBool();
}

void SettingsController::setTrayIconSuppressed(bool suppressed){
    CkbSettings::set("Program/SuppressTrayIcon", suppressed);
    AppController::instance()->setTrayVisible(!suppressed);
}

bool SettingsController::monochromeTrayIcon() const {
    return !CkbSettings("Program").value("RGBIcon", true).toBool();
}

void SettingsController::setMonochromeTrayIcon(bool monochrome){
    CkbSettings::set("Program/RGBIcon", !monochrome);
    AppController::instance()->refreshTrayIcon();
}

bool SettingsController::startDelay() const {
    return CkbSettings("Program").value("StartDelay").toBool();
}

void SettingsController::setStartDelay(bool enabled){
    CkbSettings::set("Program/StartDelay", enabled);
}

bool SettingsController::disablePreviewOnFocusLoss() const {
    return CkbSettings("Program").value("DisablePreviewOnFocusLoss", true).toBool();
}

void SettingsController::setDisablePreviewOnFocusLoss(bool disabled){
    CkbSettings::set("Program/DisablePreviewOnFocusLoss", disabled);
}

bool SettingsController::msaaDisabled() const {
    CkbSettings settings("Program");
    return settings.contains("GL/MSAA") && settings.value("GL/MSAA").toInt() == 0;
}

void SettingsController::setMsaaDisabled(bool disabled){
    CkbSettings s("Program/GL");
    if(disabled)
        s.setValue("MSAA", 0);
    else
        s.remove("MSAA");
}

bool SettingsController::idleTimerSupported(){
#ifdef Q_OS_LINUX
    return IdleTimer::isSupported();
#else
    return false;
#endif
}

bool SettingsController::idleTimerEnabled() const {
    return CkbSettings("Program").value("IdleTimerEnable", true).toBool();
}

void SettingsController::setIdleTimerEnabled(bool enabled){
#ifdef Q_OS_LINUX
    CkbSettings::set("Program/IdleTimerEnable", enabled);
    KbManager::setIdleTimer(enabled);
#else
    Q_UNUSED(enabled);
#endif
}

int SettingsController::idleTimerDuration() const {
    return CkbSettings("Program").value("IdleTimerDuration", 5).toInt();
}

void SettingsController::setIdleTimerDuration(int minutes){
#ifdef Q_OS_LINUX
    CkbSettings::set("Program/IdleTimerDuration", minutes);
    KbManager::setIdleTimer(false);
    KbManager::setIdleTimer(true);
#else
    Q_UNUSED(minutes);
#endif
}

#if defined(Q_OS_MACOS) && defined(OS_MAC_LEGACY)
bool SettingsController::mouseAccelDisabled() const {
    return !Kb::mouseAccel();
}

void SettingsController::setMouseAccelDisabled(bool disabled){
    CkbSettings::set("Program/DisableMouseAccel", disabled);
    Kb::mouseAccel(!disabled);
}

bool SettingsController::scrollAccelDisabled() const {
    return _scrollAccelDisabled;
}

void SettingsController::setScrollAccelDisabled(bool disabled){
    _scrollAccelDisabled = disabled;
    CkbSettings::set("Program/DisableScrollAccel", disabled);
    Kb::scrollSpeed(disabled ? _scrollSpeedValue : 0);
}

int SettingsController::scrollSpeed() const {
    return _scrollSpeedValue;
}

void SettingsController::setScrollSpeed(int speed){
    _scrollSpeedValue = speed;
    CkbSettings::set("Program/ScrollSpeed", speed);
    Kb::scrollSpeed(_scrollAccelDisabled ? speed : 0);
}
#endif

QString SettingsController::animationPaths(){
    return AnimScript::paths().join('\n');
}

QString SettingsController::rescanAnimations(){
    AnimScript::scan();
    int count = AnimScript::count();
    if(count == 0)
        return QCoreApplication::translate("ExtraSettingsWidget", "No animations found");
    else if(count == 1)
        return QCoreApplication::translate("ExtraSettingsWidget", "1 animation found");
    return QCoreApplication::translate("ExtraSettingsWidget", "%1 animations found").arg(count);
}

bool SettingsController::startReport(){
    // Don't allow the script to run twice
    if(_devDetect)
        return false;
    _devDetect = new QProcess();
    connect(_devDetect, &QProcess::finished, this, [this](int exitCode){ devDetectFinished(exitCode); });
    connect(_devDetect, &QProcess::destroyed, this, [this](){ _devDetect = nullptr; });
#if defined(Q_OS_LINUX)
    QString devDetectPath("ckb-next-dev-detect");
#elif defined(Q_OS_MACOS)
    QString devDetectPath("/Applications/ckb-next.app/Contents/Resources/ckb-next-dev-detect");
#endif
    _devDetect->start(devDetectPath, QStringList() << "--nouserinput");

    // Check if it was started successfully
    if(!_devDetect->waitForStarted()){
        _devDetect->deleteLater();
        return false;
    }
    return true;
}

void SettingsController::devDetectFinished(int exitCode){
    if(exitCode || !QFile::exists("/tmp/ckb-next-dev-detect-report.gz")){
        emit reportFinished(false, QString::fromUtf8(_devDetect->readAllStandardError()), exitCode);
        return;
    }
    emit reportFinished(true, QString(), exitCode);
}

bool SettingsController::saveReport(const QString& folder){
    QFile report("/tmp/ckb-next-dev-detect-report.gz");
    QString newfile(folder + "/ckb-next-report-" + QString::number(QDateTime::currentMSecsSinceEpoch()/1000) + ".gz");
    if(!report.copy(newfile))
        return false;
    _reportFolder = folder;
    _savedReport = newfile;
    return true;
}

void SettingsController::showReport(){
    // Try to show the newly generated file
#if defined(Q_OS_LINUX)
    if(QProcess::execute("sh", QStringList() << "-c" << "gtk-launch `xdg-mime query default inode/directory` '" + _savedReport + "'"))
        QProcess::execute("xdg-open", QStringList() << _reportFolder);
#elif defined(Q_OS_MACOS)
    QProcess::execute("open", QStringList() << "-R" << _savedReport);
#endif
}

void SettingsController::finishReport(bool removeReport){
    if(removeReport)
        QFile::remove("/tmp/ckb-next-dev-detect-report.gz");
    _reportFolder.clear();
    _savedReport.clear();
    if(_devDetect)
        _devDetect->deleteLater();
}

#ifdef Q_OS_MACOS
void SettingsController::uninstall(){
    // Open a terminal and run the uninstall script
    QProcess::execute("open", QStringList() << "-a" << "Terminal" << "/Applications/ckb-next.app/Contents/Resources/uninstall.sh");
}
#endif
