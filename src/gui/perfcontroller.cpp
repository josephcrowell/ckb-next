#include "perfcontroller.h"
#include <cmath>
#include "ckbsettings.h"
#include "kbmode.h"
#include "kbprofile.h"
#include "media.h"

static const QString xyLinkPath = "UI/DPI/UnlinkXY";

PerfController::PerfController(QObject* parent) :
    QObject(parent), _perf(nullptr), _profile(nullptr), _xyLink(!CkbSettings::get(xyLinkPath).toBool()), _darkCore(false)
{
}

///
/// \brief PerfController::modeFromRaw return hardware mode depending on settings in \a software and \a hardware
/// \see PerfController::rawFromMode for details.
///
PerfController::HwMode PerfController::modeFromRaw(bool software, KbPerf::i_hw hardware){
    if(software){
        if(hardware == KbPerf::NORMAL)
            return BOTH;
        return RGB;
    } else {
        if(hardware == KbPerf::ON)
            return ALWAYS_ON;
        else if(hardware == KbPerf::OFF)
            return ALWAYS_OFF;
        return NORMAL;
    }
}

///
/// \brief PerfController::rawFromMode Set values of software and hardware to the value corresponding to mode
/// mode determines how colors at a key and separate indicators should be handled:
/// Mode | sw | hw
/// _ | _ | _
/// NORMAL | No color change at the key | use the separate indicator depending on key state
/// ALWAYS_ON | No color change at the key | switch on separate indicator
/// ALWAYS_OFF | No color change at the key | switch off separate indicator
/// RGB | use color change at the key depending on color sliders | switch off separate indicator
/// BOTH | use color change at the key depending on color sliders | use the separate indicator depending on key state
///
void PerfController::rawFromMode(HwMode mode, bool& software, KbPerf::i_hw& hardware){
    switch(mode){
    case NORMAL:
        software = false;
        hardware = KbPerf::NORMAL;
        break;
    case ALWAYS_ON:
        software = false;
        hardware = KbPerf::ON;
        break;
    case ALWAYS_OFF:
        software = false;
        hardware = KbPerf::OFF;
        break;
    case RGB:
        software = true;
        hardware = KbPerf::OFF;
        break;
    case BOTH:
        software = true;
        hardware = KbPerf::NORMAL;
        break;
    }
}

void PerfController::setPerf(KbPerf* newPerf, KbProfile* newProfile){
    if(_perf != newPerf || !_dpiConnection){
        // Only the shown perf moves the stage highlight. Disconnecting through the handle is
        // safe even if the previous perf was deleted with its mode (the handle is then
        // invalid, which also catches a new perf at the old address).
        disconnect(_dpiConnection);
        _dpiConnection = connect(newPerf, &KbPerf::dpiChanged, this, &PerfController::dpiChanged);
    }
    _perf = newPerf;
    _profile = newProfile;
}

void PerfController::copyToModes(const QList<KbMode*>& modes){
    foreach(KbMode* mode, modes){
        *mode->perf() = *_perf;
    }
}

int PerfController::intensity() const {
    return std::round(_perf->iOpacity() * 100.f);
}

void PerfController::setIntensity(int percent){
    if(!_perf)
        return;
    _perf->iOpacity(percent / 100.f);
}

PerfController::Indicator PerfController::indicator(int index) const {
    Indicator i;
    _perf->getIndicator((KbPerf::indicator)index, i.color1, i.color2, i.color3, i.software, i.hardware);
    i.mode = modeFromRaw(i.software, i.hardware);
    return i;
}

bool PerfController::setIndicator(int index, bool enabled, int hwMode, const QColor& color1, const QColor& color2, const QColor& color3){
    if(!_perf)
        return false;
    // Read HW/SW enable state
    bool software = false;
    KbPerf::i_hw hardware = KbPerf::NONE;
    if(!isHardwareIndicator(index))
        software = enabled;
    else
        rawFromMode((HwMode)hwMode, software, hardware);
    _perf->setIndicator((KbPerf::indicator)index, color1, color2, color3, software, hardware);
    return software;
}

bool PerfController::hasK95Indicators() const {
    return _profile->keyMap().model() == KeyMap::K95;
}

bool PerfController::hasMuteIndicator() const {
    // Indicators unsupported by the Strafe
    return _profile->keyMap().model() != KeyMap::STRAFE;
}

bool PerfController::muteDeviceSupported(){
    return isMuteDeviceSupported();
}

int PerfController::muteDevice() const {
    return _perf->getMuteDevice();
}

void PerfController::setMuteDevice(int device){
    _perf->setMuteDevice(static_cast<::muteDevice>(device));
}

void PerfController::saveXyLinked(){
    CkbSettings::set(xyLinkPath, !_xyLink);
}

static inline int dpiExp(int value, int min, int max){
    const double DPI_BASE = (double)max / (double)KbPerf::DPI_MIN;
    double val = (value - min) / (double)(max - min);
    val = pow(DPI_BASE, val);
    return round(KbPerf::DPI_MIN * val);
}

static inline int dpiLog(int value, int min, int max){
    const double DPI_BASE = (double)max / (double)KbPerf::DPI_MIN;
    double val = value / (double)KbPerf::DPI_MIN;
    val = log(val) / log(DPI_BASE);
    return round(val * (max - min) + min);
}

static inline int dpiRound(int value){
    if(value > 1000)
        return round(value / 100.) * 100;
    else
        return round(value / 50.) * 50;
}

int PerfController::sliderToDpi(int position, int minimum, int maximum){
    return dpiRound(dpiExp(position, minimum, maximum));
}

int PerfController::dpiToSlider(int dpi, int minimum, int maximum){
    return dpiLog(dpi, minimum, maximum);
}

void PerfController::setDpiX(int index, int value){
    if(_xyLink)
        _perf->dpi(index, value);
    else
        _perf->dpi(index, QPoint(value, _perf->dpi(index).y()));
}

void PerfController::setDpiY(int index, int value){
    if(_xyLink)
        _perf->dpi(index, value);
    else
        _perf->dpi(index, QPoint(_perf->dpi(index).x(), value));
}

void PerfController::setDpiColor(int index, const QColor& color, bool allStages){
    if(allStages){
        for(int i = 0; i < KbPerf::DPI_COUNT; i++)
            _perf->dpiColor(i, color);
        _perf->dpiColor(KbPerf::OTHER, color);
    } else
        _perf->dpiColor(index, color);
}

void PerfController::setDpiEnabled(int index, bool enabled){
    _perf->dpiEnabled(index, enabled);
}

bool PerfController::dpiIndicator() const {
    return _perf->dpiIndicator();
}

void PerfController::setDpiIndicator(bool enabled){
    _perf->dpiIndicator(enabled);
}

bool PerfController::angleSnap() const {
    return _perf->angleSnap();
}

void PerfController::setAngleSnap(bool enabled){
    _perf->angleSnap(enabled);
}

int PerfController::liftHeightIndex() const {
    // Subtract 2 from profile values if using a dark core, since it only supports 3 heights
    if(_darkCore)
        return _perf->liftHeight() - 3;
    return _perf->liftHeight() - 1;
}

void PerfController::setLiftHeightIndex(int index){
    int height = index + 1;
    // Add 2 to the selected index, since the dark cores lift height ranges from 3 to 5, not 1 to 5
    if(_darkCore)
        height += 2;
    _perf->liftHeight((KbPerf::height)height);
}

int PerfController::dpiIndex() const {
    return _perf->getDpiIdx();
}
