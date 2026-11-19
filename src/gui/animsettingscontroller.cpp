#include "animsettingscontroller.h"
#include <QColor>
#include <QCoreApplication>
#include <cstdio>
#include "kbanim.h"

AnimParamListModel::AnimParamListModel(QObject* parent) :
    QAbstractListModel(parent)
{
}

int AnimParamListModel::rowCount(const QModelIndex& parent) const {
    if(parent.isValid())
        return 0;
    return _params.count();
}

QVariant AnimParamListModel::data(const QModelIndex& index, int role) const {
    if(!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return QVariant();
    const AnimScript::Param& p = _params.at(index.row());
    switch(role){
    case Qt::DisplayRole:
    case NameRole:
        return p.name;
    case TypeRole:
        return (int)p.type;
    case PrefixRole:
        return p.prefix;
    case PostfixRole:
        return p.postfix;
    case MinimumRole:
        return p.minimum;
    case MaximumRole:
        return p.maximum;
    case ValueRole:
        return _values.at(index.row());
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> AnimParamListModel::roleNames() const {
    QHash<int, QByteArray> names = QAbstractListModel::roleNames();
    names[NameRole] = "name";
    names[TypeRole] = "type";
    names[PrefixRole] = "prefix";
    names[PostfixRole] = "postfix";
    names[MinimumRole] = "minimum";
    names[MaximumRole] = "maximum";
    names[ValueRole] = "value";
    return names;
}

void AnimParamListModel::setParams(const QList<AnimScript::Param>& params, const QList<QVariant>& values){
    beginResetModel();
    _params = params;
    _values = values;
    endResetModel();
}

void AnimParamListModel::setValue(const QString& name, const QVariant& value){
    for(int row = 0; row < _params.count(); row++){
        if(_params.at(row).name != name)
            continue;
        _values[row] = value;
        const QModelIndex i = index(row);
        emit dataChanged(i, i, {ValueRole});
        return;
    }
}

AnimSettingsController::AnimSettingsController(KbAnim* anim, QObject* parent) :
    QObject(parent), _anim(anim), _params(new AnimParamListModel(this)), _lastDuration(1.0)
{
    const AnimScript* script = anim->script();
    QList<AnimScript::Param> params;
    QList<QVariant> values;
    QListIterator<AnimScript::Param> i(script->paramIterator());
    while(i.hasNext()){
        const AnimScript::Param& param = i.next();
        // Skip timing/playback params
        if(param.name == "duration"
                || param.name == "trigger" || param.name == "kptrigger"
                || param.name == "kpmode"
                || param.name == "delay" || param.name == "kpdelay"
                || param.name == "repeat" || param.name == "kprepeat"
                || param.name == "stop" || param.name == "kpstop"
                || param.name == "kpmodestop" || param.name == "kprelease")
            continue;
        params.append(param);
        values.append(uiValue(param, anim->parameter(param.name)));
    }
    _params->setParams(params, values);
    if(script->hasParam("duration"))
        _lastDuration = anim->parameter("duration").toDouble();
    _hasRepeat = anim->hasParameter("repeat");
}

QVariant AnimSettingsController::uiValue(const AnimScript::Param& param, const QVariant& value){
    switch(param.type){
    case AnimScript::Param::BOOL:
        return value.toBool();
    case AnimScript::Param::LONG:
    case AnimScript::Param::ANGLE:
        return value.toInt();
    case AnimScript::Param::DOUBLE:
        return value.toDouble();
    case AnimScript::Param::RGB:
        return QColor("#" + value.toString());
    case AnimScript::Param::ARGB:{
        QString val = value.toString();
        QColor color;
        if(val.length() == 8){
            color = "#" + val.right(6);
            color.setAlpha(val.left(2).toInt(nullptr, 16));
        } else
            color = "#" + val;
        return color;
    }
    case AnimScript::Param::GRADIENT:
    case AnimScript::Param::AGRADIENT:
    case AnimScript::Param::STRING:
        return value.toString();
    default:
        return QVariant();
    }
}

void AnimSettingsController::setParam(const QString& name, const QVariant& uiValue){
    // Read value based on type
    switch(_anim->script()->param(name).type){
    case AnimScript::Param::BOOL:
        _anim->parameter(name, (int)uiValue.toBool());
        break;
    case AnimScript::Param::LONG:
    case AnimScript::Param::ANGLE:
        _anim->parameter(name, uiValue.toInt());
        break;
    case AnimScript::Param::DOUBLE:
        _anim->parameter(name, uiValue.toDouble());
        break;
    case AnimScript::Param::RGB:{
        const QColor color = uiValue.value<QColor>();
        char hex[7];
        snprintf(hex, sizeof(hex), "%02x%02x%02x", color.red(), color.green(), color.blue());
        _anim->parameter(name, QString(hex));
        break;
    }
    case AnimScript::Param::ARGB:{
        const QColor color = uiValue.value<QColor>();
        char hex[9];
        snprintf(hex, sizeof(hex), "%02x%02x%02x%02x", color.alpha(), color.red(), color.green(), color.blue());
        _anim->parameter(name, QString(hex));
        break;
    }
    case AnimScript::Param::GRADIENT:
    case AnimScript::Param::AGRADIENT:
    case AnimScript::Param::STRING:
        _anim->parameter(name, uiValue.toString());
        break;
    default:
        return;
    }
    _params->setValue(name, uiValue);
}

bool AnimSettingsController::flag(const QString& name) const {
    return _anim->parameter(name).toBool();
}

double AnimSettingsController::number(const QString& name) const {
    return _anim->parameter(name).toDouble();
}

bool AnimSettingsController::hasDuration() const {
    return _anim->script()->hasParam("duration");
}

bool AnimSettingsController::hasRepeat() const {
    return _hasRepeat;
}

bool AnimSettingsController::stopChecked(const QString& name) const {
    if(_hasRepeat)
        return _anim->parameter(name).toInt() < 0;
    return _anim->parameter(name).toDouble() > 0.;
}

QVariant AnimSettingsController::stopValue(const QString& name) const {
    if(_hasRepeat)
        return _anim->parameter(name).toInt();
    double stop = _anim->parameter(name).toDouble();
    if(stop <= 0.)
        return _lastDuration;
    return stop;
}

void AnimSettingsController::setStop(const QString& name, bool checked, const QVariant& uiValue){
    // stop and kpstop have defeat switches
    if(checked == _hasRepeat){
        _anim->parameter(name, -1);
        return;
    }
    setParam(name, uiValue);
}

QStringList AnimSettingsController::keypressModes() const {
    // Strings keep the AnimSettingDialog context so existing translations still apply
    if(_anim->script()->hasKeypress()){
        // If the script supports keypresses, show the option to handle them that way (default)
        return {QCoreApplication::translate("AnimSettingDialog", "on pressed key"),
                QCoreApplication::translate("AnimSettingDialog", "on whole keyboard"),
                QCoreApplication::translate("AnimSettingDialog", "on keyboard (once)")};
    }
    // Otherwise, just show the choice of whether to start it every time or just once
    return {QCoreApplication::translate("AnimSettingDialog", "every time"),
            QCoreApplication::translate("AnimSettingDialog", "only once")};
}

int AnimSettingsController::keypressModeIndex() const {
    int selected = _anim->parameter("kpmode").toInt();
    if(!_anim->script()->hasKeypress())
        selected--;
    if(selected < 0 || selected > keypressModes().count())
        selected = 0;
    return selected;
}

void AnimSettingsController::setKeypressMode(int index){
    // kpmode uses a drop-down, and selection is inverted
    int selected = index;
    if(!_anim->script()->hasKeypress())
        selected++;
    _anim->parameter("kpmode", selected);
}
