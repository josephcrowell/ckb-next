#include "lightcontroller.h"
#include <QCoreApplication>
#include <QGuiApplication>
#include <limits>
#include "animationlistmodel.h"
#include "animscript.h"
#include "ckbsettings.h"
#include "kbanim.h"
#include "kblight.h"

LightController::LightController(QObject* parent) :
    QObject(parent), _light(nullptr), _animations(new AnimationListModel(this))
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
        disconnect(_light, &KbLight::didLoad, this, &LightController::reloadAnimations);
        // Disabled animated colors for previous light
        applyShowAnimated(false);
    }
    connect(newLight, &KbLight::updated, this, &LightController::lightUpdated);
    if(newLight)
        connect(newLight, &KbLight::didLoad, this, &LightController::reloadAnimations);
    _light = newLight;
    _animations->setAnimations(newLight ? newLight->animList() : KbLight::AnimList());
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

void LightController::reloadAnimations(){
    _animations->setAnimations(_light->animList());
    emit animationsReloaded();
}

QString LightController::animationName(const AnimScript* script, int presetId){
    QString animName = script->presets()[presetId];
    // If the preset is named "Default", then just use the script name
    if(!animName.compare("Default", Qt::CaseInsensitive))
        animName = script->name();
    else if(animName.compare(script->name(), Qt::CaseInsensitive)){
        // Append the animation script name otherwise, and only if preset and script name are not identical
        animName.append(" (");
        animName.append(script->name());
        animName.append(")");
    }
    return animName;
}

KbAnim* LightController::addAnimation(const AnimScript* script, int presetId, const QStringList& keys){
    if(!_light)
        return nullptr;
    KbAnim* animation = _light->addAnim(script, keys, animationName(script, presetId), script->preset(presetId));
    _animations->append(animation);
    return animation;
}

KbAnim* LightController::duplicateAnimation(KbAnim* anim){
    if(!_light)
        return nullptr;
    KbAnim* animation = _light->duplicateAnim(anim);
    _animations->setAnimations(_light->animList());
    return animation;
}

void LightController::deleteAnimation(KbAnim* anim){
    if(!_light || !anim)
        return;
    _animations->remove(anim);
    KbLight::AnimList animList = _light->animList();
    animList.removeAll(anim);
    _light->animList(animList);
    anim->deleteLater();
}

void LightController::reorderAnimations(const QList<KbAnim*>& order){
    if(!_light)
        return;
    _light->animList(order);
    _animations->setAnimations(order);
}

bool LightController::renameAnimation(KbAnim* anim, const QString& name){
    // Don't allow empty names
    if(!anim || name.isEmpty())
        return false;
    anim->name(name.trimmed());
    _animations->renamed(anim);
    return true;
}

void LightController::setAnimationOpacity(KbAnim* anim, double percent){
    if(anim)
        anim->opacity(percent / 100.);
}

void LightController::setAnimationBlendMode(KbAnim* anim, int mode){
    if(anim)
        anim->mode((KbAnim::Mode)mode);
}

void LightController::setAnimationKeys(KbAnim* anim, const QStringList& keys){
    if(!anim || !_light)
        return;
    QStringList newKeys = keys;
    // If any keys were selected previously that aren't in the keymap now, leave them in
    // This is important for layout compatibility - e.g. being able to select both bslash_iso and bslash, because no single layout contains both
    const KeyMap& map = _light->map();
    foreach(const QString& key, anim->keys()){
        if(!map.contains(key))
            newKeys << key;
    }
    anim->keys(newKeys);
}

void LightController::finishAnimationSettings(KbAnim* anim, bool accepted){
    if(!anim)
        return;
    if(!accepted){
        anim->resetParams();
        return;
    }
    // Apply settings and restart all animations
    anim->commitParams();
    anim->reInit();
    restartAnimation();
}

void LightController::restartAnimation(){
    if(_light)
        _light->restartAnimation();
}
