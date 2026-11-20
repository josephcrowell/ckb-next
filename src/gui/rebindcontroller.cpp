#include "rebindcontroller.h"
#include <QCoreApplication>
#include <QRegularExpression>
#include "kbanim.h"
#include "kbbind.h"
#include "kblight.h"
#include "kbmode.h"
#include "kbperf.h"
#include "kbprofile.h"
#include "keyaction.h"
#include "macrotablemodel.h"

const int RebindController::DPI_OFFSET = -KeyAction::DPI_CYCLE_UP + 1;
const int RebindController::DPI_CUSTOM_INDEX = KeyAction::DPI_CUSTOM + RebindController::DPI_OFFSET;

static const QString MACRO_MAGIC_COLON_REPLACEMENT = QStringLiteral("&das_IST_31N_col0n;");

RebindController::RebindController(QObject* parent) :
    QObject(parent), _bind(nullptr), _profile(nullptr), _macros(new MacroTableModel(this))
{
    // Populate key lists
    _keys[ModifierKeys] << "caps" << "lshift" << "rshift" << "lctrl" << "rctrl" << "lwin" << "rwin" << "lalt" << "ralt" << "rmenu" << "fn";
    _keys[FunctionKeys] << "esc" << "f1" << "f2" << "f3" << "f4" << "f5" << "f6" << "f7" << "f8" << "f9" << "f10" << "f11" << "f12" << "prtscn" << "scroll" << "pause" << "f13" << "f14" << "f15" << "f16" << "f17" << "f18" << "f19" << "f20";
    _keys[NumpadKeys] << "numlock" << "num0" << "num1" << "num2" << "num3" << "num4" << "num5" << "num6" << "num7" << "num8" << "num9" << "numslash" << "numstar" << "numminus" << "numplus" << "numdot" << "numenter";
    _keys[MediaKeys] << "stop" << "prev" << "play" << "next" << "volup" << "voldn" << "mute" << "eject";
    _keys[MouseButtons] << "mouse1" << "mouse2" << "mouse3";
    _keys[MouseExtraButtons] << "mouse4" << "mouse5" << "mouse6" << "mouse7" << "mouse8";
    _keys[WheelDirections] << "wheelup" << "wheeldn" << "wheellf" << "wheelrg";
#ifndef Q_OS_MACOS
    _keys[FunctionKeys] << "f21" << "f22" << "f23" << "f24";
#endif
    _keys[FunctionKeys] << "ins" << "del" << "home" << "end" << "pgup" << "pgdn" << "power" << "lightup" << "lightdn" << "up" << "down" << "left" << "right";

    // Display names
    foreach(const QString& key, _keys[ModifierKeys])
        _names[ModifierKeys] << KeyMap::friendlyName(key);
    foreach(const QString& key, _keys[FunctionKeys])
        _names[FunctionKeys] << KeyMap::friendlyName(key).replace("\n", " / ");
    foreach(const QString& key, _keys[NumpadKeys])
        _names[NumpadKeys] << KeyMap::friendlyName(key).remove("NumPad ");
    foreach(const QString& key, _keys[MediaKeys])
        _names[MediaKeys] << KeyMap::friendlyName(key);
#ifdef Q_OS_MACOS
    // Replace some OSX keys with their actual meanings
    _names[ModifierKeys][_keys[ModifierKeys].indexOf("lwin")] = "Left Cmd";
    _names[ModifierKeys][_keys[ModifierKeys].indexOf("rwin")] = "Right Cmd";
    _names[ModifierKeys][_keys[ModifierKeys].indexOf("lalt")] = "Left Option";
    _names[ModifierKeys][_keys[ModifierKeys].indexOf("ralt")] = "Right Option";
    _names[FunctionKeys][_keys[FunctionKeys].indexOf("prtscn")] = "F13";
    _names[FunctionKeys][_keys[FunctionKeys].indexOf("scroll")] = "F14";
    _names[FunctionKeys][_keys[FunctionKeys].indexOf("pause")] = "F15";
    _names[FunctionKeys][_keys[FunctionKeys].indexOf("f13")] = "F13 (Windows/Linux)";
    _names[FunctionKeys][_keys[FunctionKeys].indexOf("f14")] = "F14 (Windows/Linux)";
    _names[FunctionKeys][_keys[FunctionKeys].indexOf("f15")] = "F15 (Windows/Linux)";
    _names[FunctionKeys][_keys[FunctionKeys].indexOf("ins")] = "Help";
    _names[NumpadKeys][_keys[NumpadKeys].indexOf("numlock")] = "Clear";
#endif
}

void RebindController::setBind(KbBind* newBind, KbProfile* newProfile){
    _bind = newBind;
    _profile = newProfile;

    // Populate typing keys by position
    QStringList& typingKeys = _keys[TypingKeys];
    QStringList& typingNames = _names[TypingKeys];
    typingKeys.clear();
    typingNames.clear();
    // Use the K95 map as it has all keys
    const KeyMap& map = KeyMap(KeyMap::K95, _bind->map().layout());
    foreach(const QString& name, map.byPosition()){
        KeyAction action(_bind->defaultAction(name));
        if(action.isNormal() && !_keys[ModifierKeys].contains(name) && !_keys[FunctionKeys].contains(name) && !_keys[NumpadKeys].contains(name) && !_keys[MediaKeys].contains(name) && name != "enter" && name != "tab" && name != "bspace"){
            const Key& pos = map.key(name);
            typingNames << pos.friendlyName();
            typingKeys.append(name);
        }
    }
    typingKeys << "enter" << "tab" << "bspace";
    typingNames << "Enter" << "Tab" << "Backspace";
    if(!map.isISO()){
        // Add ISO backslash (aka KEY_102ND) to ANSI options
        typingKeys << "bslash_iso";
        typingNames << "Backslash (ISO layout)";
    }

    // Populate mode list
    _modeNames.clear();
    _modeNames << "(Previous)" << "(Next)";
    int idx = 1;
    foreach(KbMode* mode, newProfile->modes())
        _modeNames << QString("%1: %2").arg(idx++).arg(mode->name());
}

QStringList RebindController::dpiNames() const {
    QStringList names;
    if(!_bind->isMouse())
        return names;
    const KbPerf* perf = _bind->perf();
    for(int i = 0; i < KbPerf::DPI_COUNT; i++){
        bool sniper = (i == 0);
        QPoint dpi = perf->dpi(i);
        QString text = QCoreApplication::translate("RebindWidget", sniper ? "Sniper:\t%1 x %2" : "%3:\t%1 x %2").arg(dpi.x()).arg(dpi.y());
        if(!sniper) text = text.arg(i);
        names << text;
    }
    return names;
}

QStringList RebindController::animationNames() const {
    QStringList names;
    foreach(KbAnim* anim, _bind->light()->animList())
        names << anim->name();
    return names;
}

QString RebindController::commonAction() const {
    bool hasAction = false;
    QString action;
    if(_selection.count() == 1){
        action = _bind->action(_selection.first());
        if(!action.isEmpty())
            hasAction = true;
    } else {
        // Select an action from the UI if all keys are the same.
        // Select nothing if any are unbound or different
        foreach(const QString& key, _selection){
            QString keyAction = _bind->action(key);
            if(keyAction.isEmpty() || (hasAction && keyAction != action)){
                hasAction = false;
                break;
            } else if(action.isEmpty()){
                action = keyAction;
                hasAction = true;
            }
        }
    }
    if(!hasAction)
        action = "";
    return action;
}

RebindController::ActionForm RebindController::form(const QString& action) const {
    ActionForm f;
    KeyAction act(action);
    if(act.isMouse()){
        // Mouse buttons (indexOf returns -1 if not found, index zero is blank)
        f.kind = ActionForm::Mouse;
        f.tab = TAB_MOUSE;
        f.mouse = _keys[MouseButtons].indexOf(action) + 1;
        f.mouseExtra = _keys[MouseExtraButtons].indexOf(action) + 1;
        f.wheel = _keys[WheelDirections].indexOf(action) + 1;
        if(act.isDPI()){
            QPoint custom;
            int value = act.dpiInfo(custom);
            if(value <= -DPI_OFFSET || value > KeyAction::DPI_CUSTOM)
                return f;
            f.dpi = value + DPI_OFFSET;
            if(value == KeyAction::DPI_CUSTOM)
                f.dpiCustom = custom;
        }
    } else if(act.isNormal()){
        // Standard key
        f.kind = ActionForm::Key;
        f.tab = TAB_KB;
        f.typing = _keys[TypingKeys].indexOf(action) + 1;
        f.modifier = _keys[ModifierKeys].indexOf(action) + 1;
        f.function = _keys[FunctionKeys].indexOf(action) + 1;
        f.numpad = _keys[NumpadKeys].indexOf(action) + 1;
        f.media = _keys[MediaKeys].indexOf(action) + 1;
    } else if(act.isProgram()){
        // Program key
        f.kind = ActionForm::Program;
        f.tab = TAB_PROGRAM;
        int stop = act.programInfo(f.programPress, f.programRelease);
        switch(stop & 0x0F){
        case KeyAction::PROGRAM_PR_MULTI:
            break;
        default:
            f.pressSingle = true;
            f.pressMode = stop & 0x0F;
            break;
        }
        switch(stop & 0xF0){
        case KeyAction::PROGRAM_RE_MULTI:
            break;
        case KeyAction::PROGRAM_RE_INDEF:
            f.releaseSingle = true;
            f.releaseMode = 0;
            break;
        case KeyAction::PROGRAM_RE_KPSTOP:
            f.releaseSingle = true;
            f.releaseMode = 1;
            break;
        }
    } else if(act.isAnim()){
        // Animation key
        f.kind = ActionForm::Animation;
        f.tab = TAB_ANIM;
        bool onlyOnce = false, stopOnRelease = false;
        QUuid id = act.animInfo(onlyOnce, stopOnRelease);
        // Find this animation in the list. If not found, -1 will be returned, resulting in the blank space being selected
        f.animation = _bind->light()->findAnimIdx(id) + 1;
        f.animRepeat = !onlyOnce;
        f.animStopOnRelease = stopOnRelease;
    } else if(act.isSpecial()){
        // Other special keys
        f.kind = ActionForm::Special;
        f.tab = TAB_SPECIAL;
        int param;
        f.special = act.specialInfo(param);
        if(f.special == "mode"){
            // Mode selection. Check wrap-around flag
            if(param == KeyAction::MODE_PREV_WRAP)
                param = KeyAction::MODE_PREV;
            else if(param == KeyAction::MODE_NEXT_WRAP)
                param = KeyAction::MODE_NEXT;
            else if(param < 0)
                f.modeWrap = false;
            // Set mode to current selection, or to mode 1 if invalid
            param += 3;
            if(param >= 0 && param < _modeNames.count() + 1)
                f.mode = param;
            else
                // 0 -> "", 1 -> Prev, 2 -> Next, 3 -> Mode 1
                f.mode = 3;
        } else if(f.special == "macro"){
            f.tab = TAB_MACRO;
            if(act.isMacro()){
                // This string needs to exist for as long as the references below are used
                QString value = act.value();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                // Split the action and discard the "$macro" from the beginning
                QVector<QStringRef> macroData = value.midRef(7).split(QChar(':'));
#else
                QStringView valueView(value);
                QList<QStringView> macroData = valueView.sliced(7).split(QChar(':'));
#endif

                const int dataCount = macroData.count();
                // Macro strings have had different number of field depending on
                // their version:
                // - Initial version: 3 fields,
                // - next iteration: 4 fields, appended the raw macro data,
                //                   which might be "x" in certain situations,
                // - current iteration: 5 fields, appended repetition debounce & delay.
                if(3 <= dataCount && dataCount <= 5){
                    f.macroRead = true;
                    // Colons in the name field are escaped
                    f.macroName = macroData[2].toString();
                    f.macroName.replace(MACRO_MAGIC_COLON_REPLACEMENT, ":");

                    // Pick the appropriate string to parse due to legacy formats
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                    QStringRef macroString = macroData[0];
#else
                    QStringView macroString = macroData[0];
#endif
                    if(dataCount == 4 && macroData[3] != QStringLiteral("x"))
                        macroString = macroData[3];
                    f.macroString = macroString.toString();

                    // If the daemon macro string contains even a single delay, then the we set the
                    // delay to "as typed" instead of "default"
                    QRegularExpression re("=\\d+");
// Use different match function depending on Qt version, 6.5.0 deprecates
// QRegularExpression::match() in favor of QRegularExpression::matchView() when
// using a QStringView parameter.
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
                    f.macroAsTyped = re.matchView(macroData[0]).hasMatch();
#else // QT_VERSION < 6.5.0
                    f.macroAsTyped = re.match(macroData[0]).hasMatch();
#endif

                    // Set the repetition delay value if it's supplied.
                    if(dataCount == 5){
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                        QVector<QStringRef> repetitionData = macroData[4].split(QChar(';'));
#else // QT_VERSION >= 6.0.0
                        QList<QStringView> repetitionData = macroData[4].split(QChar(';'));
#endif
                        f.macroDebounce = repetitionData[0].toString();
                        f.macroDelay = repetitionData[1].toString();
                    } else {
                        f.macroDebounce = QString::number(MACRO_DEFAULT_REPETITION_DEBOUNCE);
                        f.macroDelay = QString::number(MACRO_DEFAULT_REPETITION_DELAY);
                    }
                }
            }
        }
        // Brightness control. Also check wrap
        if(f.special == "light"){
            f.lightWrap = true;
            if(param == KeyAction::LIGHT_DOWN_WRAP)
                param = KeyAction::LIGHT_DOWN;
            else if(param == KeyAction::LIGHT_UP_WRAP)
                param = KeyAction::LIGHT_UP;
            else
                f.lightWrap = false;
            if(param < 0 || param > 1)
                param = -1;
            f.light = param + 1;
        }
        // Win lock
        if(f.special == "lock"){
            if(param < 0 || param > 2)
                param = -1;
            f.lock = param + 1;
        }
    }
    return f;
}

void RebindController::loadMacro(const ActionForm& form){
    // If there's an error, we'll ignore it and parse as much as possible
    _macros->fromString(form.macroString, false);
}

bool RebindController::appliesMacro(const ActionForm& f) const {
    // Same order and tests as action()
    return f.typing <= 0 && f.modifier <= 0 && f.function <= 0 && f.numpad <= 0 && f.media <= 0
            && f.mouse <= 0 && f.mouseExtra <= 0 && f.wheel <= 0 && f.dpi <= 0 && f.mode <= 0
            && f.light <= 0 && f.lock <= 0 && f.animation <= 0
            && f.programPress.isEmpty() && f.programRelease.isEmpty() && _macros->length();
}

int RebindController::unmatchedMacroLine(const ActionForm& form) const {
    if(!appliesMacro(form))
        return -1;
    return _macros->isMacroMatched();
}

QString RebindController::action(const ActionForm& f) const {
    if(f.typing > 0)
        return _keys[TypingKeys][f.typing - 1];
    else if(f.modifier > 0)
        return _keys[ModifierKeys][f.modifier - 1];
    else if(f.function > 0)
        return _keys[FunctionKeys][f.function - 1];
    else if(f.numpad > 0)
        return _keys[NumpadKeys][f.numpad - 1];
    else if(f.media > 0)
        return _keys[MediaKeys][f.media - 1];
    else if(f.mouse > 0)
        return _keys[MouseButtons][f.mouse - 1];
    else if(f.mouseExtra > 0)
        return _keys[MouseExtraButtons][f.mouseExtra - 1];
    else if(f.wheel > 0)
        return _keys[WheelDirections][f.wheel - 1];
    else if(f.dpi > 0)
        return KeyAction::dpiAction(f.dpi - DPI_OFFSET, f.dpiCustom.x(), f.dpiCustom.y());
    else if(f.mode > 0)
        return KeyAction::modeAction(f.mode - 3 - (f.modeWrap && f.mode < 3 ? 2 : 0));
    else if(f.light > 0)
        return KeyAction::lightAction(f.light - 1 + (f.lightWrap ? 2 : 0));
    else if(f.lock > 0)
        return KeyAction::lockAction(f.lock - 1);
    else if(f.animation > 0){
        KbAnim* anim = _bind->light()->animList().at(f.animation - 1);
        return KeyAction::animAction(anim->guid(), !f.animRepeat, f.animStopOnRelease);
    } else if(!f.programPress.isEmpty() || !f.programRelease.isEmpty()){
        int kpStop = 0, krStop = 0;
        if(!f.programPress.isEmpty()){
            if(!f.pressSingle)
                kpStop = KeyAction::PROGRAM_PR_MULTI;
            else
                kpStop = f.pressMode;
        }
        if(!f.programRelease.isEmpty()){
            if(!f.releaseSingle)
                krStop = KeyAction::PROGRAM_RE_MULTI;
            else if(f.releaseMode == 0)
                krStop = KeyAction::PROGRAM_RE_INDEF;
            else
                krStop = KeyAction::PROGRAM_RE_KPSTOP;
        }
        return KeyAction::programAction(f.programPress, f.programRelease, kpStop | krStop);
    } else if(_macros->length()){
        // Generate macro format string
        // Format is $macro:daemon_string:preview_string:user_specified_macro_name:original_recorded_macro_string:repetition_debounce;repetition_delay
        // $macro: is added by KeyAction::macroAction()
        // daemon_string is the one sent to the daemon, and original_recorded_macro_string is the raw user data
        // preview_string is now unused as it can be generated from the remaining data
        // Any colons in user_specified_macro_name are escaped
        QString escapedMacroName = f.macroName;
        escapedMacroName.replace(":", MACRO_MAGIC_COLON_REPLACEMENT);
        QString finalDaemonString = _macros->toString(false);
        QString originalString = _macros->toString(true);
        QString macro = QString("%1::%2:%3:%4;%5").arg(finalDaemonString, escapedMacroName, originalString, f.macroDebounce, f.macroDelay);
        return KeyAction::macroAction(macro);
    }
    return QString();
}

void RebindController::apply(const QStringList& keys, const ActionForm& form, bool unbind){
    const QString newAction = action(form);
    if(!newAction.isNull())
        _bind->setAction(keys, newAction);
    else if(unbind)
        _bind->noAction(keys);
}

void RebindController::resetSelection(){
    _bind->resetAction(_selection);
}

void RebindController::unbindSelection(){
    _bind->noAction(_selection);
}
