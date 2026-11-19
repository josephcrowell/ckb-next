#include "lightcontroller.h"
#include <QCoreApplication>
#include <QGuiApplication>
#include <limits>
#include "ckbsettings.h"
#include "kblight.h"

LightController::LightController(QObject* parent) :
    QObject(parent), _light(nullptr)
{
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, &LightController::applicationStateChanged);
    // Restore "show animated" setting
    _showAnimated = !CkbSettings::get("UI/Light/ShowBaseOnly").toBool();
}

void LightController::setLight(KbLight* newLight){
    if(_light == newLight)
        return;
    if(_light){
        disconnect(_light, &KbLight::updated, this, &LightController::lightUpdated);
        // Disabled animated colors for previous light
        applyShowAnimated(false);
    }
    connect(newLight, &KbLight::updated, this, &LightController::lightUpdated);
    _light = newLight;
    if(_showAnimated)
        applyShowAnimated(true);
}

bool LightController::setSelection(const QStringList& selection){
    if(_light == nullptr)
        return false;
    // Determine selected color (invalid color if no selection or if they're not all the same)
    QColor selectedColor;
    const QColorMap& colorMap = _light->colorMap();
    foreach(const QString& key, selection){
        QColor color = colorMap.value(key);
        if(!selectedColor.isValid())
            selectedColor = color;
        else if(color != selectedColor){
            selectedColor = QColor();
            break;
        }
    }
    _selection = selection;
    _selectedColor = selectedColor;
    return true;
}

QString LightController::selectionText() const {
    // Strings keep the KbLightWidget context so existing translations still apply
    int count = _selection.count();
    if(count == 0)
        return QCoreApplication::translate("KbLightWidget", "Click to select");
    const KeyMap& map = _light->map();
    const bool zones = !map.isKeyboard() || map.model() == KeyMap::K55 || map.model() == KeyMap::K55PRO;
    // This is done this way to aid translation
    if(count == 1)
        return zones ? QCoreApplication::translate("KbLightWidget", "1 zone selected")
                     : QCoreApplication::translate("KbLightWidget", "1 key selected");
    return zones ? QCoreApplication::translate("KbLightWidget", "%1 zones selected").arg(count)
                 : QCoreApplication::translate("KbLightWidget", "%1 keys selected").arg(count);
}

void LightController::setColor(const QColor& newColor){
    if(_light){
        foreach(QString key, _selection)
            _light->color(key, newColor);
        emit colorsChanged();
    }
}

void LightController::toggleSidelight(){
    if(_light){
        if (_light->colorMap()["lsidel"] == 0xFF000000){
                _light->color("lsidel",QRgb(0xFFFFFFFF));
                _light->color("rsidel",QRgb(0xFFFFFFFF));
        } else {
                _light->color("lsidel", QRgb(0xFF000000));
                _light->color("rsidel", QRgb(0xFF000000));
        }
        emit colorsChanged();
    }
}

// TODO: Merge with above
void LightController::toggleM95Light(){
    if(_light){
        if (_light->colorMap()["back"] == 0xFF000000)
            _light->color("back",QRgb(0xFFFFFFFF));
        else
            _light->color("back", QRgb(0xFF000000));
        emit colorsChanged();
    }
}

int LightController::brightness() const {
    return _light ? _light->dimming() : 0;
}

void LightController::setBrightness(int index){
    if(_light)
        _light->dimming(index);
}

void LightController::setShowAnimated(bool show){
    _showAnimated = show;
    applyShowAnimated(show);
}

void LightController::applyShowAnimated(bool show){
    // FIXME: set globally if there's more than one KbLightWidget active
    // Connect/disconnect animation slot
    if(show){
        if(_light)
            startAnimationPreview();
    } else {
        if(_light)
            stopAnimationPreview();
        emit previewFrame(ColorMap(), QSet<QString>(), std::numeric_limits<quint64>::max());
    }
    CkbSettings::set("UI/Light/ShowBaseOnly", !show);
}

void LightController::applicationStateChanged(Qt::ApplicationState state){
    if(!CkbSettings::get("Program/DisablePreviewOnFocusLoss", true).toBool())
        return;
    if (state == Qt::ApplicationActive) {
        if(_light && _showAnimated)
            startAnimationPreview();
    } else {
        if(_light)
            stopAnimationPreview();
        emit previewFrame(ColorMap(), QSet<QString>(), std::numeric_limits<quint64>::max());
    }
}

void LightController::startAnimationPreview(){
    connect(_light, &KbLight::frameDisplayed, this, &LightController::previewFrame);
}

void LightController::stopAnimationPreview(){
    disconnect(_light, &KbLight::frameDisplayed, this, &LightController::previewFrame);
}
